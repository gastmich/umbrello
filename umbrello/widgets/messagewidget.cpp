/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2002-2012                                               *
 *   Umbrello UML Modeller Authors <uml-devel@uml.sf.net>                  *
 ***************************************************************************/

// onw header
#include "messagewidget.h"

//app includes
#include "classifier.h"
#include "debug_utils.h"
#include "floatingtextwidget.h"
#include "messagewidgetcontroller.h"
#include "listpopupmenu.h"
#include "objectwidget.h"
#include "operation.h"
#include "uml.h"
#include "umldoc.h"
#include "umlview.h"
#include "uniqueid.h"

//qt includes
#include <QPolygon>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QtGui/QPainter>

//kde includes
#include <klocale.h>

/**
 * Constructs a MessageWidget.
 *
 * @param scene   The parent to this class.
 * @param a       The role A widget for this message.
 * @param b       The role B widget for this message.
 * @param y       The vertical position to display this message.
 * @param sequenceMessageType Whether synchronous or asynchronous
 * @param id      A unique id used for deleting this object cleanly.
 *                The default (-1) will prompt generation of a new ID.
 */
MessageWidget::MessageWidget(UMLScene * scene, ObjectWidget* a, ObjectWidget* b,
                             int y, Uml::Sequence_Message_Type sequenceMessageType,
                             Uml::IDType id /* = Uml::id_None */)
  : UMLWidget(scene, WidgetBase::wt_Message, id, new MessageWidgetController(this))
{
    init();
    m_pOw[Uml::A] = a;
    m_pOw[Uml::B] = b;
    m_nY = y;
    m_sequenceMessageType = sequenceMessageType;
    if (m_sequenceMessageType == Uml::sequence_message_creation) {
        y -= m_pOw[Uml::B]->height() / 2;
        m_pOw[Uml::B]->setY(y);
    }
    updateResizability();
    calculateWidget();
    y = y < getMinY() ? getMinY() : y;
    y = y > getMaxY() ? getMaxY() : y;
    m_nY = y;

    this->activate();
}

/**
 * Constructs a MessageWidget.
 *
 * @param scene              The parent to this class.
 * @param sequenceMessageType The Uml::Sequence_Message_Type of this message widget
 * @param id                The ID to assign (-1 will prompt a new ID.)
 */
MessageWidget::MessageWidget(UMLScene * scene, Uml::Sequence_Message_Type seqMsgType,
                             Uml::IDType id)
  : UMLWidget(scene, WidgetBase::wt_Message, id, new MessageWidgetController(this))
{
    init();
    m_sequenceMessageType = seqMsgType;
}

/**
 * Constructs a Lost or Found MessageWidget.
 *
 * @param scene  The parent to this class.
 * @param a      The role A widget for this message.
 * @param xclick The horizontal position clicked by the user
 * @param yclick The vertical position clicked by the user
 * @param sequenceMessageType Whether lost or found
 * @param id     The ID to assign (-1 will prompt a new ID.)
 */
MessageWidget::MessageWidget(UMLScene * scene, ObjectWidget* a, int xclick, int yclick,
                             Uml::Sequence_Message_Type sequenceMessageType,
                             Uml::IDType id /*= Uml::id_None*/)
  : UMLWidget(scene, WidgetBase::wt_Message, id, new MessageWidgetController(this))
{
    init();
    m_pOw[Uml::A] = a;
    m_pOw[Uml::B] = a;

    m_sequenceMessageType = sequenceMessageType;
    m_nY = yclick;

    xclicked = xclick;
    yclicked = yclick;
    m_nY = yclick;

    updateResizability();
    calculateWidget();
    yclick = yclick < getMinY() ? getMinY() : yclick;
    yclick = yclick > getMaxY() ? getMaxY() : yclick;
    m_nY = yclick;
    yclicked = yclick;

    this->activate();
}

/**
 * Initializes key variables of the class.
 */
void MessageWidget::init()
{
    m_ignoreSnapToGrid = true;
    m_ignoreSnapComponentSizeToGrid = true;
    m_pOw[Uml::A] = m_pOw[Uml::B] = NULL;
    m_pFText = NULL;
    m_nY = 0;
    setVisible(true);
}

/**
 * Standard destructor.
 */
MessageWidget::~MessageWidget()
{
}

/**
 * Update the UMLWidget::m_resizable flag according to the
 * charactersitics of this message.
 */
void MessageWidget::updateResizability()
{
    if (m_sequenceMessageType == Uml::sequence_message_synchronous ||
        m_pOw[Uml::A] == m_pOw[Uml::B])
        UMLWidget::m_resizable = true;
    else
        UMLWidget::m_resizable = false;
}

/**
 * Calls drawSynchronous() or drawAsynchronous().
 */
void MessageWidget::paint(QPainter& p, int offsetX, int offsetY)
{
    if(!m_pOw[Uml::A] || !m_pOw[Uml::B]) {
        return;
    }
    setPenFromSettings(p);
    if (m_sequenceMessageType == Uml::sequence_message_synchronous) {
        drawSynchronous(p, offsetX, offsetY);
    } else if (m_sequenceMessageType == Uml::sequence_message_asynchronous) {
        drawAsynchronous(p, offsetX, offsetY);
    } else if (m_sequenceMessageType == Uml::sequence_message_creation) {
        drawCreation(p, offsetX, offsetY);
    } else if (m_sequenceMessageType == Uml::sequence_message_lost) {
        drawLost(p, offsetX, offsetY);
    } else if (m_sequenceMessageType == Uml::sequence_message_found) {
        drawFound(p, offsetX, offsetY);
    } else {
        uWarning() << "Unknown message type";
    }
}

