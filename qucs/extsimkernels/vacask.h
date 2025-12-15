/***************************************************************************
                           vacask.h
                             ----------------
    begin                : Fri Jul 12 2024
    copyright            : (C) 2024
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef VACASK_H
#define VACASK_H

#include <QString>
#include <QStringList>
#include <QDataStream>
#include "schematic.h"
#include "abstractspicekernel.h"

/*!
  \file vacask.h
  \brief Declaration of the Vacask class
*/

/*!
 * \brief The Vacask class provides a VACASK simulator skeleton.
 */
class Vacask : public AbstractSpiceKernel
{
    Q_OBJECT

private:
    QString a_spinit_name;

    bool checkNodeNames(QStringList &incompat);
    static QString collectSpiceinit(Schematic* sch);
    bool findMathFuncInc(QString &mathf_inc);
    QString getParentSWPscript(Component *pc_swp, QString sim, bool before, bool &hasDblSWP);
    QString getParentSWPCntVar(Component *pc_swp, QString sim);
    void cleanSpiceinit();
    void createSpiceinit(const QString &initial_spiceinit);

public:
    explicit Vacask(Schematic* schematic, QObject *parent = nullptr);
    void SaveNetlist(QString filename, bool netlist2Console);
    void setSimulatorCmd(QString cmd);
    void setSimulatorParameters(QString parameters);

protected:
    void createNetlist(
            QTextStream& stream,
            QStringList& simulations,
            QStringList& vars,
            QStringList& outputs);

public slots:
    void slotSimulate();

protected slots:
    void slotProcessOutput();
};

#endif // VACASK_H

