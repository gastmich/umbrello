/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2002-2020                                               *
 *   Umbrello UML Modeller Authors <umbrello-devel@kde.org>                *
 ***************************************************************************/

#ifndef ACTOR_H
#define ACTOR_H

#include "umlcanvasobject.h"

/**
 * This class contains the non-graphical information required for a UML Actor.
 * This class inherits from @ref UMLCanvasObject which contains most of the
 * information.
 * The @ref UMLDoc class creates instances of this type.
 *
 * @short Information for a non-graphical UML Actor.
 * @author Paul Hensgen <phensgen@techie.com>
 * @see UMLCanvasObject
 * Bugs and comments to umbrello-devel@kde.org or https://bugs.kde.org
 */
class UMLActor : public UMLCanvasObject
{
    Q_OBJECT
public:
    explicit UMLActor(const QString & name = QString(), Uml::ID::Type id = Uml::ID::None);
    virtual ~UMLActor();

    virtual void init();

    virtual UMLObject* clone() const;

    void saveToXMI1(QDomDocument & qDoc, QDomElement & qElement);

protected:
    bool load1(QDomElement & element);
};

#endif
