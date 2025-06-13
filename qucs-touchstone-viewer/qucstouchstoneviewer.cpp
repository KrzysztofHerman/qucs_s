#include "qucstouchstoneviewer.h"
#include <QHeaderView> // Required for QHeaderView
#include <QtMath> // For qDegreesToRadians and qRadiansToDegrees if needed, M_PI is in cmath
#include <QTemporaryFile> // Required for QTemporaryFile
#include <QLabel> // Required for QLabel (used in createWidgets)
#include <QClipboard> // For accessing the system clipboard
#include <QApplication> // Required for QApplication::clipboard()
#include <QMessageBox>
#include <QLineEdit>
#include <QDoubleValidator>

#include <random>
#include <vector>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <limits>
#include <complex>

QTextEdit* QucsTouchstoneViewer::S_logOutputArea = nullptr;

QucsTouchstoneViewer::QucsTouchstoneViewer(QWidget *parent)
    : QMainWindow(parent)
{
    createWidgets();
    setWindowTitle(tr("Qucs Touchstone Viewer"));
    setMinimumSize(800, 600);

    m_isTargetFrequencyApplied = false;
    m_analysisResultsAvailable = false;
    m_analyzedNumPorts = 0;
    m_actual_ftarget_hz_calc = 0.0;
    m_Z0_calc = 50.0;
    m_y11_calc = m_y12_calc = m_y21_calc = m_y22_calc = std::complex<double>(0.0, 0.0);

    // Initialize new low frequency members
    m_lowFreqAnalysisResultsAvailable = false;
    m_actual_flow_hz_calc = 0.0;
    m_Z0_calc_low = 50.0;
    m_y11_calc_low = m_y12_calc_low = m_y21_calc_low = m_y22_calc_low = std::complex<double>(0.0, 0.0);

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

    // File operations (Row 0)
    openButton = new QPushButton(tr("Select Touchstone File"), this);
    connect(openButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::openFile);
    loadInternalDataButton = new QPushButton(tr("Load Internal Test Data"), this);
    connect(loadInternalDataButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::loadInternalTestData);
    controlsLayout->addWidget(openButton, 0, 0);
    controlsLayout->addWidget(loadInternalDataButton, 0, 1);

    // Network Synthesis controls (Row 1)
    QLabel *networkTypeLabel = new QLabel(tr("Network Type:"), this);
    networkTypeComboBox = new QComboBox(this);
    networkTypeComboBox->addItem(tr("Inductor-pi"));
    networkTypeComboBox->addItem(tr("MiM-capacitor-pi"));
    synthesizeButton = new QPushButton(tr("Synthesize"), this);
    connect(synthesizeButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::onSynthesizeClicked);
    controlsLayout->addWidget(networkTypeLabel, 1, 0);
    controlsLayout->addWidget(networkTypeComboBox, 1, 1);
    controlsLayout->addWidget(synthesizeButton, 1, 2);

    // Target Frequency controls (Row 2)
    QLabel *targetFreqLabel = new QLabel(tr("Target Frequency:"), this);
    targetFrequencyInput = new QLineEdit("1", this);
    QDoubleValidator *freqValidator = new QDoubleValidator(this);
    freqValidator->setNotation(QDoubleValidator::StandardNotation);
    freqValidator->setBottom(0);
    targetFrequencyInput->setValidator(freqValidator);
    targetFrequencyInput->setFixedWidth(100);

    targetFrequencyUnitComboBox = new QComboBox(this);
    targetFrequencyUnitComboBox->addItem("GHz");
    targetFrequencyUnitComboBox->addItem("MHz");
    targetFrequencyUnitComboBox->addItem("kHz");
    targetFrequencyUnitComboBox->addItem("Hz");
    targetFrequencyUnitComboBox->setCurrentText("GHz");

    analyzeFrequencyButton = new QPushButton(tr("Analyze Frequency"), this);
    connect(analyzeFrequencyButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::onAnalyzeFrequencyClicked);

    QHBoxLayout *targetFreqLayout = new QHBoxLayout();
    targetFreqLayout->addWidget(targetFrequencyInput);
    targetFreqLayout->addWidget(targetFrequencyUnitComboBox);

    controlsLayout->addWidget(targetFreqLabel, 2, 0);
    controlsLayout->addLayout(targetFreqLayout, 2, 1);
    controlsLayout->addWidget(analyzeFrequencyButton, 2, 2);

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
        m_fullTouchstoneData = readTouchstoneFile(filePath);

        m_analysisResultsAvailable = false;
        m_analyzedNumPorts = 0;
        m_lowFreqAnalysisResultsAvailable = false;

        if (!m_fullTouchstoneData.isEmpty() && m_fullTouchstoneData.contains("frequency") && !m_fullTouchstoneData["frequency"].isEmpty()) {
            m_isTargetFrequencyApplied = false;
            displayData();
            logOutputArea->append(QString("File loaded: %1. Displaying initial data.").arg(QFileInfo(filePath).fileName()));
        } else {
            qWarning() << "readTouchstoneFile returned empty or invalid data for:" << filePath;
            m_fullTouchstoneData.clear();
            m_isTargetFrequencyApplied = false;
            dataTable->setRowCount(0);
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

        m_fullTouchstoneData = readTouchstoneFile(tempFile.fileName());

        m_analysisResultsAvailable = false;
        m_analyzedNumPorts = 0;
        m_lowFreqAnalysisResultsAvailable = false;

        if (!m_fullTouchstoneData.isEmpty() && m_fullTouchstoneData.contains("frequency") && !m_fullTouchstoneData["frequency"].isEmpty()) {
            m_isTargetFrequencyApplied = false;
            displayData();
        } else {
            qWarning() << "Could not read or parse valid data from the internal S1P test data.";
            m_fullTouchstoneData.clear();
            m_isTargetFrequencyApplied = false;
            dataTable->setRowCount(0);
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
        logOutputArea->append("\n--- Synthesizing Inductor-pi Network ---");

        QString rShunt1Val, cShunt1Val, lSeriesVal, rSeriesVal, cShunt2Val, rShunt2Val;
        bool usedCalculatedValues = false;

        if (m_analysisResultsAvailable && m_analyzedNumPorts == 2) {
            logOutputArea->append(QString("Attempting to use calculated values based on analysis at %1 Hz, Z0 = %2 Ohms.")
                .arg(QString::number(m_actual_ftarget_hz_calc, 'g', 10))
                .arg(QString::number(m_Z0_calc, 'f', 2)));

            logOutputArea->append(QString("Using Y-parameters: Y11=%1; Y12=%2; Y21=%3; Y22=%4")
                .arg(formatComplex(m_y11_calc))
                .arg(formatComplex(m_y12_calc))
                .arg(formatComplex(m_y21_calc))
                .arg(formatComplex(m_y22_calc)));

            bool calculationError = false;

            if (m_actual_ftarget_hz_calc <= 1e-9) {
                logOutputArea->append("Warning: Target frequency for Inductor-pi calculation is zero or too small. Falling back to random values.");
                calculationError = true;
            }

            double omega_target = 0.0;
            if (!calculationError) {
                omega_target = 2.0 * M_PI * m_actual_ftarget_hz_calc;
                if (omega_target == 0.0) {
                    logOutputArea->append("Error: omega_target is zero. Cannot calculate L/C. Falling back to random values.");
                    calculationError = true;
                }
            }

            double Rseries_raw = 0.0, Lseries_raw = 0.0, Cshunt1_raw = 0.0, Cshunt2_raw = 0.0, Rshunt1_raw = 0.0, Rshunt2_raw = 0.0;

            if (!calculationError) {
                std::complex<double> ymn = (m_y12_calc + m_y21_calc) / 2.0;
                std::complex<double> y11_plus_ymn = m_y11_calc + ymn;
                std::complex<double> y22_plus_ymn = m_y22_calc + ymn;
                std::complex<double> Zshunt1_complex, Zshunt2_complex, Zseries_complex;

                if (std::abs(y11_plus_ymn) < 1e-12) { logOutputArea->append("Error: Denominator (y11 + ymn) for Zshunt1 is near zero."); calculationError = true; }
                if (!calculationError) Zshunt1_complex = 1.0 / y11_plus_ymn;

                if (std::abs(y22_plus_ymn) < 1e-12) { logOutputArea->append("Error: Denominator (y22 + ymn) for Zshunt2 is near zero."); calculationError = true; }
                if (!calculationError) Zshunt2_complex = 1.0 / y22_plus_ymn;

                if (std::abs(ymn) < 1e-12) { logOutputArea->append("Error: Denominator (ymn) for Zseries is near zero."); calculationError = true; }
                if (!calculationError) Zseries_complex = -1.0 / ymn;

                if (!calculationError) {
                    Rseries_raw = Zseries_complex.real();
                    Lseries_raw = (omega_target != 0.0) ? (Zseries_complex.imag() / omega_target) : std::numeric_limits<double>::infinity();

                    double Cs1_raw_intermediate = (std::abs(Zshunt1_complex.imag()) > 1e-18 && omega_target != 0.0) ? (-1.0 / (omega_target * Zshunt1_complex.imag())) : std::numeric_limits<double>::infinity();
                    double Cs2_raw_intermediate = (std::abs(Zshunt2_complex.imag()) > 1e-18 && omega_target != 0.0) ? (-1.0 / (omega_target * Zshunt2_complex.imag())) : std::numeric_limits<double>::infinity();

                    logOutputArea->append(QString("Intermediate calculated: Cs1_raw_intermediate=%1 F, Cs2_raw_intermediate=%2 F").arg(Cs1_raw_intermediate).arg(Cs2_raw_intermediate));

                    if (Cs1_raw_intermediate <= 1e-18 || std::isinf(Cs1_raw_intermediate) || std::isnan(Cs1_raw_intermediate)) {
                        logOutputArea->append(QString("Warning: Calculated Cs1_raw_intermediate is non-positive or invalid (%1 F).").arg(Cs1_raw_intermediate));
                        calculationError = true;
                    }
                    if (Cs2_raw_intermediate <= 1e-18 || std::isinf(Cs2_raw_intermediate) || std::isnan(Cs2_raw_intermediate)) {
                        logOutputArea->append(QString("Warning: Calculated Cs2_raw_intermediate is non-positive or invalid (%1 F).").arg(Cs2_raw_intermediate));
                        calculationError = true;
                    }

                    if (!calculationError) {
                        Cshunt1_raw = (Cs1_raw_intermediate + Cs2_raw_intermediate) / 2.0;
                        Cshunt2_raw = Cshunt1_raw;
                        if (Cshunt1_raw <= 1e-18) {
                             logOutputArea->append(QString("Warning: Averaged Cshunt value is non-positive or extremely small (%1 F).").arg(Cshunt1_raw));
                             calculationError = true;
                        }
                    }

                    Rshunt1_raw = Zshunt1_complex.real();
                    Rshunt2_raw = Zshunt2_complex.real();

                    logOutputArea->append(QString("Raw calculated before final validation: Rser=%1, Lser=%2, Csh1=%3, Csh2=%4, Rsh1=%5, Rsh2=%6")
                        .arg(Rseries_raw).arg(Lseries_raw).arg(Cshunt1_raw).arg(Cshunt2_raw).arg(Rshunt1_raw).arg(Rshunt2_raw));

                    if (Lseries_raw <= 1e-15) { logOutputArea->append(QString("Warning: Calculated Lseries is non-positive or extremely small (%1 H).").arg(Lseries_raw)); calculationError = true; }

                    if (Rseries_raw < 0) { logOutputArea->append(QString("Warning: Calculated Rseries is negative (%1 Ohm). Using abs value.").arg(Rseries_raw)); Rseries_raw = std::abs(Rseries_raw); }
                    if (Rshunt1_raw < 0) { logOutputArea->append(QString("Warning: Calculated Rshunt1 is negative (%1 Ohm). Using abs value.").arg(Rshunt1_raw)); Rshunt1_raw = std::abs(Rshunt1_raw); }
                    if (Rshunt2_raw < 0) { logOutputArea->append(QString("Warning: Calculated Rshunt2 is negative (%1 Ohm). Using abs value.").arg(Rshunt2_raw)); Rshunt2_raw = std::abs(Rshunt2_raw); }

                    if (!calculationError) {
                        rShunt1Val = formatComponentValue(Rshunt1_raw, "R");
                        cShunt1Val = formatComponentValue(Cshunt1_raw, "C");
                        lSeriesVal = formatComponentValue(Lseries_raw, "L");
                        rSeriesVal = formatComponentValue(Rseries_raw, "R");
                        cShunt2Val = formatComponentValue(Cshunt2_raw, "C");
                        rShunt2Val = formatComponentValue(Rshunt2_raw, "R");
                        usedCalculatedValues = true;
                        logOutputArea->append("Successfully used calculated component values for Inductor-pi.");
                    }
                }
            }
            if (calculationError) {
                 logOutputArea->append("Fallback to random values for Inductor-pi due to calculation errors or non-physical results.");
            }
        }

        if (!usedCalculatedValues) {
            logOutputArea->append("Using random values for Inductor-pi components (either no analysis data, not 2-port, or calculation failed).");
            rShunt1Val = generateFormattedRandomValue(1.0, 100e3, "R");
            cShunt1Val = generateFormattedRandomValue(1e-12, 10e-6, "C");
            lSeriesVal = generateFormattedRandomValue(1e-9, 100e-3, "L");
            rSeriesVal = generateFormattedRandomValue(1.0, 100e3, "R");
            cShunt2Val = generateFormattedRandomValue(1e-12, 10e-6, "C");
            rShunt2Val = generateFormattedRandomValue(1.0, 100e3, "R");
        }

        logOutputArea->append(QString("Final Inductor-pi values to be used in XML:"));
        logOutputArea->append(QString("  Rshunt1 (R1): %1").arg(rShunt1Val));
        logOutputArea->append(QString("  Cshunt1 (C1): %1").arg(cShunt1Val));
        logOutputArea->append(QString("  Lseries (L1): %1").arg(lSeriesVal));
        logOutputArea->append(QString("  Rseries (R3): %1").arg(rSeriesVal));
        logOutputArea->append(QString("  Cshunt2 (C2): %1").arg(cShunt2Val));
        logOutputArea->append(QString("  Rshunt2 (R2): %1").arg(rShunt2Val));

        QString schematicXml = QString(
            "<Qucs Schematic 25.1.2>\n"
            "<Components>\n"
            "<R R1 1 280 660 15 -26 0 1 \"%1\" 1 \"%1\" 0 \"0.0\" 0 \"0.0\" 0 \"0.0\" 0 \"european\" 0>\n"
            "<C C1 1 280 580 17 -26 0 1 \"%2\" 1 \"%2\" 0 \"neutral\" 0>\n"
            "<Port P1 1 560 510 4 -40 0 2 \"2\" 0 \"analog\" 0 \"v\" 0 \"\" 0>\n"
            "<Port P2 1 240 510 -23 -40 1 0 \"1\" 0 \"analog\" 0 \"v\" 0 \"\" 0>\n"
            "<L L1 1 360 510 -26 10 0 0 \"%3\" 1 \"%3\" 0>\n"
            "<R R3 1 460 510 -26 15 0 0 \"%4\" 1 \"%4\" 0 \"0.0\" 0 \"0.0\" 0 \"0.0\" 0 \"european\" 0>\n"
            "<C C2 1 540 580 17 -26 0 1 \"%5\" 1 \"%5\" 0 \"neutral\" 0>\n"
            "<R R2 1 540 660 15 -26 0 1 \"%6\" 1 \"%6\" 0 \"0.0\" 0 \"0.0\" 0 \"0.0\" 0 \"european\" 0>\n"
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
            "<Diagrams>\n</Diagrams>\n"
            "<Paintings>\n</Paintings>\n"
        ).arg(rShunt1Val)
         .arg(cShunt1Val)
         .arg(lSeriesVal)
         .arg(rSeriesVal)
         .arg(cShunt2Val)
         .arg(rShunt2Val);

        QClipboard *clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(schematicXml);
            logOutputArea->append("Inductor-pi schematic XML copied to clipboard.");
            QMessageBox::information(this, tr("Synthesize Inductor-pi"), tr("Schematic XML for Inductor-pi network has been generated and copied to clipboard."));
        } else {
            logOutputArea->append("Error: Could not access clipboard.");
            QMessageBox::warning(this, tr("Synthesize Inductor-pi"), tr("Error: Could not access system clipboard."));
        }

    } else if (selectedNetwork == tr("MiM-capacitor-pi")) {
        logOutputArea->append("\n--- Synthesizing MiM-capacitor-pi Network ---");
        QString cShunt1Val, lSeriesVal, rSeriesVal, cShunt2Val, cMimVal;
        bool usedCalculatedMiMValues = false;

        if (m_analysisResultsAvailable && m_lowFreqAnalysisResultsAvailable && m_analyzedNumPorts == 2) {
            logOutputArea->append(QString("Attempting to use calculated values based on analysis at:"));
            logOutputArea->append(QString("  f_target (primary): %1 Hz (Z0=%2 Ohm)")
                .arg(QString::number(m_actual_ftarget_hz_calc, 'g', 10))
                .arg(QString::number(m_Z0_calc, 'f', 2)));
            logOutputArea->append(QString("  f_low (secondary): %1 Hz (Z0=%2 Ohm)")
                .arg(QString::number(m_actual_flow_hz_calc, 'g', 10))
                .arg(QString::number(m_Z0_calc_low, 'f', 2)));

            logOutputArea->append(QString("Using Y-params at f_target: Y11=%1; Y12=%2; Y21=%3; Y22=%4")
                .arg(formatComplex(m_y11_calc)).arg(formatComplex(m_y12_calc))
                .arg(formatComplex(m_y21_calc)).arg(formatComplex(m_y22_calc)));
            logOutputArea->append(QString("Using Y-params at f_low: Y11=%1; Y12=%2; Y21=%3; Y22=%4")
                .arg(formatComplex(m_y11_calc_low)).arg(formatComplex(m_y12_calc_low))
                .arg(formatComplex(m_y21_calc_low)).arg(formatComplex(m_y22_calc_low)));

            bool calculationError = false;
            double Cser_low_raw = 0.0;

            if (m_actual_flow_hz_calc <= 1e-9) {
                logOutputArea->append("Error: Low frequency (flow) for calculation is zero or too small. Cannot proceed.");
                calculationError = true;
            } else {
                double omegal = 2.0 * M_PI * m_actual_flow_hz_calc;
                std::complex<double> ymn_low = (m_y12_calc_low + m_y21_calc_low) / 2.0;

                if (std::abs(ymn_low) < 1e-12) {
                    logOutputArea->append("Error (low freq): Denominator (ymn_low) for Zseries_low_complex is near zero.");
                    calculationError = true;
                } else {
                    std::complex<double> Zseries_low_complex = -1.0 / ymn_low;
                    if (Zseries_low_complex.imag() >= -1e-18 || omegal == 0.0) {
                        logOutputArea->append(QString("Error (low freq): -Zseries_low.imag (%1) is not sufficiently negative or omega is zero for Cser_low calc.")
                                              .arg(Zseries_low_complex.imag()));
                        calculationError = true;
                    } else {
                        Cser_low_raw = 1.0 / (-Zseries_low_complex.imag() * omegal);
                        logOutputArea->append(QString("Intermediate Cser_low (raw): %1 F").arg(Cser_low_raw));
                        if (Cser_low_raw <= 1e-18) {
                            logOutputArea->append(QString("Warning (low freq): Calculated Cser_low is non-positive or extremely small (%1 F).").arg(Cser_low_raw));
                            calculationError = true;
                        }
                    }
                }
            }

            double Rseries_raw = 0.0, Lseries_raw = 0.0, Cmim_raw = 0.0, Cshunt1_raw_calc = 0.0, Cshunt2_raw_calc = 0.0;

            if (!calculationError && m_actual_ftarget_hz_calc <= 1e-9) {
                 logOutputArea->append("Error: Target frequency for calculation is zero or too small. Cannot reliably calculate L/C values.");
                 calculationError = true;
            }

            if (!calculationError) {
                double omegat = 2.0 * M_PI * m_actual_ftarget_hz_calc;
                std::complex<double> ymn_tgt = (m_y12_calc + m_y21_calc) / 2.0;
                std::complex<double> Zshunt1_tgt_complex, Zshunt2_tgt_complex, Zseries_tgt_complex;

                if (std::abs(m_y11_calc + ymn_tgt) < 1e-12) { logOutputArea->append("Error (target freq): Denominator (y11_tgt + ymn_tgt) for Zshunt1 is near zero."); calculationError = true; }
                if (!calculationError) Zshunt1_tgt_complex = 1.0 / (m_y11_calc + ymn_tgt);

                if (std::abs(m_y22_calc + ymn_tgt) < 1e-12) { logOutputArea->append("Error (target freq): Denominator (y22_tgt + ymn_tgt) for Zshunt2 is near zero."); calculationError = true; }
                if (!calculationError) Zshunt2_tgt_complex = 1.0 / (m_y22_calc + ymn_tgt);

                if (std::abs(ymn_tgt) < 1e-12) { logOutputArea->append("Error (target freq): Denominator (ymn_tgt) for Zseries is near zero."); calculationError = true; }
                if (!calculationError) Zseries_tgt_complex = -1.0 / ymn_tgt;

                if (!calculationError) {
                    Rseries_raw = Zseries_tgt_complex.real();
                    if (omegat == 0.0 || Cser_low_raw <= 1e-18) {
                        logOutputArea->append("Error (target freq): omegat or Cser_low is zero/invalid for Lseries calculation.");
                        calculationError = true;
                    } else {
                        Lseries_raw = (Zseries_tgt_complex.imag() + 1.0 / (omegat * Cser_low_raw)) / omegat;
                        logOutputArea->append(QString("Intermediate Lseries (raw): %1 H").arg(Lseries_raw));
                        if (Lseries_raw <= 1e-15) { logOutputArea->append(QString("Warning (target freq): Calculated Lseries is non-positive or extremely small (%1 H).").arg(Lseries_raw)); calculationError = true; }
                    }
                }

                if (!calculationError) {
                     if (omegat == 0.0 || Cser_low_raw <= 1e-18) {
                        logOutputArea->append("Error (target freq): omegat or Cser_low is zero/invalid for X_intermediate calculation.");
                        calculationError = true;
                     } else {
                        double X_intermediate = omegat * Lseries_raw - 1.0 / (omegat * Cser_low_raw);
                        logOutputArea->append(QString("Intermediate X: %1").arg(X_intermediate));
                        if (std::abs(X_intermediate) < 1e-18 || omegat == 0.0) {
                            logOutputArea->append("Error (target freq): Denominator (-X_intermediate * omegat) for Cmim is near zero.");
                            calculationError = true;
                        } else {
                            Cmim_raw = 1.0 / (-X_intermediate * omegat);
                            logOutputArea->append(QString("Intermediate Cmim (raw): %1 F").arg(Cmim_raw));
                            if (Cmim_raw <= 1e-18) { logOutputArea->append(QString("Warning (target freq): Calculated Cmim is non-positive or extremely small (%1 F).").arg(Cmim_raw)); calculationError = true;}
                        }
                     }
                }

                if (!calculationError) {
                    if (std::abs(Zshunt1_tgt_complex.imag()) < 1e-18 || omegat == 0.0) { logOutputArea->append("Error (target freq): Denom for Cshunt1 is near zero or imag part is zero."); calculationError = true;}
                    else { Cshunt1_raw_calc = -1.0 / (omegat * Zshunt1_tgt_complex.imag()); if (Cshunt1_raw_calc <= 1e-18) {logOutputArea->append(QString("Warning (target freq): Cshunt1 non-positive/small (%1F).").arg(Cshunt1_raw_calc)); calculationError=true;}}

                    if (std::abs(Zshunt2_tgt_complex.imag()) < 1e-18 || omegat == 0.0) { logOutputArea->append("Error (target freq): Denom for Cshunt2 is near zero or imag part is zero."); calculationError = true;}
                    else { Cshunt2_raw_calc = -1.0 / (omegat * Zshunt2_tgt_complex.imag()); if (Cshunt2_raw_calc <= 1e-18) {logOutputArea->append(QString("Warning (target freq): Cshunt2 non-positive/small (%1F).").arg(Cshunt2_raw_calc)); calculationError=true;}}
                }

                if (!calculationError) {
                     if (Rseries_raw < 0) {logOutputArea->append(QString("Warning: Calculated Rseries is negative (%1 Ohm). Using abs value.").arg(Rseries_raw)); Rseries_raw = std::abs(Rseries_raw);}

                    cShunt1Val = formatComponentValue(Cshunt1_raw_calc, "C");
                    lSeriesVal = formatComponentValue(Lseries_raw, "L");
                    rSeriesVal = formatComponentValue(Rseries_raw, "R");
                    cShunt2Val = formatComponentValue(Cshunt2_raw_calc, "C");
                    cMimVal    = formatComponentValue(Cmim_raw, "C");
                    usedCalculatedMiMValues = true;
                    logOutputArea->append("Successfully used calculated component values for MiM-capacitor-pi.");
                }
            }
            if (calculationError) {
                 logOutputArea->append("Fallback to random values for MiM-capacitor-pi due to calculation errors or non-physical results.");
            }
        } else {
            logOutputArea->append("Analysis results for both frequencies not available or not 2-port. Using random values for MiM-capacitor-pi.");
        }

        if (!usedCalculatedMiMValues) {
            logOutputArea->append("Generating random values for MiM-capacitor-pi components.");
            cShunt1Val = generateFormattedRandomValue(1e-12, 10e-6, "C");
            lSeriesVal = generateFormattedRandomValue(1e-9, 100e-3, "L");
            rSeriesVal = generateFormattedRandomValue(1.0, 100e3, "R");
            cShunt2Val = generateFormattedRandomValue(1e-12, 10e-6, "C");
            cMimVal    = generateFormattedRandomValue(1e-12, 10e-6, "C");
        }

        logOutputArea->append(QString("Final MiM-capacitor-pi values to be used in XML:"));
        logOutputArea->append(QString("  Cshunt1 (C1): %1").arg(cShunt1Val));
        logOutputArea->append(QString("  Lseries (L1): %1").arg(lSeriesVal));
        logOutputArea->append(QString("  Rseries (R2): %1").arg(rSeriesVal));
        logOutputArea->append(QString("  Cshunt2 (C2): %1").arg(cShunt2Val));
        logOutputArea->append(QString("  Cmim    (C3): %1").arg(cMimVal));

        QString schematicXml = QString(
            "<Qucs Schematic 25.1.2>\n"
            "<Components>\n"
            "<C C1 1 220 230 17 -26 0 1 \"%1\" 1 \"%1\" 0 \"neutral\" 0>\n"
            "<Port P2 1 180 160 -23 -40 1 0 \"1\" 0 \"analog\" 0 \"v\" 0 \"\" 0>\n"
            "<GND * 1 220 280 0 0 0 0>\n"
            "<L L1 1 360 160 -26 10 0 0 \"%2\" 1 \"%2\" 0>\n"
            "<Port P1 1 540 160 4 -40 0 2 \"2\" 0 \"analog\" 0 \"v\" 0 \"\" 0>\n"
            "<R R2 1 440 160 -26 15 0 0 \"%3\" 1 \"%3\" 0 \"0.0\" 0 \"0.0\" 0 \"0.0\" 0 \"european\" 0>\n"
            "<C C2 1 520 230 17 -26 0 1 \"%4\" 1 \"%4\" 0 \"neutral\" 0>\n"
            "<GND * 1 520 280 0 0 0 0>\n"
            "<C C3 1 290 160 -26 17 0 0 \"%5\" 1 \"%5\" 0 \"neutral\" 0>\n"
            "</Components>\n"
            "<Wires>\n"
            "<180 160 220 160 \"\" 0 0 0 \"\">\n"
            "<220 160 220 200 \"\" 0 0 0 \"\">\n"
            "<220 260 220 280 \"\" 0 0 0 \"\">\n"
            "<390 160 410 160 \"\" 0 0 0 \"\">\n"
            "<520 160 540 160 \"\" 0 0 0 \"\">\n"
            "<520 160 520 200 \"\" 0 0 0 \"\">\n"
            "<470 160 520 160 \"\" 0 0 0 \"\">\n"
            "<520 260 520 280 \"\" 0 0 0 \"\">\n"
            "<320 160 330 160 \"\" 0 0 0 \"\">\n"
            "<220 160 260 160 \"\" 0 0 0 \"\">\n"
            "</Wires>\n"
            "<Diagrams>\n</Diagrams>\n"
            "<Paintings>\n</Paintings>\n"
        ).arg(cShunt1Val)
         .arg(lSeriesVal)
         .arg(rSeriesVal)
         .arg(cShunt2Val)
         .arg(cMimVal);

        QClipboard *clipboard = QApplication::clipboard();
        if (clipboard) {
            clipboard->setText(schematicXml);
            logOutputArea->append("MiM-capacitor-pi schematic XML copied to clipboard.");
            QMessageBox::information(this, tr("Synthesize MiM-capacitor-pi"), tr("Schematic XML for MiM-capacitor-pi network has been generated and copied to clipboard."));
        } else {
            logOutputArea->append("Error: Could not access clipboard.");
            QMessageBox::warning(this, tr("Synthesize MiM-capacitor-pi"), tr("Error: Could not access system clipboard."));
        }

    } else {
        QMessageBox::warning(this, tr("Synthesize"), QString(tr("Synthesis for '%1' is not implemented yet.")).arg(selectedNetwork));
    }
}

