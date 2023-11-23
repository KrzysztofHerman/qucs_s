/***************************************************************************
                         R_SPICE.cpp  -  description
                   --------------------------------------
    begin                    : Fri Mar 9 2007
    copyright              : (C) 2007 by Gunther Kraut
    email                     : gn.kraut@t-online.de
    spice4qucs code added  Sun. 5 April 2015
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
#include "R_IHP.h"
#include "components/component.h"
#include "node.h"
#include "misc.h"
#include "extsimkernels/spicecompat.h"


R_IHP::R_IHP()
{
    Description = QObject::tr("SPICE R:\nMultiple line ngspice or Xyce R specifications allowed using \"+\" continuation lines.\nLeave continuation lines blank when NOT in use.  ");
    Simulator = spicecompat::simSpice;

    Lines.append(new qucs::Line(-18, -9, 18, -9,QPen(Qt::darkRed,3)));
    Lines.append(new qucs::Line( 18, -9, 18,  9,QPen(Qt::darkRed,3)));
    Lines.append(new qucs::Line( 18,  9,-18,  9,QPen(Qt::darkRed,3)));
    Lines.append(new qucs::Line(-18,  9,-18, -9,QPen(Qt::darkRed,3)));
    Lines.append(new qucs::Line(-30,  0,-18,  0,QPen(Qt::darkBlue,2)));
    Lines.append(new qucs::Line( 18,  0, 30,  0,QPen(Qt::darkBlue,2)));
    
    Ports.append(new Port(-30,  0));
    Ports.append(new Port( 30,  0));

    x1 = -30; y1 = -11;
    x2 =  30; y2 =  11;

    tx = x1+4;
    ty = y2+4;

    Model = "R_IHP";
    SpiceModel = "R";
    Name  = "R";

    Props.append(new Property("model", "rsil", true,"[rsil,rppd,rhigh,ptap,ntap]"));
    Props.append(new Property("Letter", "X", false,"[R,X]"));
    Props.append(new Property("W", "1u", true,"Width"));
    Props.append(new Property("L", "10u", true,"Length"));
// Here calculate an initial value based on resistor type and its W and L
    Props.append(new Property("R", "value", true,"Resistance value"));



    rotate();  // fix historical flaw
}

R_IHP::~R_IHP()
{
}

Component* R_IHP::newOne()
{
  return new R_IHP();
}

Element* R_IHP::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr(" R Resistor");
  BitmapFile = (char *) "R_SPICE";

  if(getNewOne)  return new R_IHP();
  return 0;
}


QString R_IHP::netlist()
{
    return QString("");
}

QString R_IHP::spice_netlist(bool)
{
    QString ltr =getProperty("Letter")->Value;
    QString s = spicecompat::check_refdes(Name,ltr);
    for (Port *p1 : Ports) {
        QString nam = p1->Connection->Name;
        if (nam=="gnd") nam = "0";
        s += " "+ nam+" ";   // node names
    }

    QString R = Props.at(0)->Value;
    QString W = getProperty("W")->Value;
    QString L = getProperty("L")->Value;

    if(  R.length()  > 0)          s += QString("%1 ").arg(R);
    if(  W.length() > 0 )   s += QString("W=%1 ").arg(W);
    if(  L.length() > 0 )   s += QString("L=%1 ").arg(L);
    s += "\n";

    return s;
}
