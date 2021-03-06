/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2003-2020                                               *
 *   Umbrello UML Modeller Authors <umbrello-devel@kde.org>                *
 ***************************************************************************/

#ifndef PACKAGE_H
#define PACKAGE_H

#include "umlcanvasobject.h"
#include "umlclassifierlist.h"
#include "umlentitylist.h"

// forward declarations
class UMLAssociation;

/**
 * This class contains the non-graphical information required for a UML
 * Package.
 * This class inherits from @ref UMLCanvasObject which contains most of the
 * information.
 *
 * @short Non-graphical information for a Package.
 * @author Jonathan Riddell
 * @see UMLCanvasObject
 * Bugs and comments to umbrello-devel@kde.org or https://bugs.kde.org
 */
class UMLPackage : public UMLCanvasObject
{
    Q_OBJECT
public:
    explicit UMLPackage(const QString & name = QString(), Uml::ID::Type id = Uml::ID::None);
    virtual ~UMLPackage();

    virtual void copyInto(UMLObject *lhs) const;

    virtual UMLObject* clone() const;

    bool addObject(UMLObject *pObject);
    void removeObject(UMLObject *pObject);

    virtual void removeAllObjects();

    UMLObjectList &containedObjects();

    void addAssocToConcepts(UMLAssociation* assoc);
    void removeAssocFromConcepts(UMLAssociation *assoc);

    UMLObject * findObject(const QString &name);
    UMLObject * findObjectById(Uml::ID::Type id);

    void appendPackages(UMLPackageList& packages, bool includeNested = true);
    void appendClassifiers(UMLClassifierList& classifiers,
                            bool includeNested = true);
    void appendClassesAndInterfaces(UMLClassifierList& classifiers,
                                    bool includeNested = true);
    void appendEntities(UMLEntityList& entities,
                        bool includeNested = true);

    virtual bool resolveRef();

    virtual void saveToXMI1(QDomDocument& qDoc, QDomElement& qElement);

protected:
    virtual bool load1(QDomElement& element);

    /**
     * References to the objects contained in this package.
     * The UMLPackage is the owner of the objects.
     */
    UMLObjectList m_objects;

};

#endif