/**
 * Draw a solid (triangular) arrowhead pointing in the given direction.
 * The direction can be either Qt::LeftArrow or Qt::RightArrow.
 */
void MessageWidget::drawSolidArrowhead(QPainter& p, int x, int y, Qt::ArrowType direction)
{
    int arrowheadExtentX = 4;
    if (direction == Qt::RightArrow) {
        arrowheadExtentX = -arrowheadExtentX;
    }
    QPolygon points;
    points.putPoints(0, 3, x, y, x + arrowheadExtentX, y - 3, x + arrowheadExtentX, y + 3);
    p.setBrush( QBrush(p.pen().color()) );
    p.drawPolygon(points);
}

/**
 * Draw an arrow pointing in the given direction.
 * The arrow head is not solid, i.e. it is made up of two lines
 * like so:  --->
 * The direction can be either Qt::LeftArrow or Qt::RightArrow.
 */
void MessageWidget::drawArrow(QPainter& p, int x, int y, int w,
                              Qt::ArrowType direction, bool useDottedLine /* = false */)
{
    int arrowheadStartX = x;
    int arrowheadExtentX = 4;
    if (direction == Qt::RightArrow) {
        arrowheadStartX += w;
        arrowheadExtentX = -arrowheadExtentX;
    }
    // draw upper half of arrowhead
    p.drawLine(arrowheadStartX, y, arrowheadStartX + arrowheadExtentX, y - 3);
    // draw lower half of arrowhead
    p.drawLine(arrowheadStartX, y, arrowheadStartX + arrowheadExtentX, y + 3);
    // draw arrow line
    if (useDottedLine) {
        QPen pen = p.pen();
        pen.setStyle(Qt::DotLine);
        p.setPen(pen);
    }
    p.drawLine(x, y, x + w, y);
}

/**
 * Draws the calling arrow with filled in arrowhead, the
 * timeline box and the returning arrow with a dashed line and
 * stick arrowhead.
 */
void MessageWidget::drawSynchronous(QPainter& p, int offsetX, int offsetY)
{
    int x1 = m_pOw[Uml::A]->x();
    int x2 = m_pOw[Uml::B]->x();
    int w = width() - 1;
    int h = height();

    bool messageOverlaps = m_pOw[Uml::A] -> messageOverlap( y(), this );

    if(m_pOw[Uml::A] == m_pOw[Uml::B]) {
        p.fillRect( offsetX, offsetY, 17, h,  QBrush(Qt::white) );              //box
        p.drawRect(offsetX, offsetY, 17, h);                                    //box
        offsetX += 17;
        w -= 17;
        offsetY += 3;
        const int lowerLineY = offsetY + h - 6;
        // draw upper line segment (leaving the life line)
        p.drawLine(offsetX, offsetY, offsetX + w, offsetY);
        // draw line segment parallel to (and at the right of) the life line
        p.drawLine(offsetX + w, offsetY, offsetX + w, lowerLineY);
        // draw lower line segment (back to the life line)
        drawArrow(p, offsetX, lowerLineY, w, Qt::LeftArrow);
        offsetX -= 17;
        offsetY -= 3;
    } else if(x1 < x2) {
        if (messageOverlaps)  {
            offsetX += 8;
            w -= 8;
        }
        QPen pen = p.pen();
        int startX = offsetX + w - 16;
        p.fillRect(startX, offsetY, 17, h,  QBrush(Qt::white));         //box
        p.drawRect(startX, offsetY, 17, h);                             //box
        p.drawLine(offsetX, offsetY + 4, startX, offsetY + 4);          //arrow line
        drawSolidArrowhead(p, startX - 1, offsetY + 4, Qt::RightArrow);
        drawArrow(p, offsetX, offsetY + h - 3, w - 16, Qt::LeftArrow, true); // return arrow
        if (messageOverlaps)  {
            offsetX -= 8; //reset for drawSelected()
        }
    } else      {
        if (messageOverlaps)  {
            w -=8;
        }
        QPen pen = p.pen();
        p.fillRect( offsetX, offsetY, 17, h,  QBrush(Qt::white) );              //box
        p.drawRect(offsetX, offsetY, 17, h);                                    //box
        p.drawLine(offsetX + 18, offsetY + 4, offsetX + w, offsetY + 4);        //arrow line
        drawSolidArrowhead(p, offsetX + 17, offsetY + 4, Qt::LeftArrow);
        drawArrow(p, offsetX + 18, offsetY + h - 3, w - 18, Qt::RightArrow, true); // return arrow
    }

    if(m_selected) {
        drawSelected(&p, offsetX, offsetY);
    }
}

/**
 * Draws a solid arrow line and a stick arrow head.
 */
