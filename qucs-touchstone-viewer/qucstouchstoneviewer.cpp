#include "qucstouchstoneviewer.h"
#include <QHeaderView> // Required for QHeaderView
#include <QtMath> // For qDegreesToRadians and qRadiansToDegrees if needed, M_PI is in cmath
#include <QTemporaryFile> // Required for QTemporaryFile
#include <QLabel> // Required for QLabel (used in createWidgets)
#include <QClipboard> // For accessing the system clipboard
#include <QApplication> // Required for QApplication::clipboard()
#include <QMessageBox> // Already implicitly included by QFileDialog, but good to be explicit

#include <random>  // For std::mt19937, std::uniform_real_distribution (already in .h but good practice for .cpp)
#include <vector>  // For std::vector (already in .h)
#include <iomanip> // For std::fixed, std::setprecision (already in .h)
#include <sstream> // For std::ostringstream (already in .h)
#include <algorithm> // For std::sort

QTextEdit* QucsTouchstoneViewer::S_logOutputArea = nullptr;

QucsTouchstoneViewer::QucsTouchstoneViewer(QWidget *parent)
    : QMainWindow(parent)
{
    createWidgets();
    setWindowTitle(tr("Qucs Touchstone Viewer"));
    setMinimumSize(800, 600);

    S_logOutputArea = logOutputArea;
    qInstallMessageHandler(qtMessageHandler);

    qDebug() << "Touchstone Viewer initialized. Logging started.";
}

QucsTouchstoneViewer::~QucsTouchstoneViewer()
{
    // qInstallMessageHandler(nullptr);
}

void QucsTouchstoneViewer::createWidgets()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    QWidget *controlsWidget = new QWidget();
    QGridLayout *controlsLayout = new QGridLayout(controlsWidget);

    openButton = new QPushButton(tr("Select Touchstone File"), this);
    connect(openButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::openFile);

    loadInternalDataButton = new QPushButton(tr("Load Internal Test Data"), this);
    connect(loadInternalDataButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::loadInternalTestData);

    controlsLayout->addWidget(openButton, 0, 0);
    controlsLayout->addWidget(loadInternalDataButton, 0, 1);

    QLabel *networkTypeLabel = new QLabel(tr("Network Type:"), this);
    networkTypeComboBox = new QComboBox(this);
    networkTypeComboBox->addItem(tr("Inductor-pi"));

    synthesizeButton = new QPushButton(tr("Synthesize"), this);
    connect(synthesizeButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::onSynthesizeClicked);

    controlsLayout->addWidget(networkTypeLabel, 1, 0);
    controlsLayout->addWidget(networkTypeComboBox, 1, 1);
    controlsLayout->addWidget(synthesizeButton, 1, 2);

    dataTable = new QTableWidget(this);
    dataTable->setColumnCount(6);
    QStringList headers = {"Frequency (GHz)", "S11 (dB)", "S12 (dB)", "S21 (dB)", "S22 (dB)", "Z0 (Ohm)"};
    dataTable->setHorizontalHeaderLabels(headers);
    dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    logOutputArea = new QTextEdit(this);
    logOutputArea->setReadOnly(true);
    logOutputArea->setFontFamily("monospace");
    logOutputArea->setMinimumHeight(150);

    mainLayout->addWidget(controlsWidget);
    mainLayout->addWidget(dataTable, 1);
    mainLayout->addWidget(logOutputArea, 0);

    setCentralWidget(centralWidget);
}

void QucsTouchstoneViewer::openFile()
{
    if (logOutputArea) {
        logOutputArea->clear();
    }
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Touchstone File"),
                                                    "", tr("Touchstone files (*.s*p);;All files (*.*)"));
    if (!filePath.isEmpty()) {
        QMap<QString, QList<double>> data = readTouchstoneFile(filePath);
        if (!data.isEmpty() && data.contains("frequency") && !data["frequency"].isEmpty() && data["frequency"].size() > 0) {
            displayData(data);
        } else {
            qWarning() << "readTouchstoneFile returned empty or invalid data for:" << filePath;
            QMessageBox::warning(this, tr("Error"), tr("Could not read or parse valid data from the Touchstone file. Check logs for details."));
        }
    }
}

