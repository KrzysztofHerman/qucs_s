#include "qucsmeanwindow.h"
#include <QMessageBox>
#include <QString>

QucsMeanWindow::QucsMeanWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Qucs Mean Calculator");

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    input1LineEdit = new QLineEdit(this);
    input1LineEdit->setPlaceholderText("Enter first number");
    layout->addWidget(input1LineEdit);

    input2LineEdit = new QLineEdit(this);
    input2LineEdit->setPlaceholderText("Enter second number");
    layout->addWidget(input2LineEdit);

    calculateButton = new QPushButton("Calculate", this);
    layout->addWidget(calculateButton);

    resultLabel = new QLabel("Result: ", this);
    layout->addWidget(resultLabel);

    setCentralWidget(centralWidget);

    connect(calculateButton, &QPushButton::clicked, this, &QucsMeanWindow::calculateMean);
}

QucsMeanWindow::~QucsMeanWindow()
{
}

void QucsMeanWindow::calculateMean()
{
    bool ok1, ok2;
    double num1 = input1LineEdit->text().toDouble(&ok1);
    double num2 = input2LineEdit->text().toDouble(&ok2);

    if (ok1 && ok2) {
        double mean = (num1 + num2) / 2.0;
        resultLabel->setText("Result: " + QString::number(mean));
    } else {
        QMessageBox::warning(this, "Input Error", "Please enter valid numbers.");
        resultLabel->setText("Result: Error");
    }
}
