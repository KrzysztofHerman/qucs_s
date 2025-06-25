#ifndef QUCSOUCHSTONEVIEWER_H
#define QUCSOUCHSTONEVIEWER_H

#include <QMainWindow>
// #include <QTableWidget> // No longer needed
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
#include <cmath>
#include <limits>
#include <QTextEdit>
#include <QBuffer>
#include <QComboBox>
#include <random>
#include <vector>
#include <iomanip>
#include <sstream>
#include <QLineEdit>
#include <QDoubleValidator>
#include <complex>
#include <QSvgWidget> // Standard Qt5 include when Qt5::Svg is linked


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
    // void loadInternalTestData(); // REMOVED
    void onSynthesizeClicked();
    void onAnalyzeFrequencyClicked();
    void onNetworkTypeChanged(const QString& newType); // ADDED


private:
    void createWidgets();
    QMap<QString, QList<double>> readTouchstoneFile(const QString& filePath);
    // void displayData(int specificRowIndex = -1); // REMOVED (will be removed in step 4 of plan)
    void convert_MA_RI_to_dB(double * S_1, double * S_2, double *S_3, double *S_4, QString format);
    QString formatComponentValue(double rawValue, const QString& componentType);
    QString generateFormattedRandomValue(double minVal, double maxVal, const QString& componentType);
    double roundToNDecimals(double value, int n);

    void logSParametersForFrequencyPoint(int pointIndex, double actualFreq);
    void calculateAndLogZMatrixForFrequencyPoint(int pointIndex, double actualFreq);
    void calculateAndLogYMatrixForFrequencyPoint(int pointIndex, double actualFreq);
    bool calculateYMatrix(int pointIndex,
                          std::complex<double>& y11_out, std::complex<double>& y12_out,
                          std::complex<double>& y21_out, std::complex<double>& y22_out,
                          double& Z0_at_point_out, int& numPorts_at_point_out);

    // UI Elements
    QPushButton *openButton;
    // QPushButton *loadInternalDataButton; // REMOVED
    QTextEdit *logOutputArea;
    QComboBox *networkTypeComboBox;
    QPushButton *synthesizeButton;
    QLineEdit *targetFrequencyInput;
    QComboBox *targetFrequencyUnitComboBox;
    QPushButton *analyzeFrequencyButton;
    // QTableWidget *dataTable; // REMOVED
    QSvgWidget *networkDisplayWidget; // ADDED


    // Member variables for storing data and state
    QMap<QString, QList<double>> m_fullTouchstoneData;
    bool m_isTargetFrequencyApplied; // Uncommented as it's used

    bool m_analysisResultsAvailable;
    int  m_analyzedNumPorts;
    double m_actual_ftarget_hz_calc;
    double m_Z0_calc;
    std::complex<double> m_y11_calc;
    std::complex<double> m_y12_calc;
    std::complex<double> m_y21_calc;
    std::complex<double> m_y22_calc;

    bool m_lowFreqAnalysisResultsAvailable;
    double m_actual_flow_hz_calc;
    double m_Z0_calc_low;
    std::complex<double> m_y11_calc_low;
    std::complex<double> m_y12_calc_low;
    std::complex<double> m_y21_calc_low;
    std::complex<double> m_y22_calc_low;

    static QTextEdit* S_logOutputArea;
public:
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};

#endif // QUCSOUCHSTONEVIEWER_H
