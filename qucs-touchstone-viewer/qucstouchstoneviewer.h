#ifndef QUCSOUCHSTONEVIEWER_H
#define QUCSOUCHSTONEVIEWER_H

#include <QMainWindow>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>
#include <QMap>
#include <QList>
#include <QFileInfo>
#include <QRegularExpression>
#include <cmath> // For fmod, sqrt, pow, log10, cos, sin, atan2
#include <limits> // Required for std::numeric_limits
#include <QTextEdit> // For log display
#include <QBuffer>   // For handling string as I/O device
#include <QComboBox> // For QComboBox
#include <random> // For std::mt19937, std::uniform_real_distribution
#include <vector> // For std::vector (used in helper function)
#include <iomanip> // For std::fixed, std::setprecision (used via stringstream)
#include <sstream> // For std::ostringstream
#include <QLineEdit>        // For target frequency input
#include <QDoubleValidator> // To validate numeric input in QLineEdit
#include <complex> // For std::complex

// Assuming tQucsSettings is defined elsewhere and accessible
struct tQucsSettings {
  int x, y;
  // Add other settings if needed by QucsTouchstoneViewer
};
extern struct tQucsSettings QucsSettings;


class QucsTouchstoneViewer : public QMainWindow
{
    Q_OBJECT

public:
    QucsTouchstoneViewer(QWidget *parent = 0);
    ~QucsTouchstoneViewer();

private slots:
    void openFile();

private:
    void createWidgets();
    QMap<QString, QList<double>> readTouchstoneFile(const QString& filePath);
    void displayData(int specificRowIndex = -1); // -1 means display default (first 10 or all if less)
    void convert_MA_RI_to_dB(double * S_1, double * S_2, double *S_3, double *S_4, QString format);


    QPushButton *openButton;
    QTableWidget *dataTable;
    QPushButton *loadInternalDataButton;
    QTextEdit *logOutputArea;

    // Static member to hold the instance of the log area for the message handler
    static QTextEdit* S_logOutputArea;

    // Helper functions for synthesis
    double roundToNDecimals(double value, int n);
    QString formatComponentValue(double rawValue, const QString& componentType);
    QString generateFormattedRandomValue(double minVal, double maxVal, const QString& componentType);

    // New UI elements for network synthesis
    QComboBox *networkTypeComboBox;
    QPushButton *synthesizeButton;

    // New UI elements for target frequency
    QLineEdit *targetFrequencyInput;
    QComboBox *targetFrequencyUnitComboBox;
    QPushButton *analyzeFrequencyButton;

    // Member variables for storing full data and filter state
    QMap<QString, QList<double>> m_fullTouchstoneData;
    bool m_isTargetFrequencyApplied;

    // Members to store results from the last primary target frequency analysis (for 2-port files)
    bool m_analysisResultsAvailable;
    int  m_analyzedNumPorts; // Number of ports of the file for which analysis results are stored
    double m_actual_ftarget_hz_calc; // Actual frequency (in Hz) of the primary target point
    double m_Z0_calc;                // Z0 at the primary target point
    std::complex<double> m_y11_calc;
    std::complex<double> m_y12_calc;
    std::complex<double> m_y21_calc;
    std::complex<double> m_y22_calc;

    // New members for secondary (low) frequency analysis results
    bool m_lowFreqAnalysisResultsAvailable;
    // m_analyzedNumPorts is shared, Z0 and actual frequency for low point will be stored
    double m_actual_flow_hz_calc; // Actual frequency (in Hz) of the low frequency point
    double m_Z0_calc_low;         // Z0 at the low frequency point
    std::complex<double> m_y11_calc_low;
    std::complex<double> m_y12_calc_low;
    std::complex<double> m_y21_calc_low;
    std::complex<double> m_y22_calc_low;

    // Helpers for S, Z, Y matrix logging
    void logSParametersForFrequencyPoint(int pointIndex, double actualFreq);
    void calculateAndLogZMatrixForFrequencyPoint(int pointIndex, double actualFreq);
    // Refactored calculation function (declaration)
    bool calculateYMatrix(int pointIndex,
                          std::complex<double>& y11_out, std::complex<double>& y12_out,
                          std::complex<double>& y21_out, std::complex<double>& y22_out,
                          double& Z0_at_point_out, int& numPorts_at_point_out);
    void calculateAndLogYMatrixForFrequencyPoint(int pointIndex, double actualFreq); // Logging wrapper
    QString formatComplex(const std::complex<double>& num); // Helper to format complex numbers

private slots:
    // ... existing openFile slot ...
    void loadInternalTestData();
    void handleLogMessage(const QString& message);
    void onSynthesizeClicked();
    void onAnalyzeFrequencyClicked(); // New slot for analyze frequency button

public: // Or private, depending on where qInstallMessageHandler is called
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};

#endif // QUCSOUCHSTONEVIEWER_H
