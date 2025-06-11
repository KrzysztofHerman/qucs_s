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
};

#endif // QUCSOUCHSTONEVIEWER_H
