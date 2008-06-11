/***************************************************************************
 * Copyright (C) 2008 by Gopala Krishna A <krishna.ggk@gmail.com>          *
 *                                                                         *
 * This is free software; you can redistribute it and/or modify            *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2, or (at your option)     *
 * any later version.                                                      *
 *                                                                         *
 * This software is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this package; see the file COPYING.  If not, write to        *
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,   *
 * Boston, MA 02110-1301, USA.                                             *
 ***************************************************************************/

#ifndef __NEWUMLWIDGET_H
#define __NEWUMLWIDGET_H

#include <QtCore/QObject>
#include <QtGui/QGraphicsItem>
#include <QtGui/QPen>
#include <QtGui/QBrush>
#include <QtGui/QFont>
#include <QtXml/QDomDocument>

#include "umlnamespace.h"

class UMLObject;
class UMLScene;
class WidgetInterfaceData;

/**
 * @short The base class for UML widgets that appear on UML diagrams.
 *
 * This class provides the common interface required for all the UML
 * widgets including rectangular and non rectangular widgets.
 * Rectangular widgets should use NewUmlRectWidget as its base.
 */
class NewUMLWidget : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    /**
     * Constructs a NewUMLWidget object with the associated UMLObject
     * to \a object and the parent item of the widget being null.
     *
     * @param object UMLObject that is represented by this
     *               widget. Pass null if this widget does not have
     *               UMLObject representation.
     *
     * @param parent The parent of the widget.
     *
     * @note The widget's visual properties are best to be set by the
     *       Widget_Factory::createWidget method.
     */
    explicit NewUMLWidget(UMLObject *object, QGraphicsItem *parent = 0);
    /**
     * Destructor
     */
    ~NewUMLWidget();

    /**
     * @return The UMLObject represented by this widget or null if
     * this widget has no UMLObject representation.
     */
    UMLObject* umlObject() const;
    /**
     * Set the UMLObject for this widget to represent.
     *
     * @todo Either remove this method, or allow it to only allow
     * specific users as making this method public violates
     * encapsulation.
     */
    void setUmlObject(UMLObject *obj);

    /**
     * If this widget represents a UMLObject, the id of that object is
     * returned. Else the id stored in this widget is returned.
     *
     * @return The identifier of this widget.
     */
    Uml::IDType id() const;
    /**
     * If this widget represents a UMLObject, the id of that object is
     * set to \a id. Else the id is stored in this widget.
     */
    void setId(Uml::IDType id);

    /**
     * @return The base type rtti info for this widget.
     */
    Uml::Widget_Type baseType() const;
    /**
     * Sets the base type for this widget.
     *
     * @todo Remove this method as it seems to violate encapsulation.
     */
    void setBaseType(Uml::Widget_Type type);

    /**
     * @return The UMLScene for this widget is returned, or 0 if the
     * widget is not stored in a scene.
     *
     * @note To add or remove widgets to scene, use
     *       UMLScene::addItem(widget)
     */
    UMLScene* umlScene() const;

    /**
     * If this widget represents a UMLObject, the documentation string
     * stored in the object is returned. Else the documentation string
     * stored in the widget is returned.
     *
     * @return A string representing the documentation for this widget
     *         which is usually set by the user.
     */
    QString documentation() const;
    /**
     * If this widget represents a UMLObject, the documentation string
     * of that object is set to \a doc. Else the documentation string
     * stored in the widget is set to \a doc.
     */
    void setDocumentation(const QString& doc);

    /**
     * If this widget represents a UMLObject, the name string stored
     * in the object is returned. Else the name string stored in the
     * widget is returned.
     *
     * @return A string representing the name for this widget
     *         which is usually set by the user.
     */
    QString name() const;
    /**
     * If this widget represents a UMLObject, the name string of that
     * object is set to \a name. Else the name string stored in the
     * widget is set to \a name.
     */
    void setName(const QString& doc);

    /**
     * @return The QPen object used to draw this widget.
     */
    QPen pen() const {
        return m_pen;
    }
    /**
     * Sets the QPen object of this widget to \a pen which is used to
     * draw this widget.
     *
     * This method implicitly calls updateGeometry() virtual method to
     * let the subclasses calculate its new bound rect based on the
     * new pen.
     */
    void setPen(const QPen& pen);

    /**
     * @return The QBrush object used to fill this widget.
     */
    QBrush brush() const {
        return m_brush;
    }
    /**
     * Sets the QBrush object of this widget to \a brush which is used
     * to fill this widget.
     *
     * This method implicitly calls update on this widget.
     */
    void setBrush(const QBrush& brush);

    /**
     * @return The font used for displaying any text inside this
     * widget.
     */
    QFont font() const {
        return m_font;
    }
    /**
     * Set the font used to display text inside this widget.
     *
     * This method implicitly updateGeometry() virtual method to let
     * the subclasses calculate its new bound rect based on the new
     * font.
     */
    void setFont(const QFont& font);


    /**
     * @return The bounding rectangle for this widget.
     * @see setBoundingRectAndShape
     */
    QRectF boundingRect() const {
        return m_boundingRect;
    }

    /**
     * @return The shape of this widget.
     * @see setBoundingRectAndShape
     */
    QPainterPath shape() const {
        return m_shape;
    }

    /**
     * A virtual method to load the properties of this widget from a
     * QDomElement into this widget.
     *
     * Subclasses should reimplement this to load addtional properties
     * required, calling this base method to load the basic properties
     * of the widget.
     *
     * @param qElement A QDomElement which contains xml info for this
     *                 widget.
     *
     * @todo Add support to load older version.
     */
    virtual bool loadFromXMI(QDomElement &qElement);
    /**
     * A virtual method to save the properties of this widget into a
     * QDomElement i.e xml.
     *
     * Subclasses should first create a new dedicated element as the
     * child of \a qElement parameter passed.
     * Then this base method should be called to save basic
     * widget properties.
     *
     * @param qDoc A QDomDocument object representing the xml document.
     * @oaram qElement A QDomElement representing xml element data.
     */
    virtual void saveToXMI(QDomDocument &qDoc, QDomElement &qElement);