void QucsTouchstoneViewer::onAnalyzeFrequencyClicked()
{
    dataTable->clearContents();
    dataTable->setRowCount(0);
    logOutputArea->clear();

    QString targetFreqStr = targetFrequencyInput->text();
    QString targetUnitStr = targetFrequencyUnitComboBox->currentText();
    logOutputArea->append(QString("Analyze Frequency input: Target: %1 %2").arg(targetFreqStr).arg(targetUnitStr));
    qDebug() << "Analyze Frequency button clicked. Target Freq:" << targetFreqStr << targetUnitStr;

    bool conversionOk;
    double targetFreqValue = targetFreqStr.toDouble(&conversionOk);

    m_analysisResultsAvailable = false;
    m_lowFreqAnalysisResultsAvailable = false;

    if (!conversionOk) {
        qWarning() << "Invalid target frequency input:" << targetFreqStr;
        QMessageBox::warning(this, tr("Invalid Input"), tr("Target frequency is not a valid number."));
        logOutputArea->append("Error: Invalid target frequency input.");
        return;
    }
    if (m_fullTouchstoneData.isEmpty() || !m_fullTouchstoneData.contains("frequency") || m_fullTouchstoneData["frequency"].isEmpty()) {
        qWarning() << "No data loaded to analyze.";
        QMessageBox::information(this, tr("No Data"), tr("Please load a Touchstone file first."));
        logOutputArea->append("Info: No data loaded to analyze.");
        return;
    }

    double f_target1_GHz = targetFreqValue;
    if (targetUnitStr == "MHz") f_target1_GHz *= 1e-3;
    else if (targetUnitStr == "kHz") f_target1_GHz *= 1e-6;
    else if (targetUnitStr == "Hz") f_target1_GHz *= 1e-9;
    logOutputArea->append(QString("Scaled Primary Target Frequency (f_target1): %1 GHz").arg(QString::number(f_target1_GHz, 'g', 10)));

    double f_target2_GHz = 0.05 * f_target1_GHz;
    logOutputArea->append(QString("Scaled Secondary Target Frequency (f_target2 = 0.05 * f_target1): %1 GHz").arg(QString::number(f_target2_GHz, 'g', 10)));

    const QList<double>& frequencies = m_fullTouchstoneData["frequency"];
    if (frequencies.isEmpty()) {
        qWarning() << "Frequency data list is present but empty.";
        logOutputArea->append("Error: Frequency data list is empty in loaded file.");
        return;
    }

    auto findClosestFreqIndex = [&](double targetFreq) -> int {
        int localClosestIndex = -1;
        double localMinDiff = std::numeric_limits<double>::max();
        for (int i = 0; i < frequencies.size(); ++i) {
            double diff = std::abs(frequencies.at(i) - targetFreq);
            if (diff < localMinDiff) {
                localMinDiff = diff;
                localClosestIndex = i;
            }
        }
        return localClosestIndex;
    };

    int closestIndex1 = findClosestFreqIndex(f_target1_GHz);
    int closestIndex2 = findClosestFreqIndex(f_target2_GHz);

    int numPortsInFile = 0;
    if (m_fullTouchstoneData.contains("n_ports") && !m_fullTouchstoneData["n_ports"].isEmpty()) {
        numPortsInFile = static_cast<int>(m_fullTouchstoneData["n_ports"].first());
    }
    m_analyzedNumPorts = numPortsInFile;

    logOutputArea->append(QString("--- Analysis Results (File has %1 port(s)) ---").arg(numPortsInFile));

    if (closestIndex1 != -1) {
        double actualFreq1_GHz = frequencies.at(closestIndex1);
        logOutputArea->append(QString("Data for Primary Target (Closest to %1 GHz is %2 GHz, Index: %3)")
            .arg(QString::number(f_target1_GHz, 'g', 10))
            .arg(QString::number(actualFreq1_GHz, 'g', 10))
            .arg(closestIndex1));

        logSParametersForFrequencyPoint(closestIndex1, actualFreq1_GHz);

        std::complex<double> y11, y12, y21, y22;
        double z0_point1;
        int numPorts_point1_check;
        bool y_calc_success1 = calculateYMatrix(closestIndex1, y11, y12, y21, y22, z0_point1, numPorts_point1_check);

        if (y_calc_success1 && numPorts_point1_check == 2 && !std::isnan(y11.real())) {
            m_y11_calc = y11; m_y12_calc = y12; m_y21_calc = y21; m_y22_calc = y22;
            m_Z0_calc = z0_point1;
            m_actual_ftarget_hz_calc = actualFreq1_GHz * 1e9;
            m_analysisResultsAvailable = true;
            logOutputArea->append("Stored Y-parameters, Z0, and frequency from PRIMARY target for potential synthesis.");
        } else if (numPorts_point1_check == 2 && (!y_calc_success1 || std::isnan(y11.real())) ) {
             logOutputArea->append("Y-parameters for PRIMARY target are singular or could not be calculated. Cannot use for synthesis.");
        } else if (numPorts_point1_check != 2 && numPortsInFile == 2) {
             logOutputArea->append("Y-Matrix calculation for PRIMARY target indicated not 2-port, though file is 2-port. Check data integrity.");
        }

        calculateAndLogZMatrixForFrequencyPoint(closestIndex1, actualFreq1_GHz);
        calculateAndLogYMatrixForFrequencyPoint(closestIndex1, actualFreq1_GHz);

    } else {
        logOutputArea->append(QString("Could not find a closest frequency for the primary target %1 GHz.").arg(QString::number(f_target1_GHz, 'g', 10)));
    }

    logOutputArea->append("");

    bool process_f2_logging = true;
    if (closestIndex2 == closestIndex1 && std::abs(f_target1_GHz - f_target2_GHz) < 1e-12) {
        logOutputArea->append(QString("Secondary target frequency is effectively identical to primary; results already processed and shown."));
        if (m_analysisResultsAvailable) {
             m_y11_calc_low = m_y11_calc; m_y12_calc_low = m_y12_calc;
             m_y21_calc_low = m_y21_calc; m_y22_calc_low = m_y22_calc;
             m_Z0_calc_low = m_Z0_calc;
             m_actual_flow_hz_calc = m_actual_ftarget_hz_calc;
             m_lowFreqAnalysisResultsAvailable = true;
             logOutputArea->append("Copied primary target results to secondary low-frequency results as targets were identical.");
        } else {
            m_lowFreqAnalysisResultsAvailable = false;
        }
        process_f2_logging = false;
    }

    if (closestIndex2 != -1 && process_f2_logging) {
        double actualFreq2_GHz = frequencies.at(closestIndex2);
        logOutputArea->append(QString("Data for Secondary Target (Closest to %1 GHz is %2 GHz, Index: %3)")
            .arg(QString::number(f_target2_GHz, 'g', 10))
            .arg(QString::number(actualFreq2_GHz, 'g', 10))
            .arg(closestIndex2));

        logSParametersForFrequencyPoint(closestIndex2, actualFreq2_GHz);

        std::complex<double> y11_low, y12_low, y21_low, y22_low;
        double z0_point2;
        int numPorts_point2_check;
        bool y_calc_success2 = calculateYMatrix(closestIndex2, y11_low, y12_low, y21_low, y22_low, z0_point2, numPorts_point2_check);

        if (y_calc_success2 && numPorts_point2_check == 2 && !std::isnan(y11_low.real())) {
            m_y11_calc_low = y11_low; m_y12_calc_low = y12_low;
            m_y21_calc_low = y21_low; m_y22_calc_low = y22_low;
            m_Z0_calc_low = z0_point2;
            m_actual_flow_hz_calc = actualFreq2_GHz * 1e9;
            m_lowFreqAnalysisResultsAvailable = true;
            logOutputArea->append("Stored Y-parameters, Z0, and frequency from SECONDARY (low-freq) target for potential synthesis.");
        } else if (numPorts_point2_check == 2 && (!y_calc_success2 || std::isnan(y11_low.real())) ) {
             logOutputArea->append("Y-parameters for SECONDARY (low-freq) target are singular or could not be calculated.");
             m_lowFreqAnalysisResultsAvailable = false;
        } else if (numPorts_point2_check != 2 && numPortsInFile == 2) {
             logOutputArea->append("Y-Matrix calculation for SECONDARY (low-freq) target indicated not 2-port, though file is 2-port. Check data integrity.");
             m_lowFreqAnalysisResultsAvailable = false;
        } else {
            m_lowFreqAnalysisResultsAvailable = false;
        }

        calculateAndLogZMatrixForFrequencyPoint(closestIndex2, actualFreq2_GHz);
        calculateAndLogYMatrixForFrequencyPoint(closestIndex2, actualFreq2_GHz);

    } else if (process_f2_logging) {
        logOutputArea->append(QString("Could not find a closest frequency for the secondary target %1 GHz.").arg(QString::number(f_target2_GHz, 'g', 10)));
        m_lowFreqAnalysisResultsAvailable = false;
    }
}


