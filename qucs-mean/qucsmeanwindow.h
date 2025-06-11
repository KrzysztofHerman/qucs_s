#ifndef QUCSMEANWINDOW_H
#define QUCSMEANWINDOW_H

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

class QucsMeanWindow : public QMainWindow
{
    Q_OBJECT

public:
    QucsMeanWindow(QWidget *parent = nullptr);
    ~QucsMeanWindow();

private slots:
    void calculateMean();

private:
    QLineEdit *input1LineEdit;
    QLineEdit *input2LineEdit;
    QPushButton *calculateButton;
    QLabel *resultLabel;
};

#endif // QUCSMEANWINDOW_H