void MessageWidget::drawAsynchronous(QPainter& p, int offsetX, int offsetY)
{
    int x1 = m_pOw[Uml::A]->x();
    int x2 = m_pOw[Uml::B]->x();
    int w = width() - 1;
    int h = height() - 1;
    bool messageOverlapsA = m_pOw[Uml::A] -> messageOverlap( y(), this );
    //bool messageOverlapsB = m_pOw[Uml::B] -> messageOverlap( y(), this );

    if(m_pOw[Uml::A] == m_pOw[Uml::B]) {
        if (messageOverlapsA)  {
            offsetX += 7;
            w -= 7;
        }
        const int lowerLineY = offsetY + h - 3;
        // draw upper line segment (leaving the life line)
        p.drawLine(offsetX, offsetY, offsetX + w, offsetY);
        // draw line segment parallel to (and at the right of) the life line
        p.drawLine(offsetX + w, offsetY, offsetX + w, lowerLineY);
        // draw lower line segment (back to the life line)
        drawArrow(p, offsetX, lowerLineY, w, Qt::LeftArrow);
        if (messageOverlapsA)  {
            offsetX -= 7; //reset for drawSelected()
        }
    } else if(x1 < x2) {
        if (messageOverlapsA)  {
            offsetX += 7;
            w -= 7;
        }
        drawArrow(p, offsetX, offsetY + 4, w, Qt::RightArrow);
        if (messageOverlapsA)  {
            offsetX -= 7;
        }
    } else      {
        if (messageOverlapsA)  {
            w -= 7;
        }
        drawArrow(p, offsetX, offsetY + 4, w, Qt::LeftArrow);
    }

    if (m_selected)
        drawSelected(&p, offsetX, offsetY);
}

/**
 * Draws a solid arrow line and a stick arrow head to the
 * edge of the target object widget instead of to the
 * sequence line.
 */
void MessageWidget::drawCreation(QPainter& p, int offsetX, int offsetY)
{
    int x1 = m_pOw[Uml::A]->x();
    int x2 = m_pOw[Uml::B]->x();
    int w = width() - 1;
    //int h = height() - 1;
    bool messageOverlapsA = m_pOw[Uml::A] -> messageOverlap( y(), this );
    //bool messageOverlapsB = m_pOw[Uml::B] -> messageOverlap( y(), this );

    const int lineY = offsetY + 4;
    if (x1 < x2) {
        if (messageOverlapsA)  {
            offsetX += 7;
            w -= 7;
        }
        drawArrow(p, offsetX, lineY, w, Qt::RightArrow);
        if (messageOverlapsA)  {
            offsetX -= 7;
        }
    } else      {
        if (messageOverlapsA)  {
            w -= 7;
        }
        drawArrow(p, offsetX, lineY, w, Qt::LeftArrow);
    }

    if (m_selected)
        drawSelected(&p, offsetX, offsetY);
}


/**
 * Draws a solid arrow line and a stick arrow head
 * and a circle
 */
void MessageWidget::drawLost(QPainter& p, int offsetX, int offsetY)
{
    int x1 = m_pOw[Uml::A]->x();
    int x2 = xclicked;
    int w1 = m_pOw[Uml::A]->width() / 2;
    x1 += w1;

    int w = width() ;

    int h = 10;
    bool messageOverlapsA = m_pOw[Uml::A] -> messageOverlap( y(), this );
    //bool messageOverlapsB = m_pOw[Uml::B] -> messageOverlap( y(), this );

    if(x1 < x2) {
        if (messageOverlapsA)  {
            offsetX += 7;
            w -= 7;
        }

        setPenFromSettings(p);
        p.setBrush( WidgetBase::lineColor() );
        p.drawEllipse(x1 + w - h , offsetY - h/2, h, h);
        drawArrow(p,offsetX, offsetY, w - h, Qt::RightArrow);

        if (messageOverlapsA)  {
            offsetX -= 7;
        }
    } else      {
        setPenFromSettings(p);
        p.setBrush( WidgetBase::lineColor() );
        p.drawEllipse(offsetX, offsetY - h/2, h, h);
        drawArrow(p, offsetX + h, offsetY, w - h, Qt::LeftArrow);
    }

    if (m_selected)
        drawSelected(&p, offsetX, offsetY);
}

/**
 * Draws a circle and a solid arrow line and a stick arrow head.
 */
void MessageWidget::drawFound(QPainter& p, int offsetX, int offsetY)
{
    int x1 = m_pOw[Uml::A]->x();
    int x2 = xclicked;
    int w = width() ;

    int h = 10;
    bool messageOverlapsA = m_pOw[Uml::A] -> messageOverlap( y(), this );
    //bool messageOverlapsB = m_pOw[Uml::B] -> messageOverlap( y(), this );

    if(x1 < x2) {
        if (messageOverlapsA)  {
            offsetX += 7;
            w -= 7;
        }
        setPenFromSettings(p);
        p.setBrush( WidgetBase::lineColor() );
        p.drawEllipse(x2, offsetY - h/2, h, h);
        drawArrow(p, x2 - w + h, offsetY, w, Qt::LeftArrow);
        if (messageOverlapsA)  {
            offsetX -= 7;
        }
    } else {
        if (messageOverlapsA)  {
            w -= 7;
        }
        setPenFromSettings(p);
        p.setBrush( WidgetBase::lineColor() );
        p.drawEllipse(x2, offsetY - h/2, h, h);
        drawArrow(p, x2, offsetY, w, Qt::RightArrow);
    }

    if (m_selected)
            drawSelected(&p, offsetX, offsetY);

}

