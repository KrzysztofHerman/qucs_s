/***************************************************************************
                          R_SPICE.h  -  description
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
 #ifndef R_IHP_H
#define R_IHP_H

#include "components/component.h"

class R_IHP : public Component {
public:
  R_IHP();
  ~R_IHP();
  Component* newOne();
  static Element* info(QString&, char* &, bool getNewOne=false);
  static Element* info_Rsil(QString&, char* &, bool getNewOne=false);
  static Element* info_Rppd(QString&, char* &, bool getNewOne=false);
  static Element* info_Rptap1(QString&, char* &, bool getNewOne=false);
  static Element* info_Rntap1(QString&, char* &, bool getNewOne=false);
protected:
  QString netlist();
  QString spice_netlist(bool isXyce = false);
};

#endif // R_SPICE_H
