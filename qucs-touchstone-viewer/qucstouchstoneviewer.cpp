#include "qucstouchstoneviewer.h"
#include <QHeaderView> // Required for QHeaderView
#include <QtMath> // For qDegreesToRadians and qRadiansToDegrees if needed, M_PI is in cmath
#include <QTemporaryFile> // Required for QTemporaryFile

QTextEdit* QucsTouchstoneViewer::S_logOutputArea = nullptr;

QucsTouchstoneViewer::QucsTouchstoneViewer(QWidget *parent)
    : QMainWindow(parent)
{
    createWidgets(); // This will now also create logOutputArea and loadInternalDataButton
    setWindowTitle(tr("Qucs Touchstone Viewer"));
    setMinimumSize(800, 600); // Increased size for log area

    // Set the static pointer for the message handler
    S_logOutputArea = logOutputArea;
    // Install the custom message handler
    qInstallMessageHandler(qtMessageHandler);

    qDebug() << "Touchstone Viewer initialized. Logging started.";
}

QucsTouchstoneViewer::~QucsTouchstoneViewer()
{
    // It's good practice to uninstall the message handler if the object providing the log area is destroyed
    // to prevent calls to a dangling S_logOutputArea.
    // However, since QucsTouchstoneViewer is the main window, it's likely S_logOutputArea
    // will be valid for the application's lifetime. If not, more careful handling is needed.
    // qInstallMessageHandler(nullptr); // Or restore previous handler if saved.
}

void QucsTouchstoneViewer::createWidgets()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget); // Renamed to mainLayout

    // Top part for buttons and table
    QWidget *topWidget = new QWidget();
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);

    openButton = new QPushButton(tr("Select Touchstone File"), this);
    connect(openButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::openFile);

    loadInternalDataButton = new QPushButton(tr("Load Internal Test Data"), this);
    connect(loadInternalDataButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::loadInternalTestData);

    topLayout->addWidget(openButton);
    topLayout->addWidget(loadInternalDataButton);

    dataTable = new QTableWidget(this);
    dataTable->setColumnCount(6);
    QStringList headers = {"Frequency (GHz)", "S11 (dB)", "S12 (dB)", "S21 (dB)", "S22 (dB)", "Z0 (Ohm)"};
    dataTable->setHorizontalHeaderLabels(headers);
    dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Log area at the bottom
    logOutputArea = new QTextEdit(this);
    logOutputArea->setReadOnly(true);
    logOutputArea->setFontFamily("monospace"); // Good for logs
    logOutputArea->setMinimumHeight(150); // Give it some initial height

    // Add widgets to main layout
    mainLayout->addWidget(topWidget);
    mainLayout->addWidget(dataTable, 1); // Give table more stretch factor
    mainLayout->addWidget(logOutputArea, 0); // Log area takes remaining space initially

    setCentralWidget(centralWidget);
}

void QucsTouchstoneViewer::openFile()
{
    if (logOutputArea) { // Ensure logOutputArea is valid
        logOutputArea->clear(); // Clear previous logs
    }
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Touchstone File"),
                                                    "", tr("Touchstone files (*.s*p);;All files (*.*)"));
    if (!filePath.isEmpty()) {
        QMap<QString, QList<double>> data = readTouchstoneFile(filePath);
        // Check data validity more thoroughly before calling displayData
        if (!data.isEmpty() && data.contains("frequency") && !data["frequency"].isEmpty() && data["frequency"].size() > 0) {
            displayData(data);
        } else {
            qWarning() << "readTouchstoneFile returned empty or invalid data for:" << filePath;
            QMessageBox::warning(this, tr("Error"), tr("Could not read or parse valid data from the Touchstone file. Check logs for details."));
        }
    }
}

// Custom Qt Message Handler
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

    // Also print to console (stderr for warnings/errors, stdout for debug/info)
    if (type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg) {
        fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
    } else {
        fprintf(stdout, "%s\n", msg.toLocal8Bit().constData());
    }

    if (type == QtFatalMsg) {
        abort(); // For fatal errors, abort as Qt's default handler would.
    }
}