/**
 * Overrides operation from UMLWidget.
 *
 * @param p Point to be checked.
 *
 * @return Non-zero if the point is on a part of the MessageWidget.
 *         NB In case of a synchronous message, the empty space
 *         between call line and return line does not count, i.e. if
 *         the point is located in that space the function returns 0.
 */
int MessageWidget::onWidget(const QPoint & p)
{
    if (m_sequenceMessageType != Uml::sequence_message_synchronous) {
        return UMLWidget::onWidget(p);
    }
    // Synchronous message:
    // Consists of top arrow (call) and bottom arrow (return.)
    if (p.x() < x() || p.x() > x() + width())
        return 0;
    const int tolerance = 5;  // pixels
    const int pY = p.y();
    const int topArrowY = y() + 3;
    const int bottomArrowY = y() + height() - 3;
    if (pY < topArrowY - tolerance || pY > bottomArrowY + tolerance)
        return 0;
    if (height() <= 2 * tolerance)
        return 1;
    if (pY > topArrowY + tolerance && pY < bottomArrowY - tolerance)
        return 0;
    return 1;
}

/**
 * Sets the text position relative to the sequence message.
 */
void MessageWidget::setTextPosition()
{
    if (m_pFText == NULL) {
        uDebug() << "m_pFText is NULL";
        return;
    }
    if (m_pFText->displayText().isEmpty()) {
        return;
    }
    m_pFText->updateGeometry();
    int ftX = constrainX(m_pFText->x(), m_pFText->width(), m_pFText->textRole());
    int ftY = y() - m_pFText->height();
    m_pFText->setX( ftX );
    m_pFText->setY( ftY );
}

/**
 * Returns the textX arg with constraints applied.
 * Auxiliary to setTextPosition() and constrainTextPos().
 */
int MessageWidget::constrainX(int textX, int textWidth, Uml::TextRole tr)
{
    int result = textX;
    const int minTextX = x() + 5;
    if (textX < minTextX || tr == Uml::TextRole::Seq_Message_Self) {
        result = minTextX;
    } else {
        ObjectWidget *objectAtRight = NULL;
        if (m_pOw[Uml::B]->x() > m_pOw[Uml::A]->x())
            objectAtRight = m_pOw[Uml::B];
        else
            objectAtRight = m_pOw[Uml::A];
        const int objRight_seqLineX = objectAtRight->x() + objectAtRight->width() / 2;
        const int maxTextX = objRight_seqLineX - textWidth - 5;
        if (maxTextX <= minTextX)
            result = minTextX;
        else if (textX > maxTextX)
            result = maxTextX;
    }
    return result;
}

/**
 * Constrains the FloatingTextWidget X and Y values supplied.
 * Overrides operation from LinkWidget.
 *
 * @param textX        candidate X value (may be modified by the constraint)
 * @param textY        candidate Y value (may be modified by the constraint)
 * @param textWidth    width of the text
 * @param textHeight   height of the text
 * @param tr           Uml::TextRole of the text
 */
void MessageWidget::constrainTextPos(UMLSceneValue &textX, UMLSceneValue &textY, UMLSceneValue textWidth, UMLSceneValue textHeight,
                                     Uml::TextRole tr)
{
    textX = constrainX(textX, textWidth, tr);
    // Constrain Y.
    const UMLSceneValue minTextY = getMinY();
    const UMLSceneValue maxTextY = getMaxY() - textHeight - 5;
    if (textY < minTextY)
        textY = minTextY;
    else if (textY > maxTextY)
        textY = maxTextY;
//     setY( textY + textHeight );   // NB: side effect
}

/**
 * Shortcut for calling m_pFText->setLink() followed by
 * this->setTextPosition().
 */
void MessageWidget::setLinkAndTextPos()
{
    if (m_pFText) {
        m_pFText->setLink(this);
        setTextPosition();
    }
}

void MessageWidget::moveEvent(QMoveEvent* /*m*/)
{
    //uDebug() << "m_pFText is " << m_pFText;
    if (!m_pFText) {
        return;
    }
    //TODO why this condition?
/*    if (m_scene->selectedCount() > 2) {
        return;
    }*/

    setTextPosition();

    emit sigMessageMoved();
}

void MessageWidget::resizeEvent(QResizeEvent* /*re*/)
{
}

/**
 * Calculate the geometry of the widget.
 */
void MessageWidget::calculateWidget()
{
    setMessageText(m_pFText);
    calculateDimensions();

    setVisible(true);

    setX(m_nPosX);
    setY(m_nY);
}

void MessageWidget::slotWidgetMoved(Uml::IDType id)
{
    const Uml::IDType idA = m_pOw[Uml::A]->localID();
    const Uml::IDType idB = m_pOw[Uml::B]->localID();
    if (idA != id && idB != id) {
        uDebug() << "id=" << ID2STR(id) << ": ignoring for idA=" << ID2STR(idA)
            << ", idB=" << ID2STR(idB);
        return;
    }
    m_nY = y();
    if (m_nY < getMinY())
        m_nY = getMinY();
    if (m_nY > getMaxY())
        m_nY = getMaxY();
    calculateWidget();
    if( !m_pFText )
        return;
    if (m_scene->selectedCount(true) > 1)
        return;
    setTextPosition();
}