void QucsTouchstoneViewer::handleLogMessage(const QString& message) {
    if (logOutputArea) {
        logOutputArea->append(message);
    }
}

QString QucsTouchstoneViewer::formatComplex(const std::complex<double>& num) {
    if (std::isinf(num.real()) || std::isinf(num.imag()) ||
        std::isnan(num.real()) || std::isnan(num.imag())) {
        QString realStr = std::isinf(num.real()) ? (num.real() > 0 ? "inf" : "-inf") : (std::isnan(num.real()) ? "nan" : QString::number(num.real(), 'f', 4));
        QString imagStr = std::isinf(num.imag()) ? (num.imag() > 0 ? "inf" : "-inf") : (std::isnan(num.imag()) ? "nan" : QString::number(std::abs(num.imag()), 'f', 4));
        return QString("%1 %2 j%3").arg(realStr).arg(num.imag() < 0 ? "-" : "+").arg(imagStr);
    }
    return QString::asprintf("%.4f %c j%.4f", num.real(), (num.imag() < 0 ? '-' : '+'), std::abs(num.imag()));
}

void QucsTouchstoneViewer::logSParametersForFrequencyPoint(int pointIndex, double actualFreq) {
    if (pointIndex < 0 || !m_fullTouchstoneData.contains("frequency") || pointIndex >= m_fullTouchstoneData["frequency"].size()) {
        logOutputArea->append(QString("Error: Invalid index %1 for S-parameter logging.").arg(pointIndex));
        return;
    }

    logOutputArea->append(QString("\n--- S-Parameters at %1 GHz ---").arg(QString::number(actualFreq, 'g', 10)));

    int numPorts = 0;
    if (m_fullTouchstoneData.contains("n_ports") && !m_fullTouchstoneData["n_ports"].isEmpty()) {
        numPorts = static_cast<int>(m_fullTouchstoneData["n_ports"].first());
    }

    if (numPorts == 0) {
        logOutputArea->append("Number of ports unknown, cannot log S-parameters reliably.");
        return;
    }

    for (int i = 1; i <= numPorts; ++i) {
        for (int j = 1; j <= numPorts; ++j) {
            QString s_re_key = QString("S%1%2_re").arg(i).arg(j);
            QString s_im_key = QString("S%1%2_im").arg(i).arg(j);

            if (m_fullTouchstoneData.contains(s_re_key) && m_fullTouchstoneData.contains(s_im_key) &&
                pointIndex < m_fullTouchstoneData[s_re_key].size() && pointIndex < m_fullTouchstoneData[s_im_key].size()) {

                double s_re = m_fullTouchstoneData[s_re_key].at(pointIndex);
                double s_im = m_fullTouchstoneData[s_im_key].at(pointIndex);

                if (std::isnan(s_re) || std::isnan(s_im)) {
                    logOutputArea->append(QString("S%1%2 = NaN").arg(i).arg(j));
                } else {
                    QString s_param_name = QString("S%1%2").arg(i).arg(j);
                    logOutputArea->append(s_param_name + " = " + formatComplex({s_re, s_im}));
                }
            } else {
                logOutputArea->append(QString("S%1%2 = Data N/A").arg(i).arg(j));
            }
        }
    }
}