void QucsTouchstoneViewer::qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        break;
    case QtInfoMsg:
        txt = QString("Info: %1").arg(msg);
        break;
    }

    if (S_logOutputArea) {
         QMetaObject::invokeMethod(S_logOutputArea, "append", Qt::QueuedConnection, Q_ARG(QString, txt));
    }

    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
    } else {
        fprintf(stdout, "%s\n", msg.toLocal8Bit().constData());
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void QucsTouchstoneViewer::loadInternalTestData() {
    if(logOutputArea) {
        logOutputArea->clear();
    }
    qDebug() << "Loading internal S1P test data...";
    QString internalData =
        "# HZ S RI R 50\n"
        "1.00000000000000000000e+09 -8.46515980894904096488e-01 +3.82129464747608060815e-01\n"
        "1.04500000000000000000e+09 +7.66950422926827468650e-01 +5.06598385795697825351e-01\n"
        "1.09000000000000000000e+09 -7.40092995892248639578e-02 -9.18543147793533520939e-01\n"
        "1.13500000000000000000e+09 -7.84313619526967986673e-01 +4.87399953901208826679e-01\n"
        "1.18000000000000000000e+09 +8.27597192619389798729e-01 +3.86355153329461353806e-01\n"
        "1.22500000000000000000e+09 -1.98965544753984008297e-01 -8.95226442863656934890e-01\n"
        "1.27000000000000000000e+09 -7.08973922134685685670e-01 +5.83501861798306320495e-01\n"
        "1.31500000000000000000e+09 +8.70128641781341860550e-01 +2.58949445769425745656e-01\n"
        "1.36000000000000000000e+09 -3.18042669221875073937e-01 -8.55715355687028722542e-01\n"
        "1.40500000000000000000e+09 -6.21653232228549512683e-01 +6.68814615271009271780e-01\n"
        "1.45000000000000000000e+09 +8.93654065574718714515e-01 +1.27182133467183100528e-01\n"
        "1.49500000000000000000e+09 -4.29312049976689813491e-01 -8.01204252693833773868e-01\n"
        "1.54000000000000000000e+09 -5.23720562306002390685e-01 +7.41826867193043137938e-01\n"
        "1.58500000000000000000e+09 +8.97757786263793877701e-01 -6.01937001568708836274e-03\n"
        "1.63000000000000000000e+09 -5.31064362360373354299e-01 -7.33080849935221268154e-01\n"
        "1.67500000000000000000e+09 -4.16754460109290847392e-01 +8.01166944135256797743e-01\n"
        "1.72000000000000000000e+09 +8.82516757194892864646e-01 -1.37695855400943534264e-01\n"
        "1.76500000000000000000e+09 -6.21818176421991219982e-01 -6.52887024248738345733e-01\n"
        "1.81000000000000000000e+09 -3.02536121210643660362e-01 +8.45636179240584429095e-01\n";

    QTemporaryFile tempFile("test_internal_s1p_XXXXXX.s1p");
    if (tempFile.open()) {
        QTextStream out(&tempFile);
        out << internalData;
        tempFile.close();
        qDebug() << "Temporary internal S1P test file created at:" << tempFile.fileName();

        QMap<QString, QList<double>> data = readTouchstoneFile(tempFile.fileName());
        if (!data.isEmpty() && data.contains("frequency") && !data["frequency"].isEmpty() && data["frequency"].size() > 0) {
            displayData(data);
        } else {
            qWarning() << "Could not read or parse valid data from the internal S1P test data.";
            QMessageBox::warning(this, tr("Internal S1P Test Error"), tr("Could not read or parse valid data from the internal S1P test data. Check logs."));
        }
    } else {
        qWarning() << "Could not create temporary file for internal S1P test data.";
        QMessageBox::critical(this, tr("Internal S1P Test Error"), tr("Could not create temporary file for internal S1P test data."));
    }
}

void QucsTouchstoneViewer::onSynthesizeClicked()
{
    QString selectedNetwork = networkTypeComboBox->currentText();
    qDebug() << "Synthesize button clicked for network type:" << selectedNetwork;

    if (selectedNetwork == tr("Inductor-pi")) {
        logOutputArea->append("Synthesizing Inductor-pi network...");

        QString rShunt1Val = generateFormattedRandomValue(1.0, 100e3, "R");
        QString cShunt1Val = generateFormattedRandomValue(1e-12, 10e-6, "C");
        QString lSeriesVal = generateFormattedRandomValue(1e-9, 100e-3, "L");
        QString rSeriesVal = generateFormattedRandomValue(1.0, 100e3, "R");
        QString cShunt2Val = generateFormattedRandomValue(1e-12, 10e-6, "C");
        QString rShunt2Val = generateFormattedRandomValue(1.0, 100e3, "R");

        logOutputArea->append(QString("Generated values:"));
        logOutputArea->append(QString("  Rshunt1: %1").arg(rShunt1Val));
        logOutputArea->append(QString("  Cshunt1: %1").arg(cShunt1Val));
        logOutputArea->append(QString("  Lseries: %1").arg(lSeriesVal));
        logOutputArea->append(QString("  Rseries: %1").arg(rSeriesVal));
        logOutputArea->append(QString("  Cshunt2: %1").arg(cShunt2Val));
        logOutputArea->append(QString("  Rshunt2: %1").arg(rShunt2Val));

        QString schematicXml = QString(
            "<Qucs Schematic 25.1.2>\n"
            "<Components>\n"
            // Rshunt1: %1 used for property name and property value
            "<R R1 1 280 660 15 -26 0 1 \"%1\" 1 \"%1\" 0 \"0.0\" 0 \"0.0\" 0 \"26.85\" 0 \"european\" 0>\n"
            // Cshunt1: %2 used for property name and property value
            "<C C1 1 280 580 17 -26 0 1 \"%2\" 1 \"%2\" 0 \"neutral\" 0>\n"
            "<Port P1 1 560 510 4 -40 0 2 \"2\" 0 \"analog\" 0 \"v\" 0 \"\" 0>\n"
            "<Port P2 1 240 510 -23 -40 1 0 \"1\" 0 \"analog\" 0 \"v\" 0 \"\" 0>\n"
            // Lseries: %3 used for property name and property value
            "<L L1 1 360 510 -26 10 0 0 \"%3\" 1 \"%3\" 0>\n"
            // Rseries: %4 used for property name and property value
            "<R R3 1 460 510 -26 15 0 0 \"%4\" 1 \"%4\" 0 \"0.0\" 0 \"0.0\" 0 \"26.85\" 0 \"european\" 0>\n"
            // Cshunt2: %5 used for property name and property value
            "<C C2 1 540 580 17 -26 0 1 \"%5\" 1 \"%5\" 0 \"neutral\" 0>\n"
            // Rshunt2: %6 used for property name and property value
            "<R R2 1 540 660 15 -26 0 1 \"%6\" 1 \"%6\" 0 \"0.0\" 0 \"0.0\" 0 \"26.85\" 0 \"european\" 0>\n"
            "<GND * 1 540 710 0 0 0 0>\n"
            "<GND * 1 280 710 0 0 0 0>\n"
            "</Components>\n"
            "<Wires>\n"
            "<240 510 280 510 \"\" 0 0 0 \"\">\n"
            "<280 510 280 550 \"\" 0 0 0 \"\">\n"
            "<540 510 560 510 \"\" 0 0 0 \"\">\n"
            "<540 510 540 550 \"\" 0 0 0 \"\">\n"
            "<280 510 330 510 \"\" 0 0 0 \"\">\n"
            "<490 510 540 510 \"\" 0 0 0 \"\">\n"
            "<390 510 430 510 \"\" 0 0 0 \"\">\n"
            "<280 610 280 630 \"\" 0 0 0 \"\">\n"
            "<280 690 280 710 \"\" 0 0 0 \"\">\n"
            "<540 690 540 710 \"\" 0 0 0 \"\">\n"
            "<540 610 540 630 \"\" 0 0 0 \"\">\n"
            "</Wires>\n"
            "<Diagrams>\n"
            "</Diagrams>\n"
            "<Paintings>\n"
            "</Paintings>\n"
        ).arg(rShunt1Val).arg(cShunt1Val).arg(lSeriesVal).arg(rSeriesVal).arg(cShunt2Val).arg(rShunt2Val);

        QClipboard *clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(schematicXml);
            logOutputArea->append("Inductor-pi schematic XML copied to clipboard.");
            QMessageBox::information(this, tr("Synthesize Inductor-pi"), tr("Schematic XML for Inductor-pi network has been generated and copied to clipboard."));
        } else {
            logOutputArea->append("Error: Could not access clipboard.");
            QMessageBox::warning(this, tr("Synthesize Inductor-pi"), tr("Error: Could not access system clipboard."));
        }

    } else {
        QMessageBox::warning(this, tr("Synthesize"), QString(tr("Synthesis for '%1' is not implemented yet.")).arg(selectedNetwork));
    }
}

