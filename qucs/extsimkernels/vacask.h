/***************************************************************************
                               vacask.h
                             ----------------
    begin                : Wed Jul 10 2024
    copyright            : (C) 2024
 ***************************************************************************/

#ifndef VACASK_H
#define VACASK_H

#include "ngspice.h"

class Vacask : public Ngspice
{
    Q_OBJECT

public:
    Vacask(Schematic* schematic, QObject* parent = nullptr);

protected:
    void startNetlist(QTextStream& stream,
                      spicecompat::SpiceDialect dialect = spicecompat::SPICEDefault) override;

private:
    static QString wrapNodes(const Component* component, const QString& raw);
};

#endif // VACASK_H