bool QucsTouchstoneViewer::calculateYMatrix(int pointIndex,
                                           std::complex<double>& y11_out, std::complex<double>& y12_out,
                                           std::complex<double>& y21_out, std::complex<double>& y22_out,
                                           double& Z0_at_point_out, int& numPorts_at_point_out)
{
    numPorts_at_point_out = 0;
    if (m_fullTouchstoneData.contains("n_ports") && !m_fullTouchstoneData["n_ports"].isEmpty()) {
        numPorts_at_point_out = static_cast<int>(m_fullTouchstoneData["n_ports"].first());
    }

    if (numPorts_at_point_out != 2) {
        return false;
    }

    if (pointIndex < 0 || !m_fullTouchstoneData.contains("Z0") || pointIndex >= m_fullTouchstoneData["Z0"].size()) {
        return false;
    }
    Z0_at_point_out = m_fullTouchstoneData["Z0"].at(pointIndex);
    if (Z0_at_point_out == 0.0) {
        return false;
    }
    double Y0_val = 1.0 / Z0_at_point_out;

    std::complex<double> s11, s12, s21, s22;
    QStringList s_indices = {"11", "12", "21", "22"};
    std::vector<std::complex<double>*> s_params_ptrs = {&s11, &s12, &s21, &s22};

    for (size_t k = 0; k < s_indices.size(); ++k) {
        QString re_key = QString("S%1_re").arg(s_indices.at(k));
        QString im_key = QString("S%1_im").arg(s_indices.at(k));
        if (m_fullTouchstoneData.contains(re_key) && m_fullTouchstoneData.contains(im_key) &&
            pointIndex < m_fullTouchstoneData[re_key].size() && pointIndex < m_fullTouchstoneData[im_key].size()) {
            *(s_params_ptrs[k]) = {m_fullTouchstoneData[re_key].at(pointIndex), m_fullTouchstoneData[im_key].at(pointIndex)};
            if (std::isnan(s_params_ptrs[k]->real()) || std::isnan(s_params_ptrs[k]->imag())) {
                return false;
            }
        } else {
            return false;
        }
    }

    std::complex<double> den_y = (1.0 + s11) * (1.0 + s22) - s12 * s21;

    if (std::abs(den_y) < 1e-12) {
        y11_out = y12_out = y21_out = y22_out = std::numeric_limits<double>::quiet_NaN();
        return true;
    }

    y11_out = Y0_val * ((1.0 - s11) * (1.0 + s22) + s12 * s21) / den_y;
    y12_out = Y0_val * (-2.0 * s12) / den_y;
    y21_out = Y0_val * (-2.0 * s21) / den_y;
    y22_out = Y0_val * ((1.0 + s11) * (1.0 - s22) + s12 * s21) / den_y;
    return true; // Success
}