protected Q_SLOTS:
    /**
     * This slot is connected to UMLObject::modified() signal to sync
     * the required changes as well as to update visual aspects
     * corresponding to the change.
     *
     * @note Subclasses can *reimplement* this slot to fine tune its
     * own updations.
     */
    virtual void slotUMLObjectDataChanged();


protected:
    /**
     * This virtual method is called by this NewUMLWidget base class
     * to notify subclasses about the need to change its boundingRect
     * Example. When a new pen is set on a widget which paints
     *
     * The default implementation just calls update()
     */
    virtual void updateGeometry();

    /**
     * This method sets the bounding rectangle and shape of this
     * widget to \a rect and \a path. These two objects are stored
     * locally in this widget for performance reasons. Also the
     * prepareGeometryChange() method is implicitly called to update
     * QGraphicsScene indexes.
     *
     * @param rect The bounding rectangle for this widget.
     *
     * @param path If not specified or set to empty path will add the
     *             boundingRect to the path and set that as the shape
     *             of this widget.
     *
     * @note The bounding rect being set should also be compensated
     *       with half pen width if the widget is painting an
     *       outline/stroke.
     *
     * @note Also note that, subclasses reimplementing "boundingRect()
     *       const" virtual method will not be affected by this method
     *       unless the subclassed widget explicitly uses it.
     *
     * @see Widget_Utils::adjustedBoundingRect
     */
    void setBoundingRectAndShape(const QRectF &rect,
                                 const QPainterPath& path = QPainterPath());


    UMLObject *m_umlObject;

    Uml::Widget_Type m_baseType;

    QPen m_pen;
    QBrush m_brush;
    QFont m_font;

    QRectF m_boundingRect;
    QPainterPath m_shape;

    WidgetInterfaceData *m_widgetInterfaceData;

private:
    /*
     * Disable the copy constructor and assignment operator.
     */
    DISABLE_COPY(NewUMLWidget)
};

#endif //__NEWUMLWIDGET_H
