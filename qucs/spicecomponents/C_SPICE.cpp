/***************************************************************************
                         C_SPICE.cpp  -  description
                   --------------------------------------
    begin                    : Fri Mar 9 2007
    copyright              : (C) 2007 by Gunther Kraut
    email                     : gn.kraut@t-online.de
    spice4qucs code added  Fri. 3 April 2015
    copyright              : (C) 2015 by Mike Brinson
    email                    : mbrin72043@yahoo.co.uk

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "C_SPICE.h"
#include "node.h"
#include "misc.h"
#include "extsimkernels/spicecompat.h"


C_SPICE::C_SPICE()
{
    Description = QObject::tr("SPICE C:\nMultiple line ngspice or Xyce C specifications allowed using \"+\" continuation lines.\nLeave continuation lines blank when NOT in use.");
    Simulator = spicecompat::simSpice;

    Lines.append(new qucs::Line( -4,-11, -4, 11,QPen(Qt::darkRed,3)));
    Lines.append(new qucs::Line(  4,-11,  4, 11, QPen(Qt::darkRed,3)));
    Lines.append(new qucs::Line(-30,  0, -4,  0,  QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line(  4,  0, 30,  0,   QPen(Qt::darkBlue,2)));
    

    Ports.append(new Port( 30,  0));
    Ports.append(new Port(-30,  0));

    x1 = -30; y1 = -13;
    x2 =  30; y2 =  13;

    tx = x1+4;
    ty = y2+4;
    Model = "C_SPICE";
    SpiceModel = "C";
    Name  = "C";

    Props.append(new Property("model", "", true,"modelname"));
    Props.append(new Property("Letter", "X", false,"[C,X]"));
    Props.append(new Property("W", "10u", true,"Width"));
    Props.append(new Property("L", "10u", true,"Length"));
// Here calculate an initial value based on resistor type and its W and L
    Props.append(new Property("C", "value", true,"Capacitance value"));

    rotate();  // fix historical flaw
}

C_SPICE::~C_SPICE()
{
}

Component* C_SPICE::newOne()
{
  return new C_SPICE();
}

Element* C_SPICE::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("C Capacitor");
  BitmapFile = (char *) "C_SPICE";

  if(getNewOne)  return new C_SPICE();
  return 0;
}

QString C_SPICE::netlist()
{
    return QString("");
}

QString C_SPICE::spice_netlist(bool)
{
    QString ltr =getProperty("Letter")->Value;
    QString s = spicecompat::check_refdes(Name,ltr);
    for (Port *p1 : Ports) {
        QString nam = p1->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam+" ";   // node names
    }

    QString C = Props.at(0)->Value;
    QString W = getProperty("W")->Value;
    QString L = getProperty("L")->Value;

    if(  C.length()  > 0)          s += QString("%1 ").arg(C);
    if(  W.length() > 0 )   s += QString("W=%1 ").arg(W);
    if(  L.length() > 0 )   s += QString("L=%1 ").arg(L);
    s += "\n";

    return s;
}
