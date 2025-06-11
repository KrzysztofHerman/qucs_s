#include "qucstouchstoneviewer.h"
#include <QHeaderView> // Required for QHeaderView
#include <QtMath> // For qDegreesToRadians and qRadiansToDegrees if needed, M_PI is in cmath

// QucsTouchstoneViewer constructor and destructor remain the same...
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
        if (!data.isEmpty() && data.contains("frequency") && !data["frequency"].isEmpty()) { // Check if data is not empty and contains valid frequency data
            displayData(data);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Could not read or parse valid data from the Touchstone file."));
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
        if (input_val1 < 0) {
            qWarning() << "Magnitude (MA) is negative:" << input_val1 << ". Using abs().";
            input_val1 = std::abs(input_val1);
        }
        // Use epsilon for effectively zero check; use min_positive for log10 argument if zero.
        s_db = 20.0 * log10(input_val1 > std::numeric_limits<double>::epsilon() ? input_val1 : std::numeric_limits<double>::min());
        s_ang_deg = input_val2;
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = input_val1 * std::cos(ang_rad);
        s_im = input_val1 * std::sin(ang_rad);
    } else if (format == "RI") { // Real and Imaginary
        s_re = input_val1;
        s_im = input_val2;
        double mag = std::sqrt(s_re * s_re + s_im * s_im);
        s_db = 20.0 * log10(mag > std::numeric_limits<double>::epsilon() ? mag : std::numeric_limits<double>::min());
        s_ang_deg = qRadiansToDegrees(std::atan2(s_im, s_re));
    } else if (format == "DB") { // dB and Angle (degrees)
        s_db = input_val1;
        s_ang_deg = input_val2;
        double mag_lin = std::pow(10.0, s_db / 20.0);
        double ang_rad = qDegreesToRadians(s_ang_deg);
        s_re = mag_lin * std::cos(ang_rad);
        s_im = mag_lin * std::sin(ang_rad);
    } else { // Default or unknown format, assume MA
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
    double freq_scale_to_ghz = 1.0; // Default to GHz if not specified
    double Z0 = 50.0; // Default Z0

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open the file:" << filePath;
        return file_data; // Return empty map
    }

    QTextStream in(&file);
    int number_of_ports = 0;
    bool options_line_parsed = false;
    bool first_data_line = true; // To help determine N from data if not from extension

    QFileInfo fileInfo(filePath);
    QString suffix = fileInfo.suffix().toLower();
    if (suffix.startsWith('s') && suffix.endsWith('p')) {
        bool ok;
        int n = suffix.mid(1, suffix.length() - 2).toInt(&ok);
        if (ok && n > 0) {
            number_of_ports = n;
            qDebug() << "Number of ports from extension:" << number_of_ports;
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
            QStringList parts = line.split(QRegularExpression("\s+"), Qt::SkipEmptyParts);

            if (parts.length() > 1) frequency_unit_str = parts[1].toLower();
            if (parts.length() > 2) parameter_str = parts[2].toLower();
            if (parts.length() > 3) format_str = parts[3].toLower();

            bool z0_found_keyword = false;
            for(int k=4; k < parts.length(); ++k) { // Iterate to find R or Z0
                if(parts[k].toLower() == "r" || parts[k].toLower() == "z0") {
                    if (k+1 < parts.length()) {
                        bool ok;
                        double parsed_Z0 = parts[k+1].toDouble(&ok);
                        if(ok) {
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
                 // Fallback: if # freq S FMT value (where value is Z0)
                 // This is for cases like "# GHZ S MA 50"
                 // Check if parts[4] is a number and not a keyword itself
                QString potential_z0_str = parts[4].toLower();
                if (potential_z0_str != "r" && potential_z0_str != "z0") {
                    bool ok;
                    double z_check = parts[4].toDouble(&ok);
                    if (ok) Z0 = z_check;
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
                freq_scale_to_ghz = 1.0; // Default to GHz
            }
            qDebug() << "Options line parsed: FreqUnit=" << frequency_unit_str << "Param=" << parameter_str << "Format=" << format_str << "Z0=" << Z0 << "FreqScaleToGHz=" << freq_scale_to_ghz;
            continue;
        }

        // --- Data line processing ---
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
            if (!file_data["frequency"].isEmpty()) {
                qDebug() << "Non-numeric data line after S-parameters, stopping S-parameter read:" << line;
                break;
            } else {
                qDebug() << "Skipping non-numeric data line (and no data read yet):" << line;
                continue;
            }
        }

        QStringList values = line.split(QRegularExpression("\s+"), Qt::SkipEmptyParts);
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
                qWarning() << "Error: Could not determine number of ports for file:" << filePath << ". File extension was not sNp or first data line malformed. Line:" << line;
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

        for (int i = 1; i <= number_of_ports; ++i) {
            for (int j = 1; j <= number_of_ports; ++j) {
                QString s_param_mag_key = QString("S%1%2_dB").arg(i).arg(j);
                QString s_param_ang_key = QString("S%1%2_ang").arg(i).arg(j);
                QString s_param_re_key = QString("S%1%2_re").arg(i).arg(j);
                QString s_param_im_key = QString("S%1%2_im").arg(i).arg(j);

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
                        double s_re, s_im;
                        convert_MA_RI_to_dB(&val1, &val2, &s_re, &s_im, format_str);
                        file_data[s_param_mag_key].append(val1);
                        file_data[s_param_ang_key].append(val2);
                        file_data[s_param_re_key].append(s_re);
                        file_data[s_param_im_key].append(s_im);
                        qDebug() << QString("S%1%2: dB=").arg(i).arg(j) << val1 << "Ang=" << val2 << "Re=" << s_re << "Im=" << s_im;
                    }
                    current_val_idx += 2;
                } else {
                    qDebug() << "Incomplete data on line for S" << i << j << " - Appending NaN. Line:" << line;
                    file_data[s_param_mag_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_ang_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_re_key].append(std::numeric_limits<double>::quiet_NaN());
                    file_data[s_param_im_key].append(std::numeric_limits<double>::quiet_NaN());
                    if (current_val_idx < values.length()) current_val_idx++;
                }
            }
        }

        int s_params_read_on_line = (current_val_idx -1) / 2; // Number of pairs read
         if (s_params_read_on_line < expected_s_param_pairs) {
             qWarning() << "Warning: Data line seems incomplete or S-parameters span multiple lines. Expected"
                        << expected_s_param_pairs << "S-parameter pairs, processed" << s_params_read_on_line
                        << "from line:" << line;
         }
    }

    // Store the finally determined number of ports
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

void QucsTouchstoneViewer::displayData(const QMap<QString, QList<double>>& data)
{
    dataTable->clearContents();

    if (!data.contains("frequency") || data["frequency"].isEmpty()) {
        dataTable->setRowCount(0);
        if (data.isEmpty()) {
            // Message may have been shown by openFile if readTouchstoneFile returned empty
        } else {
             if (!(data.contains("frequency") && !data["frequency"].isEmpty())) { // Check specifically if frequency is the issue
                 QMessageBox::information(this, tr("Info"), tr("File parsed, but no valid frequency data points found."));
            } else { // This case should ideally not be reached if the outer condition is true
                 QMessageBox::information(this, tr("Info"), tr("No frequency data points found in the file."));
            }
        }
        return;
    }

    const QList<double>& freq = data["frequency"];
    int numRowsToShow = qMin(10, freq.size());
    dataTable->setRowCount(numRowsToShow);
    qDebug() << "Displaying" << numRowsToShow << "rows.";

    QStringList sParamIndices = {"11", "12", "21", "22"};

    for (int i = 0; i < numRowsToShow; ++i) {
        dataTable->setItem(i, 0, new QTableWidgetItem(QString::number(freq.at(i), 'g', 10)));

        for (int j = 0; j < sParamIndices.size(); ++j) {
            QString s_param_key_db = QString("S%1_dB").arg(sParamIndices.at(j));
            if (data.contains(s_param_key_db) && i < data[s_param_key_db].size()) {
                double val = data[s_param_key_db].at(i);
                dataTable->setItem(i, j + 1, new QTableWidgetItem(std::isnan(val) ? "NaN" : QString::number(val, 'f', 4)));
            } else {
                dataTable->setItem(i, j + 1, new QTableWidgetItem("N/A"));
            }
        }

        if (data.contains("Z0") && i < data["Z0"].size()) {
             dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data["Z0"].at(i), 'f', 2)));
        } else if (data.contains("Z0") && !data["Z0"].isEmpty()){
            dataTable->setItem(i, 5, new QTableWidgetItem(QString::number(data["Z0"].first(), 'f', 2)));
        } else {
            dataTable->setItem(i, 5, new QTableWidgetItem("50.00 (default)"));
        }
    }
}
