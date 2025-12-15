/***************************************************************************
                               vacask.cpp
                             ----------------
    begin                : Fri Jul 12 2024
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "vacask.h"

#include <QDebug>

/*!
  \file vacask.cpp
  \brief Implementation of the Vacask class
*/

Vacask::Vacask(Schematic* schematic, QObject *parent) :
    AbstractSpiceKernel(schematic, parent),
    a_spinit_name()
{
    qDebug() << "VACASK was launched";
}

void Vacask::createNetlist(
        QTextStream& stream,
        QStringList& simulations,
        QStringList& vars,
        QStringList& outputs)
{
    Q_UNUSED(stream);
    Q_UNUSED(simulations);
    Q_UNUSED(vars);
    Q_UNUSED(outputs);
}

QString Vacask::getParentSWPscript(Component *pc_swp, QString sim, bool before, bool &hasDblSWP)
{
    Q_UNUSED(pc_swp);
    Q_UNUSED(sim);
    Q_UNUSED(before);
    Q_UNUSED(hasDblSWP);
    return QString();
}

QString Vacask::getParentSWPCntVar(Component *pc_swp, QString sim)
{
    Q_UNUSED(pc_swp);
    Q_UNUSED(sim);
    return QString();
}

void Vacask::slotSimulate()
{
    // TODO: implement simulation orchestration
}

bool Vacask::checkNodeNames(QStringList &incompat)
{
    Q_UNUSED(incompat);
    return false;
}

bool Vacask::findMathFuncInc(QString &mathf_inc)
{
    Q_UNUSED(mathf_inc);
    return false;
}

QString Vacask::collectSpiceinit(Schematic* sch)
{
    Q_UNUSED(sch);
    return QString();
}

void Vacask::cleanSpiceinit()
{
}

void Vacask::createSpiceinit(const QString &initial_spiceinit)
{
    Q_UNUSED(initial_spiceinit);
}

void Vacask::setSimulatorCmd(QString cmd)
{
    Q_UNUSED(cmd);
}

void Vacask::setSimulatorParameters(QString parameters)
{
    Q_UNUSED(parameters);
}

void Vacask::slotProcessOutput()
{
}

void Vacask::SaveNetlist(QString filename, bool netlist2Console)
{
    Q_UNUSED(filename);
    Q_UNUSED(netlist2Console);
}

