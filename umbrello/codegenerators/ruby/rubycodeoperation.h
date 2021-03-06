/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2005                                                    *
 *   Richard Dale  <Richard_Dale@tipitina.demon.co.uk>                     *
 *   copyright (C) 2006-2020                                               *
 *   Umbrello UML Modeller Authors <umbrello-devel@kde.org>                *
 ***************************************************************************/

#ifndef RUBYCODEOPERATION_H
#define RUBYCODEOPERATION_H

#include "codeoperation.h"

#include <QString>

class RubyClassifierCodeDocument;

class RubyCodeOperation : virtual public CodeOperation
{
    Q_OBJECT
public:

    /**
     * Empty Constructor
     */
    RubyCodeOperation (RubyClassifierCodeDocument * doc, UMLOperation * op, const QString & body = QString(), const QString & comment = QString());

    /**
     * Empty Destructor
     */
    virtual ~RubyCodeOperation ();

    virtual int lastEditableLine();

protected:

    void updateMethodDeclaration();

};

#endif // RUBYCODEOPERATION_H