void QucsTouchstoneViewer::calculateAndLogZMatrixForFrequencyPoint(int pointIndex, double actualFreq) {
    int numPorts = 0;
    if (m_fullTouchstoneData.contains("n_ports") && !m_fullTouchstoneData["n_ports"].isEmpty()) {
        numPorts = static_cast<int>(m_fullTouchstoneData["n_ports"].first());
    }

    if (numPorts != 2) {
        logOutputArea->append(QString("Z-Matrix calculation skipped: Only supported for 2-port data (found %1 ports).").arg(numPorts));
        return;
    }

    if (pointIndex < 0 || !m_fullTouchstoneData.contains("Z0") || pointIndex >= m_fullTouchstoneData["Z0"].size()) {
        logOutputArea->append(QString("Error: Invalid index %1 or missing Z0 for Z-matrix calculation.").arg(pointIndex));
        return;
    }
    double Z0_val = m_fullTouchstoneData["Z0"].at(pointIndex);

    std::complex<double> s11, s12, s21, s22;
    QStringList s_indices = {"11", "12", "21", "22"};
    std::vector<std::complex<double>*> s_params_ptrs = {&s11, &s12, &s21, &s22};

    for (size_t k = 0; k < s_indices.size(); ++k) {
        QString re_key = QString("S%1_re").arg(s_indices.at(k));
        QString im_key = QString("S%1_im").arg(s_indices.at(k));
        if (m_fullTouchstoneData.contains(re_key) && m_fullTouchstoneData.contains(im_key) &&
            pointIndex < m_fullTouchstoneData[re_key].size() && pointIndex < m_fullTouchstoneData[im_key].size()) {
            *(s_params_ptrs[k]) = {m_fullTouchstoneData[re_key].at(pointIndex), m_fullTouchstoneData[im_key].at(pointIndex)};
             if (std::isnan(s_params_ptrs[k]->real()) || std::isnan(s_params_ptrs[k]->imag())) {
                logOutputArea->append(QString("Error: S%1 contains NaN, cannot calculate Z-Matrix.").arg(s_indices.at(k)));
                return;
            }
        } else {
            logOutputArea->append(QString("Error: Missing S%1 data for Z-Matrix calculation.").arg(s_indices.at(k)));
            return;
        }
    }

    logOutputArea->append(QString("\n--- Z-Matrix at %1 GHz (Z0 = %2 Ohms) ---")
        .arg(QString::number(actualFreq, 'g', 10)).arg(Z0_val));

    std::complex<double> den_z = (1.0 - s11) * (1.0 - s22) - s12 * s21;

    if (std::abs(den_z) < 1e-12) {
        logOutputArea->append("Z-Matrix: Denominator is near zero, parameters are singular/infinite.");
        logOutputArea->append("Z11 = Singular");
        logOutputArea->append("Z12 = Singular");
        logOutputArea->append("Z21 = Singular");
        logOutputArea->append("Z22 = Singular");
        return;
    }

    std::complex<double> Z11 = Z0_val * ((1.0 + s11) * (1.0 - s22) + s12 * s21) / den_z;
    std::complex<double> Z12 = Z0_val * (2.0 * s12) / den_z;
    std::complex<double> Z21 = Z0_val * (2.0 * s21) / den_z;
    std::complex<double> Z22 = Z0_val * ((1.0 - s11) * (1.0 + s22) + s12 * s21) / den_z;

    logOutputArea->append(QString("Z11 = %1").arg(formatComplex(Z11)));
    logOutputArea->append(QString("Z12 = %1").arg(formatComplex(Z12)));
    logOutputArea->append(QString("Z21 = %1").arg(formatComplex(Z21)));
    logOutputArea->append(QString("Z22 = %1").arg(formatComplex(Z22)));
}