void QucsTouchstoneViewer::loadInternalTestData() {
    if (logOutputArea) {
        logOutputArea->clear(); // Clear previous logs
    }
    qDebug() << "Loading internal test data...";
    QString internalData =
        "# HZ S RI R 50\n"
        "1.00000000000000000000e+09 -9.80710605771547339060e-01 +1.95465355819101765933e-01 +9.57923040231001455972e-06 +4.80619842391346986199e-05 +9.57923040231001455972e-06 +4.80619842391346986199e-05 -9.80710605771547339060e-01 +1.95465355819101765933e-01\n"
        "1.04500000000000000000e+09 -9.78880267386821323328e-01 +2.04434386424487635203e-01 +1.25489219116246239109e-05 +6.00872106259126012649e-05 +1.25489219116246239109e-05 +6.00872106259126012649e-05 -9.78880267386821323328e-01 +2.04434386424487635203e-01\n"
        "1.09000000000000000000e+09 -9.76959039486574365441e-01 +2.13426871222935438110e-01 +1.62617526123439660383e-05 +7.44379848774005868726e-05 +1.62617526123439660383e-05 +7.44379848774005868726e-05 -9.76959039486574365441e-01 +2.13426871222935438110e-01\n"
        "1.13500000000000000000e+09 -9.74945474762582309225e-01 +2.22443953485435913509e-01 +2.08644033683502456034e-05 +9.14462062414572263245e-05 +2.08644033683502456034e-05 +9.14462062414572263245e-05 -9.74945474762582309225e-01 +2.22443953485435941264e-01\n"
        "1.18000000000000000000e+09 -9.72838039368598250789e-01 +2.31486794497696934947e-01 +2.65258441025909904459e-05 +1.11476554096129074696e-04 +2.65258441025909904459e-05 +1.11476554096129074696e-04 -9.72838039368598250789e-01 +2.31486794497696934947e-01\n"
        "1.22500000000000000000e+09 -9.70635109149991071043e-01 +2.40556574554572549784e-01 +3.34399207454875673873e-05 +1.34928597078936565127e-04 +3.34399207454875673873e-05 +1.34928597078936565127e-04 -9.70635109149990960020e-01 +2.40556574554572522029e-01\n"
        "1.27000000000000000000e+09 -9.68334965577859474450e-01 +2.49654493987659570342e-01 +4.18281876468712198553e-05 +1.62239004787232164251e-04 +4.18281876468712198553e-05 +1.62239004787232164251e-04 -9.68334965577859474450e-01 +2.49654493987659542586e-01\n"
        "1.31500000000000000000e+09 -9.65935791364279427995e-01 +2.58781774227693439627e-01 +5.19430088755720511542e-05 +1.93883868111675671106e-04 +5.19430088755720511542e-05 +1.93883868111675671106e-04 -9.65935791364279539017e-01 +2.58781774227693439627e-01\n"
        "1.36000000000000000000e+09 -9.63435665732913504300e-01 +2.67939658903311672677e-01 +6.40709543671961394371e-05 +2.30381134422425382004e-04 +6.40709543671961394371e-05 +2.30381134422425382004e-04 -9.63435665732913504300e-01 +2.67939658903311728189e-01\n"
        "1.40500000000000000000e+09 -9.60832559316443512998e-01 +2.77129414977661359121e-01 +7.85365197085371324502e-05 +2.72293163962556209173e-04 +7.85365197085371324502e-05 +2.72293163962556209173e-04 -9.60832559316443512998e-01 +2.77129414977661359121e-01\n"
        "1.45000000000000000000e+09 -9.58124328649269108027e-01 +2.86352333924192958836e-01 +9.57062015038469146794e-05 +3.20229413906998031016e-04 +9.57062015038469146794e-05 +3.20229413906998031016e-04 -9.58124328649269108027e-01 +2.86352333924192958836e-01\n"
        "1.49500000000000000000e+09 -9.55308710220502232957e-01 +2.95609732942789416033e-01 +1.15992963796950055138e-04 +3.74849256607401738316e-04 +1.15992963796950055138e-04 +3.74849256607401738316e-04 -9.55308710220502121935e-01 +2.95609732942789416033e-01\n"
        "1.54000000000000000000e+09 -9.52383314048469875601e-01 +3.04902956217125431504e-01 +1.39861134972628256383e-04 +4.36864938550998878862e-04 +1.39861134972628256383e-04 +4.36864938550998878862e-04 -9.52383314048469875601e-01 +3.04902956217125431504e-01\n"
        "1.58500000000000000000e+09 -9.49345616733679342758e-01 +3.14233376213810311484e-01 +1.67831778986878379402e-04 +5.07044686498861561091e-04 +1.67831778986878379402e-04 +5.07044686498861561091e-04 -9.49345616733679342758e-01 +3.14233376213810255972e-01\n"
        "1.63000000000000000000e+09 -9.46192953942413650381e-01 +3.23602395023421252063e-01 +2.00488589742236187913e-04 +5.86215967116756535120e-04 +2.00488589742236187913e-04 +5.86215967116756535120e-04 -9.46192953942413650381e-01 +3.23602395023421252063e-01\n"
        "1.67500000000000000000e+09 -9.42922512267730161817e-01 +3.33011445742972866935e-01 +2.38484363004810688298e-04 +6.75268906146331390934e-04 +2.38484363004810688298e-04 +6.75268906146331390934e-04 -9.42922512267730161817e-01 +3.33011445742972866935e-01\n"
        "1.72000000000000000000e+09 -9.39531320408591552606e-01 +3.42461993898632133249e-01 +2.82548106537879734068e-04 +7.75159872756728848646e-04 +2.82548106537879734068e-04 +7.75159872756728848646e-04 -9.39531320408591552606e-01 +3.42461993898632188760e-01\n"
        "1.76500000000000000000e+09 -9.36016239601031974082e-01 +3.51955538906586373749e-01 +3.33492856199082510302e-04 +8.86915234131667451489e-04 +3.33492856199082510302e-04 +8.86915234131667451489e-04 -9.36016239601031863060e-01 +3.51955538906586373749e-01\n"
        "1.81000000000000000000e+09 -9.32373953227547569433e-01 +3.61493615568802439952e-01 +3.92224273725990107021e-04 +1.01163528454101584418e-03 +3.92224273725990107021e-04 +1.01163528454101584418e-03 -9.32373953227547569433e-01 +3.61493615568802495464e-01\n";

    QTemporaryFile tempFile("test_internal_XXXXXX.s2p");
    if (tempFile.open()) {
        QTextStream out(&tempFile);
        out << internalData.replace("\n", "\n"); // Ensure newlines are correct for file
        tempFile.close();
        qDebug() << "Temporary internal test file created at:" << tempFile.fileName();

        QMap<QString, QList<double>> data = readTouchstoneFile(tempFile.fileName());
        if (!data.isEmpty() && data.contains("frequency") && !data["frequency"].isEmpty() && data["frequency"].size() > 0) {
            displayData(data);
        } else {
            qWarning() << "Could not read or parse valid data from the internal test data.";
             QMessageBox::warning(this, tr("Internal Test Error"), tr("Could not read or parse valid data from the internal test data. Check logs."));
        }
    } else {
        qWarning() << "Could not create temporary file for internal test data.";
        QMessageBox::critical(this, tr("Internal Test Error"), tr("Could not create temporary file for internal test data."));
    }
}