void QucsTouchstoneViewer::handleLogMessage(const QString& message) {
    if (logOutputArea) {
        logOutputArea->append(message);
    }
}

double QucsTouchstoneViewer::roundToNDecimals(double value, int n) {
    double multiplier = std::pow(10.0, n);
    return std::round(value * multiplier) / multiplier;
}

QString QucsTouchstoneViewer::generateFormattedRandomValue(double minVal, double maxVal, const QString& componentType)
{
    // Initialize random number generator (remains the same)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib(minVal, maxVal);
    double rawValue = distrib(gen);

    // Ensure rawValue is not exactly zero if minVal or maxVal are not, to avoid issues with log or division by zero with prefixes.
    // This is a pragmatic fix for potential edge cases with very small random numbers.
    if (rawValue == 0.0 && (minVal != 0.0 || maxVal != 0.0)) {
        // If it randomly hit 0.0 but the range wasn't centered on 0, pick a tiny non-zero or re-roll.
        // For simplicity, if minVal is positive, use a small fraction of minVal.
        if (minVal > 0) rawValue = minVal * 0.01 + std::numeric_limits<double>::epsilon();
        // else if maxVal is negative, use a small fraction of maxVal
        // else (range includes 0), 0.0 is fine.
        // This edge case handling might need more sophistication if ranges are tricky.
        // For now, we assume ranges are positive for R,L,C.
    }


    struct SIPrefix {
        double multiplier;
        QString prefixChar;
    };

    std::vector<SIPrefix> prefixes;

    // Define prefixes from largest to smallest multiplier
    if (componentType.toUpper() == "R") {
        prefixes = {
            {1e9, "G"}, {1e6, "M"}, {1e3, "k"},
            {1.0, ""},
            {1e-3, "m"}
        };
    } else if (componentType.toUpper() == "L") {
        prefixes = { // Henry based
            {1e9, "G"}, {1e6, "M"}, {1e3, "k"},
            {1.0, ""},
            {1e-3, "m"}, {1e-6, "u"}, {1e-9, "n"},
            {1e-12, "p"}
        };
    } else if (componentType.toUpper() == "C") {
        prefixes = { // Farad based
            // {1.0, ""}, // Base unit Farad is very large for typical components
            {1e-3, "m"}, {1e-6, "u"}, {1e-9, "n"},
            {1e-12, "p"}, {1e-15, "f"}
        };
         // For capacitors, it's common to start checking from smaller units.
         // So we will iterate from smallest to largest suitable for the [1,1000) range.
         // However, the list above is still defined largest to smallest for a consistent approach below.
    } else {
        qWarning() << "Unknown component type for random value generation:" << componentType;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << rawValue;
        return QString::fromStdString(oss.str());
    }

    QString bestPrefixChar = "";
    double bestScaledValue = rawValue; // Default to raw value, no prefix

    if (rawValue == 0.0) { // Handle zero value separately
        bestPrefixChar = ""; // No prefix for zero
        bestScaledValue = 0.0;
    } else {
        // Find the best prefix: iterate from largest to smallest
        // The goal is to find a prefix such that rawValue / prefix.multiplier is in [1.0, 1000.0)
        // If multiple fit, the first one (largest multiplier) is chosen.
        // If none make it into [1.0, 1000.0) by being too large (e.g. rawValue is > 1000 * largest_multiplier),
        // then use the largest_multiplier.
        // If none make it into [1.0, 1000.0) by being too small (e.g. rawValue is < 1.0 * smallest_multiplier),
        // then use the smallest_multiplier.

        bool foundIdealPrefix = false;
        if (!prefixes.empty()) {
            // Try to find a prefix that puts the value in the [1, 1000) range
            for (const auto& p : prefixes) {
                if (p.multiplier <= 0) continue; // Should not happen with SI units
                double scaledValue = rawValue / p.multiplier;
                if (scaledValue >= 1.0 && scaledValue < 1000.0) {
                    bestScaledValue = scaledValue;
                    bestPrefixChar = p.prefixChar;
                    foundIdealPrefix = true;
                    break;
                }
            }

            if (!foundIdealPrefix) {
                // If no ideal prefix, choose based on magnitude
                if (rawValue >= prefixes.front().multiplier * 1000.0 && prefixes.front().multiplier > 0) {
                    // Value is larger than 1000 * largest prefix, use largest prefix
                    bestScaledValue = rawValue / prefixes.front().multiplier;
                    bestPrefixChar = prefixes.front().prefixChar;
                } else {
                    // Value is smaller than 1.0 * smallest prefix (or any prefix that would make it >=1), use smallest prefix
                    // The prefixes vector is sorted largest to smallest, so .back() is smallest.
                    bestScaledValue = rawValue / prefixes.back().multiplier;
                    bestPrefixChar = prefixes.back().prefixChar;
                }
            }
        }
    }


    // Round the scaled value
    double finalValueRounded = roundToNDecimals(bestScaledValue, 2);

    // Adjust precision if rounding to 0.00 for a non-zero original value
    int precision = 2;
    if (finalValueRounded == 0.0 && rawValue != 0.0 && bestScaledValue != 0.0) {
        finalValueRounded = roundToNDecimals(bestScaledValue, 3);
        precision = 3;
        if (finalValueRounded == 0.0 && bestScaledValue != 0.0) {
            finalValueRounded = roundToNDecimals(bestScaledValue, 4);
            precision = 4;
        }
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << finalValueRounded;
    QString valueStr = QString::fromStdString(oss.str());

    if (bestPrefixChar.isEmpty()) {
        return valueStr;
    } else {
        return valueStr + " " + bestPrefixChar;
    }
}


// The rest of the file (convert_MA_RI_to_dB, readTouchstoneFile, displayData) remains the same as in the last complete version.
// For brevity, I'm not repeating them here, but they are part of the overwritten file content.

void QucsTouchstoneViewer::convert_MA_RI_to_dB(double *S_val1, double *S_val2, double *S_re_out, double *S_im_out, QString format)
{
    double input_val1 = *S_val1;
    double input_val2 = *S_val2;
    double s_db, s_ang_deg, s_re, s_im;

    format = format.toUpper();

    if (format == "MA") {
        if (input_val1 < 0) {
            qWarning() << "Magnitude (MA) is negative:" << input_val1 << ". Using abs().";
            input_val1 = std::abs(input_val1);
        }
        s_db = 20.0 * log10(input_val1 > std::numeric_limits<double>::epsilon() ? input_val1 : std::numeric_limits<double>::min());
        s_ang_deg = input_val2;
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = input_val1 * std::cos(ang_rad);
        s_im = input_val1 * std::sin(ang_rad);
    } else if (format == "RI") {
        s_re = input_val1;
        s_im = input_val2;
        double mag = std::sqrt(s_re * s_re + s_im * s_im);
        s_db = 20.0 * log10(mag > std::numeric_limits<double>::epsilon() ? mag : std::numeric_limits<double>::min());
        s_ang_deg = qRadiansToDegrees(std::atan2(s_im, s_re));
    } else if (format == "DB") {
        s_db = input_val1;
        s_ang_deg = input_val2;
        double mag_lin = std::pow(10.0, s_db / 20.0);
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = mag_lin * std::cos(ang_rad);
        s_im = mag_lin * std::sin(ang_rad);
    } else {
        qWarning() << "Unknown S-parameter format specified in file: '" << format << "'. Assuming MA.";
        if (input_val1 < 0) {
             qWarning() << "Magnitude (MA assumed) is negative:" << input_val1 << ". Using abs().";
             input_val1 = std::abs(input_val1);
        }
        s_db = 20.0 * log10(input_val1 > std::numeric_limits<double>::epsilon() ? input_val1 : std::numeric_limits<double>::min());
        s_ang_deg = input_val2;
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = input_val1 * std::cos(ang_rad);
        s_im = input_val1 * std::sin(ang_rad);
    }

    *S_val1 = s_db;
    *S_val2 = s_ang_deg;
    *S_re_out = s_re;
    *S_im_out = s_im;
}


QMap<QString, QList<double>> QucsTouchstoneViewer::readTouchstoneFile(const QString& filePath)
{
    QMap<QString, QList<double>> file_data;
    QString frequency_unit_str, parameter_str, format_str;
    double freq_scale_to_ghz = 1.0;
    double Z0 = 50.0;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Error: Cannot open the file:" << filePath;
        return file_data;
    }

    QTextStream in(&file);
    int number_of_ports = 0;
    bool options_line_parsed = false;
    bool first_data_line = true;

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    if (suffix.startsWith('s') && suffix.endsWith('p')) {
        bool ok;
        QString n_str = suffix.mid(1, suffix.length() - (suffix.endsWith("p") ? 2 : 1) );
        int n = n_str.toInt(&ok);
        if (ok && n > 0) {
            number_of_ports = n;
            qDebug() << "Number of ports from extension (" << suffix << "):" << number_of_ports;
        } else {
            qWarning() << "Could not parse N from file extension:" << suffix;
        }
    }

    qDebug() << "Starting to parse Touchstone file:" << filePath;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        qDebug() << "Read line:" << line;

        if (line.isEmpty() || line.startsWith('!')) {
            qDebug() << "Skipping empty or comment line.";
            continue;
        }

        if (line.startsWith('#')) {
            if (options_line_parsed) {
                 qWarning() << "Warning: Multiple option lines ('#') found. Using the first one's settings.";
                 continue;
            }
            options_line_parsed = true;
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

            if (parts.length() > 1) frequency_unit_str = parts[1].toLower();
            if (parts.length() > 2) parameter_str = parts[2].toLower();
            if (parts.length() > 3) format_str = parts[3].toLower();

            bool z0_found_keyword = false;
            for(int k=4; k < parts.length(); ++k) {
                if(parts[k].toLower() == "r" || parts[k].toLower() == "z0") {
                    if (k+1 < parts.length()) {
                        bool ok_z0;
                        double parsed_Z0 = parts[k+1].toDouble(&ok_z0);
                        if(ok_z0) {
                            Z0 = parsed_Z0;
                            z0_found_keyword = true;
                        } else {
                            qWarning() << "Warning: Could not parse Z0 value after R/Z0 keyword: " << parts[k+1];
                        }
                        break;
                    }
                }
            }
            if (!z0_found_keyword && parts.length() >= 5) {
                QString potential_z0_str = parts[4].toLower();
                if (potential_z0_str != "r" && potential_z0_str != "z0") { // ensure it's not a keyword
                    bool ok_z0_fallback;
                    double z_check = parts[4].toDouble(&ok_z0_fallback);
                    if (ok_z0_fallback) Z0 = z_check;
                }
            }

            if (frequency_unit_str == "hz") freq_scale_to_ghz = 1e-9;
            else if (frequency_unit_str == "khz") freq_scale_to_ghz = 1e-6;
            else if (frequency_unit_str == "mhz") freq_scale_to_ghz = 1e-3;
            else if (frequency_unit_str == "ghz") freq_scale_to_ghz = 1.0;
            else {
                if (!frequency_unit_str.isEmpty()) {
                    qWarning() << "Warning: Unknown frequency unit '" << frequency_unit_str << "'. Assuming GHz.";
                }
                freq_scale_to_ghz = 1.0;
            }
            qDebug() << "Options line parsed: FreqUnit=" << frequency_unit_str << "Param=" << parameter_str << "Format=" << format_str << "Z0=" << Z0 << "FreqScaleToGHz=" << freq_scale_to_ghz;
            continue;
        }

        if (!options_line_parsed) {
            qWarning() << "Warning: Data line encountered before option line ('#'). Assuming defaults: GHZ S MA R 50.";
            frequency_unit_str = "ghz"; parameter_str = "s"; format_str = "ma"; Z0 = 50.0; freq_scale_to_ghz = 1.0;
            options_line_parsed = true;
        }

        bool first_char_is_number = false;
        if (!line.isEmpty()) {
            QChar firstChar = line.at(0);
            first_char_is_number = firstChar.isDigit() || firstChar == '.' || firstChar == '-' || firstChar == '+';
        }

        if (!first_char_is_number) {
            if (file_data.contains("frequency") && !file_data["frequency"].isEmpty()) {
                qDebug() << "Non-numeric data line after S-parameters, stopping S-parameter read:" << line;
                break;
            } else {
                qDebug() << "Skipping non-numeric data line (and no data read yet):" << line;
                continue;
            }
        }

        QStringList values = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (values.isEmpty()) {
            qDebug() << "Skipping line that resulted in empty values list after split.";
            continue;
        }
        qDebug() << "Data line values:" << values;

        if (first_data_line && number_of_ports == 0) {
            if (values.length() > 1) {
                int s_param_data_count = values.length() - 1;
                if (parameter_str.toUpper() == "S" && s_param_data_count > 0 && s_param_data_count % 2 == 0) {
                    int n_squared = s_param_data_count / 2;
                    double n_double = std::sqrt(n_squared);
                    if (std::fmod(n_double, 1.0) == 0.0 && n_double > 0) {
                        number_of_ports = static_cast<int>(n_double);
                        qDebug() << "Number of ports determined from first data line:" << number_of_ports;
                    }
                }
            }
            if (number_of_ports == 0) {
                qWarning() << "Error: Could not determine number of ports for file:" << filePath << ". File extension was not sNp or first data line malformed/non-S type. Line:" << line;
                file.close();
                return QMap<QString, QList<double>>();
            }
        }
        first_data_line = false;

        if (number_of_ports == 0) {
            qWarning() << "Error: Number of ports is 0. Cannot process data line:" << line;
            continue;
        }

        bool freq_ok;
        double freq_val = values[0].toDouble(&freq_ok);
        if (!freq_ok) {
            qWarning() << "Warning: Could not parse frequency from value:" << values[0] << "on line:" << line << ". Skipping line.";
            continue;
        }
        file_data["frequency"].append(freq_val * freq_scale_to_ghz);
        file_data["Z0"].append(Z0);
        qDebug() << "Stored Frequency (GHz):" << (freq_val * freq_scale_to_ghz) << "Z0:" << Z0;

        int current_val_idx = 1;
        int expected_s_param_pairs = number_of_ports * number_of_ports;

        for (int i_port = 1; i_port <= number_of_ports; ++i_port) {
            for (int j_port = 1; j_port <= number_of_ports; ++j_port) {
                QString s_param_mag_key = QString("S%1%2_dB").arg(i_port).arg(j_port);
                QString s_param_ang_key = QString("S%1%2_ang").arg(i_port).arg(j_port);
                QString s_param_re_key = QString("S%1%2_re").arg(i_port).arg(j_port);
                QString s_param_im_key = QString("S%1%2_im").arg(i_port).arg(j_port);

                if (current_val_idx + 1 < values.length()) {
                    bool val1_ok, val2_ok;
                    double val1 = values[current_val_idx].toDouble(&val1_ok);
                    double val2 = values[current_val_idx + 1].toDouble(&val2_ok);

                    if (!val1_ok || !val2_ok) {
                        qWarning() << "Warning: Could not parse S-parameter data pair:" << values[current_val_idx] << "," << values[current_val_idx+1] << "on line:" << line;
                        file_data[s_param_mag_key].append(std::numeric_limits<double>::quiet_NaN());
                        file_data[s_param_ang_key].append(std::numeric_limits<double>::quiet_NaN());
                        file_data[s_param_re_key].append(std::numeric_limits<double>::quiet_NaN());
                        file_data[s_param_im_key].append(std::numeric_limits<double>::quiet_NaN());
                    } else {
                        double s_re_val, s_im_val;
                        convert_MA_RI_to_dB(&val1, &val2, &s_re_val, &s_im_val, format_str);
                        file_data[s_param_mag_key].append(val1);
                        file_data[s_param_ang_key].append(val2);
                        file_data[s_param_re_key].append(s_re_val);
                        file_data[s_param_im_key].append(s_im_val);
                        qDebug() << QString("S%1%2: dB=").arg(i_port).arg(j_port) << val1 << "Ang=" << val2 << "Re=" << s_re_val << "Im=" << s_im_val;
                    }
                    current_val_idx += 2;
                } else {
                    qDebug() << "Incomplete data on line for S" << i_port << j_port << " - Appending NaN. Line:" << line;
                    file_data[s_param_mag_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_ang_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_re_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_im_key].append(std::numeric_limits<double>::quiet_NaN());
                    if (current_val_idx < values.length()) current_val_idx++;
                }
            }
        }

        int s_params_read_on_line = (current_val_idx -1) / 2;
         if (s_params_read_on_line < expected_s_param_pairs) {
             qWarning() << "Warning: Data line seems incomplete or S-parameters span multiple lines. Expected"
                        << expected_s_param_pairs << "S-parameter pairs, processed" << s_params_read_on_line
                        << "from line:" << line;
         }
    }

    if (file_data.contains("n_ports")) {
        file_data["n_ports"].clear();
        file_data["n_ports"].append(static_cast<double>(number_of_ports));
    } else {
        file_data.insert("n_ports", QList<double>{static_cast<double>(number_of_ports)});
    }

    qDebug() << "Finished parsing. Total frequency points:" << (file_data.contains("frequency") ? file_data["frequency"].size() : 0)
             << ". Confirmed ports:" << number_of_ports;
    file.close();
    return file_data;
}

