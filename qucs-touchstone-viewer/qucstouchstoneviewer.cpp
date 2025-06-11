#include "qucstouchstoneviewer.h"
#include <QHeaderView> // Required for QHeaderView
#include <QtMath> // For qDegreesToRadians and qRadiansToDegrees if needed, M_PI is in cmath

QucsTouchstoneViewer::QucsTouchstoneViewer(QWidget *parent)
    : QMainWindow(parent)
{
    createWidgets();
    setWindowTitle(tr("Qucs Touchstone Viewer"));
    setMinimumSize(600, 400);
}

QucsTouchstoneViewer::~QucsTouchstoneViewer()
{
}

void QucsTouchstoneViewer::createWidgets()
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    openButton = new QPushButton(tr("Select Touchstone File"), this);
    connect(openButton, &QPushButton::clicked, this, &QucsTouchstoneViewer::openFile);

    dataTable = new QTableWidget(this);
    dataTable->setColumnCount(6); // Freq, S11, S12, S21, S22 (magnitude for now) + Z0
    QStringList headers = {"Frequency (GHz)", "S11 (dB)", "S12 (dB)", "S21 (dB)", "S22 (dB)", "Z0 (Ohm)"};
    dataTable->setHorizontalHeaderLabels(headers);
    dataTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // Stretch columns

    layout->addWidget(openButton);
    layout->addWidget(dataTable);

    setCentralWidget(centralWidget);
}

void QucsTouchstoneViewer::openFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Touchstone File"),
                                                    "", tr("Touchstone files (*.s*p);;All files (*.*)"));
    if (!filePath.isEmpty()) {
        QMap<QString, QList<double>> data = readTouchstoneFile(filePath);
        if (!data.isEmpty()) {
            displayData(data);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Could not read or parse the Touchstone file."));
        }
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
        if (input_val1 < 0) input_val1 = 0; // Magnitude cannot be negative
        s_db = 20.0 * log10(input_val1 > 0 ? input_val1 : std::numeric_limits<double>::min()); // Avoid log(0)
        s_ang_deg = input_val2;
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = input_val1 * std::cos(ang_rad);
        s_im = input_val1 * std::sin(ang_rad);
    } else if (format == "RI") { // Real and Imaginary
        s_re = input_val1;
        s_im = input_val2;
        double mag = std::sqrt(s_re * s_re + s_im * s_im);
        s_db = 20.0 * log10(mag > 0 ? mag : std::numeric_limits<double>::min());
        s_ang_deg = qRadiansToDegrees(std::atan2(s_im, s_re));
    } else if (format == "DB") { // dB and Angle (degrees)
        s_db = input_val1;
        s_ang_deg = input_val2;
        double mag_lin = std::pow(10.0, s_db / 20.0);
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = mag_lin * std::cos(ang_rad);
        s_im = mag_lin * std::sin(ang_rad);
    } else { // Default or unknown format, assume MA
        qWarning() << "Unknown S-parameter format, assuming MA:" << format;
        if (input_val1 < 0) input_val1 = 0;
        s_db = 20.0 * log10(input_val1 > 0 ? input_val1 : std::numeric_limits<double>::min());
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
    double freq_scale_to_ghz = 1.0; // Default to GHz if not specified
    double Z0 = 50.0; // Default Z0

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open the file:" << filePath;
        return file_data;
    }

    QTextStream in(&file);
    int number_of_ports = 0;

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    if (suffix.startsWith('s') && suffix.endsWith('p')) {
        bool ok;
        int n = suffix.mid(1, suffix.length() - 2).toInt(&ok);
        if (ok && n > 0) {
            number_of_ports = n;
        }
    }
    file_data["n_ports"].append(number_of_ports); // Store initial guess or 0

    bool options_line_parsed = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith('!')) {
            continue;
        }

        if (line.startsWith('#')) {
            if (options_line_parsed) {
                 qWarning() << "Multiple option lines found. Using the first one.";
                 continue;
            }
            options_line_parsed = true;
            QStringList parts = line.split(QRegularExpression("\s+"), Qt::SkipEmptyParts);
            // # MHZ S MA R 50
            if (parts.length() > 1) frequency_unit_str = parts[1].toLower();
            if (parts.length() > 2) parameter_str = parts[2].toLower();
            if (parts.length() > 3) format_str = parts[3].toLower();
            if (parts.length() > 4 && (parts[4].toLower() == "r" || parts[4].toLower() == "z0")) { // Check for R or Z0 keyword
                 if (parts.length() > 5) Z0 = parts[5].toDouble();
            } else if (parts.length() > 4) {
                // If R/Z0 keyword is missing, the 5th part might be Z0 if it is a number
                bool ok;
                double z_check = parts[4].toDouble(&ok);
                if (ok) Z0 = z_check;
            }


            if (frequency_unit_str == "hz") freq_scale_to_ghz = 1e-9;
            else if (frequency_unit_str == "khz") freq_scale_to_ghz = 1e-6;
            else if (frequency_unit_str == "mhz") freq_scale_to_ghz = 1e-3;
            else if (frequency_unit_str == "ghz") freq_scale_to_ghz = 1.0;
            // Default is GHz, so freq_scale_to_ghz remains 1.0 if unit is missing or unrecognized

            // Parameter string (s, y, z, h, g) can also indicate file type,
            // but number_of_ports from extension is usually more reliable for .sNp
            continue;
        }

        // Data lines
        QStringList values = line.split(QRegularExpression("\s+"), Qt::SkipEmptyParts);
        if (values.isEmpty()) continue;

        if (number_of_ports == 0 && options_line_parsed) { // Determine N from first data line if not by extension
            // N = sqrt( (num_data_cols_for_S_params / 2) )
            // num_data_cols_for_S_params = values.length() - 1 (freq col)
            if (values.length() > 1) {
                int s_param_data_count = values.length() - 1;
                if (s_param_data_count > 0 && s_param_data_count % 2 == 0) {
                    int n_squared = s_param_data_count / 2;
                    double n_double = std::sqrt(n_squared);
                    if (std::fmod(n_double, 1.0) == 0.0) { // Check if it's an integer
                        number_of_ports = static_cast<int>(n_double);
                        file_data["n_ports"].clear(); // Clear previous
                        file_data["n_ports"].append(number_of_ports);
                    }
                }
            }
            if (number_of_ports == 0) {
                qDebug() << "Could not determine number of ports from data line and extension for file:" << filePath;
                return QMap<QString, QList<double>>(); // Critical error
            }
        }
        if (number_of_ports == 0 && !options_line_parsed) {
             qWarning() << "Skipping data line before options line for file:" << filePath;
             continue; // Don't process data if we don't know N and haven't seen #
        }


        file_data["frequency"].append(values[0].toDouble() * freq_scale_to_ghz);
        file_data["Z0"].append(Z0); // Append Z0 for each frequency point

        int current_val_idx = 1;
        for (int i = 1; i <= number_of_ports; ++i) {
            for (int j = 1; j <= number_of_ports; ++j) {
                QString s_param_mag_key = QString("S%1%2_dB").arg(i).arg(j);
                QString s_param_ang_key = QString("S%1%2_ang").arg(i).arg(j);
                QString s_param_re_key = QString("S%1%2_re").arg(i).arg(j);
                QString s_param_im_key = QString("S%1%2_im").arg(i).arg(j);

                if (current_val_idx + 1 < values.length()) {
                    double val1 = values[current_val_idx].toDouble();
                    double val2 = values[current_val_idx + 1].toDouble();
                    double s_re, s_im;

                    convert_MA_RI_to_dB(&val1, &val2, &s_re, &s_im, format_str);

                    file_data[s_param_mag_key].append(val1); // val1 is now dB
                    file_data[s_param_ang_key].append(val2); // val2 is now angle
                    file_data[s_param_re_key].append(s_re);
                    file_data[s_param_im_key].append(s_im);
                    current_val_idx += 2;
                } else {
                    // Handle incomplete data for Sij (e.g. end of line, or malformed)
                    // Append NaN or a placeholder to keep lists aligned with frequency
                    file_data[s_param_mag_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_ang_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_re_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_im_key].append(std::numeric_limits<double>::quiet_NaN());
                    // Ensure we don't go out of bounds if only one value is left
                    if (current_val_idx < values.length()) current_val_idx++;
                    qDebug() << "Incomplete data for S" << i << j << "at freq" << values[0];
                }
            }
        }
    }

    file.close();
    return file_data;
}

