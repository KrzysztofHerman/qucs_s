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
    void displayData(const QMap<QString, QList<double>>& data);
    void convert_MA_RI_to_dB(double * S_1, double * S_2, double *S_3, double *S_4, QString format);


    QPushButton *openButton;
    QTableWidget *dataTable;
    QPushButton *loadInternalDataButton;
    QTextEdit *logOutputArea;

    // Static member to hold the instance of the log area for the message handler
    static QTextEdit* S_logOutputArea;

    // Helper functions for synthesis
    QString generateFormattedRandomValue(double minVal, double maxVal, const QString& componentType);
    double roundToNDecimals(double value, int n);

    // New UI elements for network synthesis
    QComboBox *networkTypeComboBox;
    QPushButton *synthesizeButton;

private slots:
    // ... existing openFile slot ...
    void loadInternalTestData();
    void handleLogMessage(const QString& message);
    void onSynthesizeClicked(); // New slot for synthesize button

public: // Or private, depending on where qInstallMessageHandler is called
    static void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
};

#endif // QUCSOUCHSTONEVIEWER_H