void QucsTouchstoneViewer::calculateAndLogYMatrixForFrequencyPoint(int pointIndex, double actualFreq) {
    logOutputArea->append(QString("\n--- Y-Matrix at %1 GHz ---").arg(QString::number(actualFreq, 'g', 10)));

    std::complex<double> y11, y12, y21, y22;
    double Z0_at_point;
    int numPorts_at_point;

    bool success = calculateYMatrix(pointIndex, y11, y12, y21, y22, Z0_at_point, numPorts_at_point);

    if (numPorts_at_point != 2) {
        logOutputArea->append(QString("Y-Matrix calculation skipped: Only supported for 2-port data (found %1 ports).").arg(numPorts_at_point));
        return;
    }
    if (!success) {
        logOutputArea->append("Error: Could not calculate Y-Matrix due to missing data, NaN S-parameters, or Z0=0.");
        return;
    }
    if (std::isnan(y11.real())) {
         logOutputArea->append("Y-Matrix: Parameters are singular/infinite (denominator was near zero).");
         logOutputArea->append("Y11 = Singular");
         logOutputArea->append("Y12 = Singular");
         logOutputArea->append("Y21 = Singular");
         logOutputArea->append("Y22 = Singular");
         return;
    }

    logOutputArea->append(QString("(Using Z0 = %1 Ohms, Y0 = 1/Z0 Siemens)").arg(Z0_at_point));
    logOutputArea->append(QString("Y11 = %1").arg(formatComplex(y11)));
    logOutputArea->append(QString("Y12 = %1").arg(formatComplex(y12)));
    logOutputArea->append(QString("Y21 = %1").arg(formatComplex(y21)));
    logOutputArea->append(QString("Y22 = %1").arg(formatComplex(y22)));
}