/**
 * Check to see if the given ObjectWidget is involved in the message.
 *
 * @param w The ObjectWidget to check for.
 * @return  true - if is contained, false - not contained.
 */
bool MessageWidget::contains(ObjectWidget * w)
{
    if(m_pOw[Uml::A] == w || m_pOw[Uml::B] == w)
        return true;
    else
        return false;
}

void MessageWidget::slotMenuSelection(QAction* action)
{
    ListPopupMenu::MenuType sel = m_pMenu->getMenuType(action);
    if(sel == ListPopupMenu::mt_Delete) {
        // This will clean up this widget and the text widget:
        m_scene -> removeWidget(this);
    } else {

        UMLWidget::slotMenuSelection( action );
    }
}

/**
 * Activates a MessageWidget.  Connects its m_pOw[] pointers
 * to UMLObjects and also send signals about its FloatingTextWidget.
 */
bool MessageWidget::activate(IDChangeLog * /*Log = 0*/)
{
    m_scene->resetPastePoint();
    // UMLWidget::activate(Log);   CHECK: I don't think we need this ?
    if (m_pOw[Uml::A] == NULL) {
        UMLWidget *pWA = m_scene->findWidget(m_widgetAId);
        if (pWA == NULL) {
            uDebug() << "role A object " << ID2STR(m_widgetAId) << " not found";
            return false;
        }
        m_pOw[Uml::A] = dynamic_cast<ObjectWidget*>(pWA);
        if (m_pOw[Uml::A] == NULL) {
            uDebug() << "role A widget " << ID2STR(m_widgetAId)
                << " is not an ObjectWidget";
            return false;
        }
    }
    if (m_pOw[Uml::B] == NULL) {
        UMLWidget *pWB = m_scene->findWidget(m_widgetBId);
        if (pWB == NULL) {
            uDebug() << "role B object " << ID2STR(m_widgetBId) << " not found";
            return false;
        }
        m_pOw[Uml::B] = dynamic_cast<ObjectWidget*>(pWB);
        if (m_pOw[Uml::B] == NULL) {
            uDebug() << "role B widget " << ID2STR(m_widgetBId)
                << " is not an ObjectWidget";
            return false;
        }
    }
    updateResizability();

    UMLClassifier *c = dynamic_cast<UMLClassifier*>(m_pOw[Uml::B]->umlObject());
    UMLOperation *op = NULL;
    if (c && !m_CustomOp.isEmpty()) {
        Uml::IDType opId = STR2ID(m_CustomOp);
        op = dynamic_cast<UMLOperation*>( c->findChildObjectById(opId, true) );
        if (op) {
            // If the UMLOperation is set, m_CustomOp isn't used anyway.
            // Just setting it empty for the sake of sanity.
            m_CustomOp.clear();
        }
    }

    if( !m_pFText ) {
        Uml::TextRole tr = Uml::TextRole::Seq_Message;
        if (m_pOw[Uml::A] == m_pOw[Uml::B])
            tr = Uml::TextRole::Seq_Message_Self;
        m_pFText = new FloatingTextWidget( m_scene, tr, "" );
        m_pFText->setFont(UMLWidget::font());
    }
    if (op)
        setOperation(op);  // This requires a valid m_pFText.
    setLinkAndTextPos();
    m_pFText -> setText("");
    m_pFText->setActivated();
    QString messageText = m_pFText->text();
    m_pFText->setVisible( messageText.length() > 1 );

    connect(m_pOw[Uml::A], SIGNAL(sigWidgetMoved(Uml::IDType)), this, SLOT(slotWidgetMoved(Uml::IDType)));
    connect(m_pOw[Uml::B], SIGNAL(sigWidgetMoved(Uml::IDType)), this, SLOT(slotWidgetMoved(Uml::IDType)));

    connect(this, SIGNAL(sigMessageMoved()), m_pOw[Uml::A], SLOT(slotMessageMoved()) );
    connect(this, SIGNAL(sigMessageMoved()), m_pOw[Uml::B], SLOT(slotMessageMoved()) );
    m_pOw[Uml::A] -> messageAdded(this);
    m_pOw[Uml::B] -> messageAdded(this);
    calculateDimensions();

    emit sigMessageMoved();
    return true;
}

/**
 * Overrides operation from LinkWidget.
 * Required by FloatingTextWidget.
 *
 * @param ft   The text widget which to update.
 */
void MessageWidget::setMessageText(FloatingTextWidget *ft)
{
    if (ft == NULL)
        return;
    QString displayText = m_SequenceNumber + ": " + operationText(m_scene);
    ft->setText(displayText);
    setTextPosition();
}

/**
 * Overrides operation from LinkWidget.
 * Required by FloatingTextWidget.
 *
 * @param ft        The text widget which to update.
 * @param newText   The new text to set.
 */
void MessageWidget::setText(FloatingTextWidget *ft, const QString &newText)
{
    ft->setText(newText);
    UMLApp::app()->document()->setModified(true);
}

