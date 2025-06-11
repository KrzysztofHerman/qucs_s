#ifndef MEANWINDOW_H
#define MEANWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPushButton;
class QLabel;
class QVBoxLayout;
class QWidget;
QT_END_NAMESPACE

class MeanWindow : public QMainWindow
{
    Q_OBJECT

public:
    MeanWindow(QWidget *parent = nullptr);
    ~MeanWindow();

private slots:
    void calculateMean();

private:
    QLineEdit *inputField1;
    QLineEdit *inputField2;
    QPushButton *calculateButton;
    QLabel *resultLabel;
    QWidget *centralWidget;
    QVBoxLayout *layout;
};

#endif // MEANWINDOW_H