// Slot to append messages to logOutputArea (can be used if message handler emits a signal)
void QucsTouchstoneViewer::handleLogMessage(const QString& message) {
    if (logOutputArea) {
        logOutputArea->append(message);
    }
}


// Convert S-parameter from Magnitude/Angle (MA), Real/Imaginary (RI) or dB/Angle (DB) to dB/Angle and Real/Imaginary
void QucsTouchstoneViewer::convert_MA_RI_to_dB(double *S_val1, double *S_val2, double *S_re_out, double *S_im_out, QString format)
{
    double input_val1 = *S_val1;
    double input_val2 = *S_val2;
    double s_db, s_ang_deg, s_re, s_im;

    format = format.toUpper();

    if (format == "MA") { // Magnitude (linear) and Angle (degrees)
        if (input_val1 < 0) {
            qWarning() << "Magnitude (MA) is negative:" << input_val1 << ". Using abs().";
            input_val1 = std::abs(input_val1);
        }
        s_db = 20.0 * log10(input_val1 > std::numeric_limits<double>::epsilon() ? input_val1 : std::numeric_limits<double>::min());
        s_ang_deg = input_val2;
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = input_val1 * std::cos(ang_rad);
        s_im = input_val1 * std::sin(ang_rad);
    } else if (format == "RI") { // Real and Imaginary
        s_re = input_val1;
        s_im = input_val2;
        double mag = std::sqrt(s_re * s_re + s_im * s_im);
        s_db = 20.0 * log10(mag > std::numeric_limits<double>::epsilon() ? mag : std::numeric_limits<double>::min());
        s_ang_deg = qRadiansToDegrees(std::atan2(s_im, s_re));
    } else if (format == "DB") { // dB and Angle (degrees)
        s_db = input_val1;
        s_ang_deg = input_val2;
        double mag_lin = std::pow(10.0, s_db / 20.0);
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = mag_lin * std::cos(ang_rad);
        s_im = mag_lin * std::sin(ang_rad);
    } else { // Default or unknown format, assume MA
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
        qWarning() << "Error: Cannot open the file:" << filePath; // Changed to qWarning for handler
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
        int n = suffix.mid(1, suffix.length() - 2).toInt(&ok);
        if (ok && n > 0) {
            number_of_ports = n;
            qDebug() << "Number of ports from extension:" << number_of_ports;
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
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts); // Escaped \s

            if (parts.length() > 1) frequency_unit_str = parts[1].toLower();
            if (parts.length() > 2) parameter_str = parts[2].toLower();
            if (parts.length() > 3) format_str = parts[3].toLower();

            bool z0_found_keyword = false;
            for(int k=4; k < parts.length(); ++k) {
                if(parts[k].toLower() == "r" || parts[k].toLower() == "z0") {
                    if (k+1 < parts.length()) {
                        bool ok;
                        double parsed_Z0 = parts[k+1].toDouble(&ok);
                        if(ok) {
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
                if (potential_z0_str != "r" && potential_z0_str != "z0") {
                    bool ok;
                    double z_check = parts[4].toDouble(&ok);
                    if (ok) Z0 = z_check;
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

        QStringList values = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts); // Escaped \s
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
                qWarning() << "Error: Could not determine number of ports for file:" << filePath << ". File extension was not sNp or first data line malformed. Line:" << line;
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

        for (int i = 1; i <= number_of_ports; ++i) {
            for (int j = 1; j <= number_of_ports; ++j) {
                QString s_param_mag_key = QString("S%1%2_dB").arg(i).arg(j);
                QString s_param_ang_key = QString("S%1%2_ang").arg(i).arg(j);
                QString s_param_re_key = QString("S%1%2_re").arg(i).arg(j);
                QString s_param_im_key = QString("S%1%2_im").arg(i).arg(j);

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
                        double s_re_val, s_im_val; // Renamed to avoid conflict with s_re, s_im in outer scope
                        convert_MA_RI_to_dB(&val1, &val2, &s_re_val, &s_im_val, format_str);
                        file_data[s_param_mag_key].append(val1);
                        file_data[s_param_ang_key].append(val2);
                        file_data[s_param_re_key].append(s_re_val);
                        file_data[s_param_im_key].append(s_im_val);
                        qDebug() << QString("S%1%2: dB=").arg(i).arg(j) << val1 << "Ang=" << val2 << "Re=" << s_re_val << "Im=" << s_im_val;
                    }
                    current_val_idx += 2;
                } else {
                    qDebug() << "Incomplete data on line for S" << i << j << " - Appending NaN. Line:" << line;
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
        if (data.isEmpty()) {
        } else {
             if (!(data.contains("frequency") && !data["frequency"].isEmpty())) {
                 QMessageBox::information(this, tr("Info"), tr("File parsed, but no valid frequency data points found."));
            } else {
                 QMessageBox::information(this, tr("Info"), tr("No frequency data points found in the file."));
            }
        }
        return;
    }

    const QList<double>& freq = data["frequency"];
    int numRowsToShow = qMin(10, freq.size());
    dataTable->setRowCount(numRowsToShow);
    qDebug() << "Displaying" << numRowsToShow << "rows.";

    QStringList sParamIndices = {"11", "12", "21", "22"};

    for (int i = 0; i < numRowsToShow; ++i) {
        dataTable->setItem(i, 0, new QTableWidgetItem(QString::number(freq.at(i), 'g', 10)));

        for (int j = 0; j < sParamIndices.size(); ++j) {
            QString s_param_key_db = QString("S%1_dB").arg(sParamIndices.at(j));
            if (data.contains(s_param_key_db) && i < data[s_param_key_db].size()) {
                double val = data[s_param_key_db].at(i);
                dataTable->setItem(i, j + 1, new QTableWidgetItem(std::isnan(val) ? "NaN" : QString::number(val, 'f', 4)));
            } else {
                dataTable->setItem(i, j + 1, new QTableWidgetItem("N/A"));
            }
        }

        if (data.contains("Z0") && i < data["Z0"].size()) {
             dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data["Z0"].at(i), 'f', 2)));
        } else if (data.contains("Z0") && !data["Z0"].isEmpty()){
            dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data["Z0"].first(), 'f', 2)));
        } else {
            dataTable->setItem(i, 5, new QTableWidgetItem("50.00 (default)"));
        }
    }
}
