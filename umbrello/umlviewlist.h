/***************************************************************************
                          umlviewlist.h  -  description
                             -------------------
    begin                : Sat Dec 29 2001
    copyright            : (C) 2001 by Gustavo Madrigal
    email                : gmadrigal@nextphere.com
  Bugs and comments to umbrello-devel@kde.org or https://bugs.kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef UMLVIEWLIST_H
#define UMLVIEWLIST_H

//#include "umlview.h"
#include <QList>
#include <QPointer>

class UMLView;

typedef QList<QPointer<UMLView>> UMLViewList;
typedef QListIterator<QPointer<UMLView>> UMLViewListIt;

#endif
