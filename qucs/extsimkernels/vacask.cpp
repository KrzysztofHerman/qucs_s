/***************************************************************************
                               vacask.cpp
 ***************************************************************************/

#include "vacask.h"

#include "components/component.h"
#include "node.h"
#include "wire.h"
#include "settings.h"

#include <algorithm>

#include <QRegularExpression>
#include <QFileInfo>

Vacask::Vacask(Schematic* schematic, QObject* parent)
    : Ngspice(schematic, parent)
{
    if (QFileInfo(QucsSettings.VacaskExecutable).isRelative()) {
        a_simulator_cmd = QFileInfo(QucsSettings.BinDir + QucsSettings.VacaskExecutable).absoluteFilePath();
    } else {
        a_simulator_cmd = QFileInfo(QucsSettings.VacaskExecutable).absoluteFilePath();
    }
    if (!QFileInfo::exists(a_simulator_cmd)) {
        a_simulator_cmd = QucsSettings.VacaskExecutable;
    }
}

void Vacask::startNetlist(QTextStream& stream, spicecompat::SpiceDialect dialect)
{
    QString s;

    // User-defined functions
    for(Component *pc : a_schematic->a_DocComps) {
        if ((pc->SpiceModel==".FUNC")||
            (pc->SpiceModel=="INCLSCR")) {
            s = pc->getExpression();
            stream<<s;
        }
    }

    // create .IC from wire labels
    QStringList wire_labels;
    for(Wire *pw : a_schematic->a_DocWires) {
        if (pw->hasLabel()) {
            QString label = pw->label()->Name;
            if (!wire_labels.contains(label)) wire_labels.append(label);
            else continue;
            QString ic = pw->label()->initValue;
            if (!ic.isEmpty()) {
                QString ic_str = QStringLiteral(".IC v(%1)=%2\n").arg(label).arg(ic);
                stream<<ic_str;
            }
        }
    }
    for(Node *pn : a_schematic->a_DocNodes) {
        Conductor *pw = (Conductor*) pn;
        if (pw->hasLabel()) {
            QString label = pw->label()->Name;
            if (!wire_labels.contains(label)) wire_labels.append(label);
            else continue;
            QString ic = pw->label()->initValue;
            if (!ic.isEmpty()) {
                QString ic_str = QStringLiteral(".IC v(%1)=%2\n").arg(label).arg(ic);
                stream<<ic_str;
            }
        }
    }

    // Parameters, Initial conditions, Options
    for(Component *pc : a_schematic->a_DocComps) {
        if (pc->isEquation) {
            s = pc->getExpression(dialect);
            stream<<s;
        }
    }

    // Components
    for(Component *pc : a_schematic->a_DocComps) {
      if(a_schematic->getIsAnalog() &&
         !(pc->isSimulation) &&
         !(pc->isEquation)) {
        s = wrapNodes(pc, pc->getSpiceNetlist(dialect));
        stream<<s;
      }
    }

    // Modelcards
    for(Component *pc : a_schematic->a_DocComps) {
        if (pc->SpiceModel==".MODEL") {
            s = pc->getSpiceModel();
            stream<<s;
        }
    }
}

QString Vacask::wrapNodes(const Component* component, const QString& raw)
{
    if (!component || component->Ports.isEmpty()) {
        return raw;
    }

    QStringList lines = raw.split('\n');
    for (QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        if (trimmed.startsWith('.') || trimmed.startsWith('*') || trimmed.startsWith('+')) {
            continue;
        }

        QStringList tokens = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (tokens.size() < 2) {
            continue;
        }

        int nodeCount = component->Ports.count();
        nodeCount = std::min(nodeCount, tokens.size() - 1);
        QStringList nodeTokens = tokens.mid(1, nodeCount);
        QStringList tailTokens = tokens.mid(1 + nodeCount);

        QString rebuilt = tokens.at(0) + QStringLiteral(" (") + nodeTokens.join(' ') + QStringLiteral(")");
        if (!tailTokens.isEmpty()) {
            rebuilt += QStringLiteral(" ") + tailTokens.join(' ');
        }

        QString leading = line.left(line.indexOf(trimmed));
        line = leading + rebuilt;
    }

    QString result = lines.join('\n');
    if (raw.endsWith('\n') && !result.endsWith('\n')) {
        result.append('\n');
    }
    return result;
}