double QucsTouchstoneViewer::roundToNDecimals(double value, int n) {
    double multiplier = std::pow(10.0, n);
    return std::round(value * multiplier) / multiplier;
}

QString QucsTouchstoneViewer::formatComponentValue(double rawValue, const QString& componentType)
{
    struct SIPrefix {
        double multiplier;
        QString prefixChar;
    };
    std::vector<SIPrefix> prefixes;

    if (componentType.toUpper() == "R") {
        prefixes = {{1e9, "G"}, {1e6, "M"}, {1e3, "k"}, {1.0, ""}, {1e-3, "m"}};
    } else if (componentType.toUpper() == "L") {
        prefixes = {{1e9, "G"}, {1e6, "M"}, {1e3, "k"}, {1.0, ""}, {1e-3, "m"}, {1e-6, "u"}, {1e-9, "n"}, {1e-12, "p"}};
    } else if (componentType.toUpper() == "C") {
        prefixes = {{1.0, ""}, {1e-3, "m"}, {1e-6, "u"}, {1e-9, "n"}, {1e-12, "p"}, {1e-15, "f"}};
    } else {
        qWarning() << "Unknown component type for formatComponentValue:" << componentType;
        std::ostringstream oss_err;
        oss_err << std::fixed << std::setprecision(2) << rawValue;
        return QString::fromStdString(oss_err.str());
    }

    QString bestPrefixChar = "";
    double bestScaledValue = rawValue;

    if (rawValue == 0.0) {
        bestPrefixChar = "";
        bestScaledValue = 0.0;
    } else {
        std::sort(prefixes.begin(), prefixes.end(), [](const SIPrefix& a, const SIPrefix& b){
            return a.multiplier > b.multiplier;
        });

        bool foundIdealPrefix = false;
        if (!prefixes.empty()) {
            for (const auto& p : prefixes) {
                if (p.multiplier <= 0) continue;

                double scaledValue = rawValue / p.multiplier;
                if (scaledValue >= 1.0 && scaledValue < 1000.0) {
                    bestScaledValue = scaledValue;
                    bestPrefixChar = p.prefixChar;
                    foundIdealPrefix = true;
                    break;
                }
            }

            if (!foundIdealPrefix) {
                if (rawValue >= prefixes.front().multiplier * 1000.0 && prefixes.front().multiplier > 0) {
                    bestScaledValue = rawValue / prefixes.front().multiplier;
                    bestPrefixChar = prefixes.front().prefixChar;
                } else {
                    bestScaledValue = rawValue / prefixes.back().multiplier;
                    bestPrefixChar = prefixes.back().prefixChar;
                }
            }
        }
    }

    double finalValueRounded = roundToNDecimals(bestScaledValue, 2);
    int precision = 2;
    if (finalValueRounded == 0.0 && rawValue != 0.0 && std::abs(bestScaledValue) > 1e-5 ) {
        finalValueRounded = roundToNDecimals(bestScaledValue, 3);
        precision = 3;
        if (finalValueRounded == 0.0 && std::abs(bestScaledValue) > 1e-5) {
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

QString QucsTouchstoneViewer::generateFormattedRandomValue(double minVal, double maxVal, const QString& componentType)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib(minVal, maxVal);
    double rawValue = distrib(gen);

    if (rawValue == 0.0 && (minVal != 0.0 || maxVal != 0.0)) {
        if (minVal > 0 && minVal < 1.0) rawValue = minVal * 0.01 + std::numeric_limits<double>::epsilon();
        else if (minVal > 0) rawValue = std::numeric_limits<double>::epsilon();
    }

    return formatComponentValue(rawValue, componentType);
}


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

void QucsTouchstoneViewer::displayData(int specificRowIndex /* = -1 */)
{
    dataTable->clearContents();
    logOutputArea->append(QString("Updating display. Specific row: %1").arg(specificRowIndex));

    if (m_fullTouchstoneData.isEmpty() || !m_fullTouchstoneData.contains("frequency") || m_fullTouchstoneData["frequency"].isEmpty()) {
        dataTable->setRowCount(0);
        qWarning() << "displayData called with no valid m_fullTouchstoneData.";
        return;
    }

    const QList<double>& freq = m_fullTouchstoneData["frequency"];
    int number_of_ports = 0;
    if (m_fullTouchstoneData.contains("n_ports") && !m_fullTouchstoneData["n_ports"].isEmpty()) {
        number_of_ports = static_cast<int>(m_fullTouchstoneData["n_ports"].first());
    }

    QStringList sParamTableColumns = {"11", "12", "21", "22"};

    if (specificRowIndex != -1) {
        if (specificRowIndex >= 0 && specificRowIndex < freq.size()) {
            dataTable->setRowCount(1);
            qDebug() << "Displaying single row index:" << specificRowIndex << "Freq:" << freq.at(specificRowIndex);

            dataTable->setItem(0, 0, new QTableWidgetItem(QString::number(freq.at(specificRowIndex), 'g', 10)));

            for (int j = 0; j < sParamTableColumns.size(); ++j) {
                QString current_s_param_index = sParamTableColumns.at(j);
                QString s_param_key_db = QString("S%1_dB").arg(current_s_param_index);
                bool should_display_sparam = false;

                if (number_of_ports == 1 && current_s_param_index == "11") should_display_sparam = true;
                else if (number_of_ports >= 2) should_display_sparam = true;

                if (should_display_sparam && m_fullTouchstoneData.contains(s_param_key_db) && specificRowIndex < m_fullTouchstoneData[s_param_key_db].size()) {
                    double val = m_fullTouchstoneData[s_param_key_db].at(specificRowIndex);
                    dataTable->setItem(0, j + 1, new QTableWidgetItem(std::isnan(val) ? "NaN" : QString::number(val, 'f', 4)));
                } else {
                    dataTable->setItem(0, j + 1, new QTableWidgetItem("N/A"));
                }
            }

            if (m_fullTouchstoneData.contains("Z0") && specificRowIndex < m_fullTouchstoneData["Z0"].size()) {
                 dataTable->setItem(0, 5, new QTableWidgetItem(QString::number(m_fullTouchstoneData["Z0"].at(specificRowIndex), 'f', 2)));
            } else if (m_fullTouchstoneData.contains("Z0") && !m_fullTouchstoneData["Z0"].isEmpty()){
                dataTable->setItem(0, 5, new QTableWidgetItem(QString::number(m_fullTouchstoneData["Z0"].first(), 'f', 2)));
            } else {
                dataTable->setItem(0, 5, new QTableWidgetItem("50.00 (default)"));
            }
        } else {
            dataTable->setRowCount(0);
            qWarning() << "displayData called with invalid specificRowIndex:" << specificRowIndex << "Max index:" << freq.size() -1;
        }
    } else {
        int numRowsToShow = qMin(10, freq.size());
        dataTable->setRowCount(numRowsToShow);
        qDebug() << "Displaying default view, rows:" << numRowsToShow;

        for (int i = 0; i < numRowsToShow; ++i) {
            dataTable->setItem(i, 0, new QTableWidgetItem(QString::number(freq.at(i), 'g', 10)));

            for (int j = 0; j < sParamTableColumns.size(); ++j) {
                QString current_s_param_index = sParamTableColumns.at(j);
                QString s_param_key_db = QString("S%1_dB").arg(current_s_param_index);
                bool should_display_sparam = false;

                if (number_of_ports == 1 && current_s_param_index == "11") should_display_sparam = true;
                else if (number_of_ports >= 2) should_display_sparam = true;

                if (should_display_sparam && m_fullTouchstoneData.contains(s_param_key_db) && i < m_fullTouchstoneData[s_param_key_db].size()) {
                    double val = m_fullTouchstoneData[s_param_key_db].at(i);
                    dataTable->setItem(i, j + 1, new QTableWidgetItem(std::isnan(val) ? "NaN" : QString::number(val, 'f', 4)));
                } else {
                    dataTable->setItem(i, j + 1, new QTableWidgetItem("N/A"));
                }
            }

            if (m_fullTouchstoneData.contains("Z0") && i < m_fullTouchstoneData["Z0"].size()) {
                 dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(m_fullTouchstoneData["Z0"].at(i), 'f', 2)));
            } else if (m_fullTouchstoneData.contains("Z0") && !m_fullTouchstoneData["Z0"].isEmpty()){
                dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(m_fullTouchstoneData["Z0"].first(), 'f', 2)));
            } else {
                dataTable->setItem(i, 5, new QTableWidgetItem("50.00 (default)"));
            }
        }
    }
}