/**
 * Overrides operation from LinkWidget.
 * Required by FloatingTextWidget.
 *
 * @param seqNum    The new sequence number string to set.
 * @param op        The new operation string to set.
 */
void MessageWidget::setSeqNumAndOp(const QString &seqNum, const QString &op)
{
    setSequenceNumber( seqNum );
    m_CustomOp = op;   ///FIXME m_pOperation
}

/**
 * Write property of QString m_SequenceNumber.
 */
void MessageWidget::setSequenceNumber(const QString &sequenceNumber)
{
    m_SequenceNumber = sequenceNumber;
}

/**
 * Read property of QString m_SequenceNumber.
 */
QString MessageWidget::sequenceNumber() const
{
    return m_SequenceNumber;
}

/**
 * Implements operation from LinkWidget.
 * Required by FloatingTextWidget.
 */
void MessageWidget::lwSetFont (QFont font)
{
    UMLWidget::setFont( font );
}

/**
 * Overrides operation from LinkWidget.
 * Required by FloatingTextWidget.
 * @todo Move to LinkWidget.
 */
UMLClassifier *MessageWidget::operationOwner()
{
    UMLObject *pObject = m_pOw[Uml::B]->umlObject();
    if (pObject == NULL)
        return NULL;
    UMLClassifier *c = dynamic_cast<UMLClassifier*>(pObject);
    return c;
}

/**
 * Implements operation from LinkWidget.
 * Motivated by FloatingTextWidget.
 */
UMLOperation *MessageWidget::operation()
{
    return static_cast<UMLOperation*>(m_pObject);
}

/**
 * Implements operation from LinkWidget.
 * Motivated by FloatingTextWidget.
 */
void MessageWidget::setOperation(UMLOperation *op)
{
    if (m_pObject && m_pFText)
        disconnect(m_pObject, SIGNAL(modified()), m_pFText, SLOT(setMessageText()));
    m_pObject = op;
    if (m_pObject && m_pFText)
        connect(m_pObject, SIGNAL(modified()), m_pFText, SLOT(setMessageText()));
}

/**
 * Overrides operation from LinkWidget.
 * Required by FloatingTextWidget.
 */
QString MessageWidget::customOpText()
{
    return m_CustomOp;
}

/**
 * Overrides operation from LinkWidget.
 * Required by FloatingTextWidget.
 */
void MessageWidget::setCustomOpText(const QString &opText)
{
    m_CustomOp = opText;
    m_pFText->setMessageText();
}

UMLClassifier * MessageWidget::seqNumAndOp(QString& seqNum, QString& op)
{
    seqNum = m_SequenceNumber;
    UMLOperation *pOperation = operation();
    if (pOperation != NULL) {
        op = pOperation->toString(Uml::SignatureType::SigNoVis);
    } else {
        op = m_CustomOp;
    }
    UMLObject *o = m_pOw[Uml::B]->umlObject();
    UMLClassifier *c = dynamic_cast<UMLClassifier*>(o);
    return c;
}

/**
 * Calculates the size of the widget by calling
 * calculateDimensionsSynchronous(),
 * calculateDimensionsAsynchronous(), or
 * calculateDimensionsCreation()
 */
void MessageWidget::calculateDimensions()
{
    if (m_sequenceMessageType == Uml::sequence_message_synchronous) {
        calculateDimensionsSynchronous();
    } else if (m_sequenceMessageType == Uml::sequence_message_asynchronous) {
        calculateDimensionsAsynchronous();
    } else if (m_sequenceMessageType == Uml::sequence_message_creation) {
        calculateDimensionsCreation();
    } else if (m_sequenceMessageType == Uml::sequence_message_lost) {
        calculateDimensionsLost();
    } else if (m_sequenceMessageType == Uml::sequence_message_found) {
        calculateDimensionsFound();
    } else {
        uWarning() << "Unknown message type";
    }
    if (! UMLApp::app()->document()->loading()) {
        adjustAssocs( x(), y() );  // adjust assoc lines
    }
}

/**
 * Calculates and sets the size of the widget for a synchronous message.
 */
void MessageWidget::calculateDimensionsSynchronous()
{
    int x = 0;

    int x1 = m_pOw[Uml::A]->x();
    int x2 = m_pOw[Uml::B]->x();
    int w1 = m_pOw[Uml::A]->width() / 2;
    int w2 = m_pOw[Uml::B]->width() / 2;
    x1 += w1;
    x2 += w2;

    int widgetWidth = 0;
    int widgetHeight = 0;
    if( m_pOw[Uml::A] == m_pOw[Uml::B] ) {
        widgetWidth = 50;
        x = x1 - 2;
    } else if( x1 < x2 ) {
        x = x1;
        widgetWidth = x2 - x1 + 8;
    } else {
        x = x2 - 8;
        widgetWidth = x1 - x2 + 8;
    }

    if ( height() < 20 ) {
        widgetHeight = 20;
    } else {
        widgetHeight = height();
    }

    m_nPosX = x;
    setSize(widgetWidth, widgetHeight);
}

/**
 * Calculates and sets the size of the widget for an asynchronous message.
 */