void QucsTouchstoneViewer::displayData(const QMap<QString, QList<double>>& data)
{
    dataTable->clearContents();

    if (!data.contains("frequency") || data["frequency"].isEmpty()) {
        dataTable->setRowCount(0);
        // Message box logic from your previous version
        if (data.isEmpty()) {
            // Message may have been shown by openFile if readTouchstoneFile returned empty
        } else {
             if (!(data.contains("frequency") && !data["frequency"].isEmpty())) { // Check specifically if frequency is the issue
                 QMessageBox::information(this, tr("Info"), tr("File parsed, but no valid frequency data points found."));
            } else { // This case should ideally not be reached if the outer condition is true
                 QMessageBox::information(this, tr("Info"), tr("No frequency data points found in the file."));
            }
        }
        return;
    }

    const QList<double>& freq = data["frequency"];
    int numRowsToShow = qMin(10, freq.size());
    dataTable->setRowCount(numRowsToShow);
    qDebug() << "Displaying" << numRowsToShow << "rows.";

    int number_of_ports = 0;
    if (data.contains("n_ports") && !data["n_ports"].isEmpty()) {
        number_of_ports = static_cast<int>(data["n_ports"].first());
    }
    qDebug() << "Displaying data for" << number_of_ports << "-port file.";

    QStringList sParamTableColumns = {"11", "12", "21", "22"}; // Corresponds to table columns 1, 2, 3, 4

    for (int i = 0; i < numRowsToShow; ++i) {
        // Frequency - Column 0
        dataTable->setItem(i, 0, new QTableWidgetItem(QString::number(freq.at(i), 'g', 10)));

        // S-parameters - Columns 1 to 4
        for (int j = 0; j < sParamTableColumns.size(); ++j) {
            QString current_s_param_index = sParamTableColumns.at(j);
            QString s_param_key_db = QString("S%1_dB").arg(current_s_param_index);

            bool should_display_sparam = false;
            if (number_of_ports == 1) {
                if (current_s_param_index == "11") {
                    should_display_sparam = true;
                }
            } else if (number_of_ports >= 2) {
                should_display_sparam = true;
            }

            if (should_display_sparam && data.contains(s_param_key_db) && i < data[s_param_key_db].size()) {
                double val = data[s_param_key_db].at(i);
                dataTable->setItem(i, j + 1, new QTableWidgetItem(std::isnan(val) ? "NaN" : QString::number(val, 'f', 4)));
            } else {
                dataTable->setItem(i, j + 1, new QTableWidgetItem("N/A"));
            }
        }

        // Z0 - Column 5
        if (data.contains("Z0") && i < data["Z0"].size()) {
             dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data["Z0"].at(i), 'f', 2)));
        } else if (data.contains("Z0") && !data["Z0"].isEmpty()){
            dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data["Z0"].first(), 'f', 2)));
        } else {
            dataTable->setItem(i, 5, new QTableWidgetItem("50.00 (default)"));
        }
    }
}