void QucsTouchstoneViewer::displayData(const QMap<QString, QList<double>>& data)
{
    dataTable->clearContents();

    if (!data.contains("frequency") || data["frequency"].isEmpty()) {
        dataTable->setRowCount(0);
        QMessageBox::information(this, tr("Info"), tr("No frequency data found in the file."));
        return;
    }

    const QList<double>& freq = data["frequency"];
    int numRowsToShow = qMin(10, freq.size());
    dataTable->setRowCount(numRowsToShow);

    QStringList sParamIndices = {"11", "12", "21", "22"}; // For S11, S12, S21, S22

    for (int i = 0; i < numRowsToShow; ++i) {
        // Frequency
        dataTable->setItem(i, 0, new QTableWidgetItem(QString::number(freq.at(i))));

        // S-parameters
        for (int j = 0; j < sParamIndices.size(); ++j) {
            QString s_param_key_db = QString("S%1_dB").arg(sParamIndices.at(j));
            if (data.contains(s_param_key_db) && i < data[s_param_key_db].size()) {
                dataTable->setItem(i, j + 1, new QTableWidgetItem(QString::number(data[s_param_key_db].at(i))));
            } else {
                dataTable->setItem(i, j + 1, new QTableWidgetItem("N/A"));
            }
        }

        // Z0
        if (data.contains("Z0") && i < data["Z0"].size()) {
             dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data["Z0"].at(i))));
        } else if (data.contains("Z0") && !data["Z0"].isEmpty()){
            // Fallback to first Z0 if list is shorter (shouldn't happen with current parser)
            dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data["Z0"].first())));
        } else {
            // Fallback if Z0 key is missing (shouldn't happen with current parser)
            dataTable->setItem(i, 5, new QTableWidgetItem("50.0 (default)"));
        }
    }
}