void MessageWidget::calculateDimensionsAsynchronous()
{
    int x = 0;

    int x1 = m_pOw[Uml::A]->x();
    int x2 = m_pOw[Uml::B]->x();
    int w1 = m_pOw[Uml::A]->width() / 2;
    int w2 = m_pOw[Uml::B]->width() / 2;
    x1 += w1;
    x2 += w2;

    int widgetWidth = 0;
    int widgetHeight = 8;
    if( m_pOw[Uml::A] == m_pOw[Uml::B] ) {
        widgetWidth = 50;
        x = x1;
        if( height() < 20 ) {
            widgetHeight = 20;
        } else {
            widgetHeight = height();
        }
    } else if( x1 < x2 ) {
        x = x1;
        widgetWidth = x2 - x1;
    } else {
        x = x2;
        widgetWidth = x1 - x2;
    }
    x += 1;
    widgetWidth -= 2;
    m_nPosX = x;
    setSize(widgetWidth, widgetHeight);
}

/**
 * Calculates and sets the size of the widget for a creation message.
 */
void MessageWidget::calculateDimensionsCreation()
{
    int x = 0;

    int x1 = m_pOw[Uml::A]->x();
    int x2 = m_pOw[Uml::B]->x();
    int w1 = m_pOw[Uml::A]->width() / 2;
    int w2 = m_pOw[Uml::B]->width();
    x1 += w1;
    if (x1 > x2)
        x2 += w2;

    int widgetWidth = 0;
    int widgetHeight = 8;
    if ( x1 < x2 ) {
        x = x1;
        widgetWidth = x2 - x1;
    } else {
        x = x2;
        widgetWidth = x1 - x2;
    }
    x += 1;
    widgetWidth -= 2;
    m_nPosX = x;
    m_nY = m_pOw[Uml::B]->y() + m_pOw[Uml::B]->height() / 2;
    setSize(widgetWidth, widgetHeight);
}

/**
 * Calculates and sets the size of the widget for a lost message.
 */
void MessageWidget::calculateDimensionsLost()
{
    int x = 0;

    int x1 = m_pOw[Uml::A]->x();
    int x2 = xclicked;
    int w1 = m_pOw[Uml::A]->width() / 2;

    x1 += w1;

    int widgetWidth = 0;
    int widgetHeight = 10;
    if( x1 < x2 ) {
        x = x1;
        widgetWidth = x2 - x1 + widgetHeight;
    } else {
        x = x2;
        widgetWidth = x1 - x2;
    }
    x += 1;
    m_nPosX = x;
    setSize(widgetWidth, widgetHeight);
}

/**
 * Calculates and sets the size of the widget for a found message.
 */
void MessageWidget::calculateDimensionsFound()
{
    int x = 0;

    int x1 = m_pOw[Uml::A]->x();
    int x2 = xclicked;
    int w1 = m_pOw[Uml::A]->width() / 2;
    x1 += w1;


    int widgetWidth = 0;
    int widgetHeight = 10;
    if( x1 < x2 ) {
        x = x1;
        widgetWidth = x2 - x1 + widgetHeight;
    } else {
        x = x2 ;
        widgetWidth = x1 - x2;
    }
    x += 1;

    m_nPosX = x;
    setSize(widgetWidth, widgetHeight);
}

/**
 * Used to cleanup any other widget it may need to delete.
 */
void MessageWidget::cleanup()
{
    if (m_pOw[Uml::A]) {
        disconnect(this, SIGNAL(sigMessageMoved()), m_pOw[Uml::A], SLOT(slotMessageMoved()) );
        m_pOw[Uml::A]->messageRemoved(this);
    }
    if (m_pOw[Uml::B]) {
        disconnect(this, SIGNAL(sigMessageMoved()), m_pOw[Uml::B], SLOT(slotMessageMoved()) );
        m_pOw[Uml::B]->messageRemoved(this);
    }

    UMLWidget::cleanup();
    if (m_pFText) {
        m_scene->removeWidget(m_pFText);
        m_pFText = NULL;
    }
}

/**
 * Sets the state of whether the widget is selected.
 *
 * @param _select   True if the widget is selected.
 */
void MessageWidget::setSelected(bool _select)
{
    UMLWidget::setSelected( _select );
    if( !m_pFText || m_pFText->displayText().isEmpty())
        return;
    if( m_selected && m_pFText->isSelected() )
        return;
    if( !m_selected && !m_pFText->isSelected() )
        return;

    m_scene -> setSelected( m_pFText, 0 );
    m_pFText -> setSelected( m_selected );
}

/**
 * Returns the minimum height this widget should be set at on
 * a sequence diagrams.  Takes into account the widget positions
 * it is related to.
 */
int MessageWidget::getMinY()
{
    if (!m_pOw[Uml::A] || !m_pOw[Uml::B]) {
        return 0;
    }
    if (m_sequenceMessageType == Uml::sequence_message_creation) {
        return m_pOw[Uml::A]->y() + m_pOw[Uml::A]->height();
    }
    int heightA = m_pOw[Uml::A]->y() + m_pOw[Uml::A]->height();
    int heightB = m_pOw[Uml::B]->y() + m_pOw[Uml::B]->height();
    int height = heightA;
    if( heightA < heightB ) {
        height = heightB;
    }
    return height;
}

/**
 * Returns the maximum height this widget should be set at on
 * a sequence diagrams.  Takes into account the widget positions
 * it is related to.
 */
