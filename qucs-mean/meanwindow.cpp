#include "meanwindow.h"

#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QMessageBox>
#include <QString>

MeanWindow::MeanWindow(QWidget *parent)
    : QMainWindow(parent)
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    layout = new QVBoxLayout(centralWidget);

    inputField1 = new QLineEdit(this);
    inputField1->setPlaceholderText("Enter first number");
    layout->addWidget(inputField1);

    inputField2 = new QLineEdit(this);
    inputField2->setPlaceholderText("Enter second number");
    layout->addWidget(inputField2);

    calculateButton = new QPushButton("Calculate", this);
    layout->addWidget(calculateButton);

    resultLabel = new QLabel("Result: ", this);
    layout->addWidget(resultLabel);

    connect(calculateButton, &QPushButton::clicked, this, &MeanWindow::calculateMean);

    setWindowTitle("Qucs Mean Calculator (Qt6)");
}

MeanWindow::~MeanWindow()
{
}

void MeanWindow::calculateMean()
{
    bool ok1, ok2;
    double num1 = inputField1->text().toDouble(&ok1);
    double num2 = inputField2->text().toDouble(&ok2);

    if (ok1 && ok2) {
        double mean = (num1 + num2) / 2.0;
        resultLabel->setText(QString("Result: %1").arg(mean));
    } else {
        QMessageBox::warning(this, "Input Error", "Please enter valid numbers.");
        resultLabel->setText("Result: Error");
    }
}