int MessageWidget::getMaxY()
{
    if( !m_pOw[Uml::A] || !m_pOw[Uml::B] ) {
        return 0;
    }
    int heightA = (int)((ObjectWidget*)m_pOw[Uml::A])->getEndLineY();
    int heightB = (int)((ObjectWidget*)m_pOw[Uml::B])->getEndLineY();
    int height = heightA;
    if( heightA > heightB ) {
        height = heightB;
    }
    return (height - this->height());
}

/**
 * Sets the related widget on the given side.
 *
 * @param ow     The ObjectWidget we are related to.
 * @param role   The Uml::Role_Type to be set for the ObjectWidget
 */
void MessageWidget::setObjectWidget(ObjectWidget * ow, Uml::Role_Type role)
{
    m_pOw[role] = ow;
    updateResizability();
}

/**
 * Returns the related widget on the given side.
 *
 * @return  The ObjectWidget we are related to.
 */
ObjectWidget* MessageWidget::objectWidget(Uml::Role_Type role)
{
    return m_pOw[role];
}

/**
 * Set the xclicked
 */
void MessageWidget::setxclicked(int xclick)
{
    xclicked = xclick;
}


/**
 * Set the yclicked
 */
void MessageWidget::setyclicked(int yclick)
{
    yclicked = yclick;
}

/**
 * Saves to the "messagewidget" XMI element.
 */
void MessageWidget::saveToXMI(QDomDocument & qDoc, QDomElement & qElement)
{
    QDomElement messageElement = qDoc.createElement( "messagewidget" );
    UMLWidget::saveToXMI( qDoc, messageElement );
    messageElement.setAttribute( "widgetaid", ID2STR(m_pOw[Uml::A]->localID()) );
    messageElement.setAttribute( "widgetbid", ID2STR(m_pOw[Uml::B]->localID()) );
    UMLOperation *pOperation = operation();
    if (pOperation)
        messageElement.setAttribute( "operation", ID2STR(pOperation->id()) );
    else
        messageElement.setAttribute( "operation", m_CustomOp );
    messageElement.setAttribute( "seqnum", m_SequenceNumber );
    messageElement.setAttribute( "sequencemessagetype", m_sequenceMessageType );
    if (m_sequenceMessageType == Uml::sequence_message_lost || m_sequenceMessageType == Uml::sequence_message_found) {
        messageElement.setAttribute( "xclicked", xclicked );
        messageElement.setAttribute( "yclicked", yclicked );
    }

    // save the corresponding message text
    if (m_pFText && !m_pFText->text().isEmpty()) {
        messageElement.setAttribute( "textid", ID2STR(m_pFText->id()) );
        m_pFText -> saveToXMI( qDoc, messageElement );
    }

    qElement.appendChild( messageElement );
}

/**
 * Loads from the "messagewidget" XMI element.
 */
bool MessageWidget::loadFromXMI(QDomElement& qElement)
{
    if ( !UMLWidget::loadFromXMI(qElement) ) {
        return false;
    }
    QString textid = qElement.attribute( "textid", "-1" );
    QString widgetaid = qElement.attribute( "widgetaid", "-1" );
    QString widgetbid = qElement.attribute( "widgetbid", "-1" );
    m_CustomOp = qElement.attribute( "operation", "" );
    m_SequenceNumber = qElement.attribute( "seqnum", "" );
    QString sequenceMessageType = qElement.attribute( "sequencemessagetype", "1001" );
    m_sequenceMessageType = (Uml::Sequence_Message_Type)sequenceMessageType.toInt();
    if (m_sequenceMessageType == Uml::sequence_message_lost || m_sequenceMessageType == Uml::sequence_message_found) {
        xclicked = qElement.attribute( "xclicked", "-1" ).toInt();
        yclicked = qElement.attribute( "yclicked", "-1" ).toInt();
    }

    m_widgetAId = STR2ID(widgetaid);
    m_widgetBId = STR2ID(widgetbid);
    m_textId = STR2ID(textid);

    Uml::TextRole tr = Uml::TextRole::Seq_Message;
    if (m_widgetAId == m_widgetBId)
        tr = Uml::TextRole::Seq_Message_Self;

    //now load child elements
    QDomNode node = qElement.firstChild();
    QDomElement element = node.toElement();
    if ( !element.isNull() ) {
        QString tag = element.tagName();
        if (tag == "floatingtext") {
            m_pFText = new FloatingTextWidget( m_scene, tr, operationText(m_scene), m_textId );
            if( ! m_pFText->loadFromXMI(element) ) {
                // Most likely cause: The FloatingTextWidget is empty.
                delete m_pFText;
                m_pFText = NULL;
            }
        } else {
            uError() << "unknown tag " << tag;
        }
    }
    return true;
}

ListPopupMenu* MessageWidget::setupPopupMenu(ListPopupMenu *menu)
{
    UMLWidget::setupPopupMenu(menu); // will setup the menu in m_pMenu
    ListPopupMenu* floatingtextSubMenu = m_pFText->setupPopupMenu();
    floatingtextSubMenu->setTitle( i18n( "Operation" ) );

    m_pMenu->addMenu( floatingtextSubMenu );

    return m_pMenu;
}

#include "messagewidget.moc"
