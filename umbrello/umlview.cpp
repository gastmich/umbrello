/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <climits>
#include <math.h>

// include files for Qt
#include <qpixmap.h>
#include <qprinter.h>
#include <qpainter.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qobjectlist.h>
#include <qobjectdict.h>
#include <qdragobject.h>
#include <qpaintdevicemetrics.h>
#include <qfileinfo.h>
#include <qptrlist.h>
#include <qcolor.h>
#include <qwmatrix.h>
#include <qregexp.h>

//kde include files
#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kcursor.h>
#include <kfiledialog.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kdebug.h>

// application specific includes
#include "umlview.h"
#include "listpopupmenu.h"
#include "uml.h"
#include "umldoc.h"
#include "umlobject.h"
#include "docwindow.h"
#include "assocrules.h"
#include "umlviewcanvas.h"
#include "dialogs/classoptionspage.h"
#include "dialogs/umlviewdialog.h"

#include "clipboard/idchangelog.h"
#include "clipboard/umldrag.h"

#include "classwidget.h"
#include "packagewidget.h"
#include "componentwidget.h"
#include "nodewidget.h"
#include "artifactwidget.h"
#include "interfacewidget.h"
#include "actorwidget.h"
#include "usecasewidget.h"
#include "notewidget.h"
#include "boxwidget.h"
#include "associationwidget.h"
#include "objectwidget.h"
#include "floatingtext.h"
#include "messagewidget.h"
#include "statewidget.h"
#include "activitywidget.h"
#include "seqlinewidget.h"

#include "umllistviewitemdatalist.h"
#include "umllistviewitemdata.h"
#include "umlobjectlist.h"
#include "association.h"

#include "umlwidget.h"

// static members
const int UMLView::defaultCanvasSize = 1300;


// constructor
UMLView::UMLView(QWidget* parent, UMLDoc* doc) : QCanvasView(parent, "AnUMLView") {
	init();
	m_pDoc = doc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::init() {
	// Initialize loaded/saved data
	m_nID = -1;
	m_pDoc = NULL;
	m_Documentation = "";
	m_Name = "umlview";
	m_Type = dt_Undefined;
	m_nLocalID = 30000;
	m_bUseSnapToGrid = false;
	m_bUseSnapComponentSizeToGrid = false;
	m_bShowSnapGrid = false;
	m_nSnapX = 10;
	m_nSnapY = 10;
	m_nZoom = 100;
	m_nCanvasWidth = UMLView::defaultCanvasSize;
	m_nCanvasHeight = UMLView::defaultCanvasSize;

	// Initialize other data
	m_AssociationList.setAutoDelete( true );
	//Setup up booleans
	m_bPaste = false;
	m_bDrawRect = false;
	m_bActivated = false;
	m_bCreateObject = false;
	m_bDrawSelectedOnly = false;
	m_bPopupShowing = false;
	m_bStartedCut = false;
	m_bMouseButtonPressed = false;
	//clear pointers
	m_pMoveAssoc = 0;
	m_pOnWidget = 0;
	m_pAssocLine = 0;
	m_pIDChangesLog = 0;
	m_pFirstSelectedWidget = 0;
	m_pMenu = 0;
	//setup graphical items
	viewport() -> setBackgroundMode( NoBackground );
	setCanvas( new UMLViewCanvas( this ) );
	canvas() -> setUpdatePeriod( 20 );
	resizeContents(defaultCanvasSize, defaultCanvasSize);
	canvas() -> resize(defaultCanvasSize, defaultCanvasSize);
	setAcceptDrops(TRUE);
	viewport() -> setAcceptDrops(TRUE);
	setDragAutoScroll(false);
	viewport() -> setMouseTracking(false);
	m_SelectionRect.setAutoDelete( true );
	m_CurrentCursor = WorkToolBar::tbb_Arrow;
	//setup signals
	connect( this, SIGNAL(sigRemovePopupMenu()), this, SLOT(slotRemovePopupMenu() ) );
	connect( UMLApp::app(), SIGNAL( sigCutSuccessful() ),
	         this, SLOT( slotCutSuccessful() ) );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UMLView::~UMLView() {
	if(m_pIDChangesLog) {
		delete    m_pIDChangesLog;
		m_pIDChangesLog = 0;
	}
	if( m_pAssocLine )
		delete m_pAssocLine;
	m_SelectionRect.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
UMLDoc* UMLView::getDocument() const {
	return m_pDoc;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::print(KPrinter *pPrinter, QPainter & pPainter) {
	int height, width;
	int offsetX = 0, offsetY = 0, widthX = 0, heightY = 0;
	//get the size of the page
	QPaintDeviceMetrics metrics(pPrinter);
	QFontMetrics fm = pPainter.fontMetrics(); // use the painter font metrics, not the screen fm!
	int fontHeight  = fm.lineSpacing();
	int marginX = pPrinter->margins().width();
	int marginY = pPrinter->margins().height();

	// The printer will probably use a different font with different font metrics,
	// force the widgets to update accordingly on paint
	forceUpdateWidgetFontMetrics(&pPainter);

	//double margin at botton of page as it doesn't print down there
	//on my printer, so play safe as default.
	if(pPrinter->orientation() == KPrinter::Portrait) {
		width = metrics.width() - marginX * 2;
		height = metrics.height() - fontHeight - 4 - marginY * 3;
	} else {
		marginX *= 2;
		width = metrics.width() - marginX * 2;
		height = metrics.height() - fontHeight - 4 - marginY * 2;
	}
	//get the smallest rect holding the diagram
	QRect rect = getDiagramRect();
	//now draw to printer

	// respect the margin
	pPainter.translate(marginX, marginY);

	// clip away everything outside of the margin
	pPainter.setClipRect(marginX, marginY,
						 width, metrics.height() - marginY * 2);

	//loop until all of the picture is printed
	int numPagesX = (int)ceilf((float)rect.width()/(float)width);
	int numPagesY = (int)ceilf((float)rect.height()/(float)height);
	int page = 0;

	// print the canvas to multiple pages
	for (int pageY = 0; pageY < numPagesY; ++pageY) {
		// tile vertically
		offsetY = pageY * height + rect.y();
		heightY = (pageY + 1) * height > rect.height()
				? rect.height() - pageY * height
				: height;
		for (int pageX = 0; pageX < numPagesX; ++pageX) {
			// tile horizontally
			offsetX = pageX * width + rect.x();
			widthX = (pageX + 1) * width > rect.width()
				? rect.width() - pageX * width
				: width;

			// make sure the part of the diagram is painted at the correct
			// place in the printout
			pPainter.translate(-offsetX,-offsetY);
			getDiagram(QRect(offsetX, offsetY,widthX, heightY),
				   pPainter);
			// undo the translation so the coordinates for the painter
			// correspond to the page again
			pPainter.translate(offsetX,offsetY);

			//draw foot note
			QString string = i18n("Diagram: %2 Page %1").arg(page + 1).arg(getName());
			QColor textColor(50, 50, 50);
			pPainter.setPen(textColor);
			pPainter.drawLine(0, height + 2, width, height + 2);
			pPainter.drawText(0, height + 4, width, fontHeight, AlignLeft, string);

			if(pageX+1 < numPagesX || pageY+1 < numPagesY) {
				pPrinter -> newPage();
				page++;
			}
		}
	}
	// next painting will most probably be to a different device (i.e. the screen)
	forceUpdateWidgetFontMetrics(0);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::setupNewWidget(UMLWidget *w, bool setNewID) {
	if (setNewID)
		w->setID( m_pDoc->getUniqueID() ); //needed for associations
	w->setX( m_Pos.x() );
	w->setY( m_Pos.y() );
	w->setVisible( true );
	w->setActivated();
	w->setFont( getFont() );
	w->slotColorChanged( getID() );
	resizeCanvasToItems();
	m_WidgetList.append( w );
	m_pDoc->setModified();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::contentsMouseReleaseEvent(QMouseEvent* ome) {

	m_bMouseButtonPressed = false;
	QMouseEvent* me = new QMouseEvent(QEvent::MouseButtonRelease, inverseWorldMatrix().map(ome->pos()),
					  ome->button(),ome->state());

	if(m_bDrawRect) {
		viewport()->setMouseTracking( false );
		m_bDrawRect = false;
		m_SelectionRect.clear();
	}
	if( m_pAssocLine ) {
		delete m_pAssocLine;
		m_pAssocLine = 0;
	}
	m_Pos.setX(me->x());
	m_Pos.setY(me->y());

	if( allocateMouseReleaseEvent(me) ) {
		return;
	}

	if( m_CurrentCursor == WorkToolBar::tbb_Arrow || me -> state() != LeftButton ) {
		viewport()->setMouseTracking( false );
		if (me->state() == RightButton) {

			/* if the user right clicks on the diagram, first the default tool is
			 * selected from the toolbar; this only happens when the default tool
			 * wasn't selected yet AND there is no other widget under the mouse
			 * pointer
			 * in any other case the right click menu will be shown */
			if ( m_CurrentCursor != WorkToolBar::tbb_Arrow )
				UMLApp::app()->getWorkToolBar()->setDefaultTool();
			else		
				setMenu();
		}
		return;
	}

	//create an activity widget
	ActivityWidget::ActivityType actType;
	if (ActivityWidget::isActivity(m_CurrentCursor, actType)) {
		ActivityWidget * temp = new ActivityWidget( this , actType );
		if( m_CurrentCursor == WorkToolBar::tbb_Activity ) {
			bool ok = false;
			QString name = KLineEditDlg::getText( i18n("Enter Activity Name"),
							      i18n("Enter the name of the new activity:"),
							      i18n("new activity"), &ok );
			if( !ok ) {
				temp->cleanup();
				delete temp;
				resizeCanvasToItems();
				m_pDoc->setModified();
				return;
			}
			temp->setName( name );
		}
		setupNewWidget( temp );
		return;
	}

	//create a state widget
	StateWidget::StateType stateType;
	if (StateWidget::isState(m_CurrentCursor, stateType)) {
		StateWidget * temp = new StateWidget( this , stateType );
		if( m_CurrentCursor == WorkToolBar::tbb_State ) {
			bool ok = false;
			QString name = KLineEditDlg::getText( i18n("Enter State Name"),
							      i18n("Enter the name of the new state:"),
							      i18n("new state"), &ok );
			if( !ok ) {
				temp->cleanup();
				delete temp;
				resizeCanvasToItems();
				m_pDoc->setModified();
				return;
			}
			temp -> setName( name );
		}
		setupNewWidget( temp );
		return;
	}

	//Create a NoteBox widget
	if(m_CurrentCursor == WorkToolBar::tbb_Note) {
		//no need to register with document but get an id from it
		//id used when checking to delete object and assocs
		NoteWidget *temp= new NoteWidget(this, getDocument()->getUniqueID());
		setupNewWidget( temp, false );
		return;
	}

	//Create a Box widget
	if(m_CurrentCursor == WorkToolBar::tbb_Box) {
		//no need to register with document but get a id from it
		//id used when checking to delete object and assocs
		BoxWidget* newBox = new BoxWidget(this, getDocument()->getUniqueID());
		setupNewWidget( newBox, false );
		return;
	}

	//Create a Floating Text widget
	if(m_CurrentCursor == WorkToolBar::tbb_Text) {
		FloatingText * ft = new FloatingText(this, tr_Floating, "");
		ft -> changeTextDlg();
		//if no text entered delete
		if(!FloatingText::isTextValid(ft -> getText())) {
			ft->cleanup();
			delete ft;
			resizeCanvasToItems();
			m_pDoc->setModified();
		} else {
			setupNewWidget( ft );
		}
		return;
	}

	//Create a Message on a Sequence diagram
	bool isSyncMsg = (m_CurrentCursor == WorkToolBar::tbb_Seq_Message_Synchronous);
	bool isAsyncMsg = (m_CurrentCursor == WorkToolBar::tbb_Seq_Message_Asynchronous);
	if (isSyncMsg || isAsyncMsg) {
		ObjectWidget* clickedOnWidget = onWidgetLine( me->pos() );
		if( !clickedOnWidget ) {
			//did not click on widget line, clear the half made message
			m_pFirstSelectedWidget = 0;
			return;
		}
		if(!m_pFirstSelectedWidget) { //we are starting a new message
			m_pFirstSelectedWidget = clickedOnWidget;
			viewport()->setMouseTracking( true );
			m_pAssocLine = new QCanvasLine( canvas() );
			m_pAssocLine->setPoints( me->x(), me->y(), me->x(), me->y() );
			m_pAssocLine->setPen( QPen( getLineColor(), 0, DashLine ) );
			m_pAssocLine->setVisible( true );
			return;
		}
		//clicked on second sequence line to create message
		FloatingText* messageText = new FloatingText(this, tr_Seq_Message, "");
		messageText->setFont( getFont() );

		Sequence_Message_Type msgType = (isSyncMsg ? sequence_message_synchronous :
							     sequence_message_asynchronous);
		ObjectWidget* pFirstSelectedObj = dynamic_cast<ObjectWidget*>(m_pFirstSelectedWidget);
		if (pFirstSelectedObj == NULL) {
			kdDebug() << "first selected widget is not an object" << endl;
			return;
		}
		MessageWidget* message = new MessageWidget(this, pFirstSelectedObj,
							   clickedOnWidget, messageText,
							   m_pDoc->getUniqueID(),
							   me->y(),
							   msgType);
		connect(this, SIGNAL(sigColorChanged(int)),
			message, SLOT(slotColorChanged(int)));

		// WAS:
		//	messageText->setID(m_pDoc -> getUniqueID());
		// EXPERIMENTALLY CHANGED TO:
			messageText->setID( message->getID() );

		messageText->setMessage( message );
		messageText->setActivated();
		message->setActivated();
		m_pFirstSelectedWidget = 0;
		resizeCanvasToItems();
		m_MessageList.append(message);
		m_pDoc->setModified();
		return;
	}

	if ( m_CurrentCursor < WorkToolBar::tbb_Actor || m_CurrentCursor > WorkToolBar::tbb_State ) {
		m_pFirstSelectedWidget = 0;
		return;
	}

	//if we are creating an object, we really create a class
	if(m_CurrentCursor == WorkToolBar::tbb_Object) {
		m_CurrentCursor = WorkToolBar::tbb_Class;
	}
	m_bCreateObject = true;
	m_pDoc->createUMLObject(convert_TBB_OT(m_CurrentCursor));
	resizeCanvasToItems();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::slotToolBarChanged(int c) {
	m_CurrentCursor = (WorkToolBar::ToolBar_Buttons)c;
	m_pFirstSelectedWidget = 0;
	m_bPaste = false;
	m_bDrawRect = false;
	if( m_pAssocLine ) {
		delete m_pAssocLine;
		m_pAssocLine = 0;
	}
	viewport() -> setMouseTracking( false );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::showEvent(QShowEvent* /*se*/) {
	UMLApp* theApp = UMLApp::app();
	WorkToolBar* tb = theApp->getWorkToolBar();
	connect(tb,SIGNAL(sigButtonChanged(int)), this, SLOT(slotToolBarChanged(int)));
	connect(this,SIGNAL(sigResetToolBar()), tb, SLOT(slotResetToolBar()));
	connect(m_pDoc, SIGNAL(sigObjectCreated(UMLObject *)),
		this, SLOT(slotObjectCreated(UMLObject *)));
	resetToolbar();

}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::hideEvent(QHideEvent* /*he*/) {
	UMLApp* theApp = UMLApp::app();
	WorkToolBar* tb = theApp->getWorkToolBar();
	disconnect(tb,SIGNAL(sigButtonChanged(int)), this, SLOT(slotToolBarChanged(int)));
	disconnect(this,SIGNAL(sigResetToolBar()), tb, SLOT(slotResetToolBar()));
	disconnect(m_pDoc, SIGNAL(sigObjectCreated(UMLObject *)), this, SLOT(slotObjectCreated(UMLObject *)));
}
////////////////////////////////////////////////////////////////////////////////////////////////////
UMLListView * UMLView::getListView() {
	return m_pDoc->getListView();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::slotObjectCreated(UMLObject* o) {
	m_bPaste = false;
	int type  = o->getBaseType();
	//check to see if we want the message
	//may be wanted by someone else e.g. list view
	if (!m_bCreateObject)
		return;

	UMLWidget* newWidget = 0;
	if(type == ot_Actor) {
		newWidget = new ActorWidget(this, o);
	} else if(type == ot_UseCase) {
		newWidget = new UseCaseWidget(this, o);
	} else if(type == ot_Package) {
		newWidget = new PackageWidget(this, o);
	} else if(type == ot_Component) {
		newWidget = new ComponentWidget(this, o);
		if (getType() == dt_Deployment) {
			newWidget->setIsInstance(true);
		}
	} else if(type == ot_Artifact) {
		newWidget = new ArtifactWidget(this, o);
	} else if(type == ot_Node) {
		newWidget = new NodeWidget(this, o);
	} else if(type == ot_Interface) {
	        InterfaceWidget* interfaceWidget = new InterfaceWidget(this, o);
		Diagram_Type diagramType = getType();
		if (diagramType == dt_Component || diagramType == dt_Deployment) {
			interfaceWidget->setDrawAsCircle(true);
		}
		newWidget = (UMLWidget*)interfaceWidget;
	} else if(type == ot_Class ) { // CORRECT?
		//see if we really want an object widget or class widget
		if(getType() == dt_Class) {
			newWidget = new ClassWidget(this, o);
		} else {
			newWidget = new ObjectWidget(this, o, getLocalID() );
		}
	} else {
		kdWarning() << "ERROR: trying to create an invalid widget" << endl;
		return;
	}
	int y=m_Pos.y();

	if (newWidget->getBaseType() == wt_Object && this->getType() == dt_Sequence) {
		y = 80 - newWidget->height();
	}
	newWidget->setX( m_Pos.x() );
	newWidget->setY( y );
	newWidget->moveEvent( 0 );//needed for ObjectWidget to set seq. line properly
	newWidget->setVisible( true );
	newWidget->setActivated();
	newWidget->setFont( getFont() );
	newWidget->slotColorChanged( getID() );
	m_bCreateObject = false;
	switch( type ) {
		case ot_Actor:
		case ot_UseCase:
		case ot_Class:
		case ot_Package:
		case ot_Component:
		case ot_Node:
		case ot_Artifact:
		case ot_Interface:
			createAutoAssociations(newWidget);
			break;
	}
	resizeCanvasToItems();
	m_WidgetList.append(newWidget);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::slotObjectRemoved(UMLObject * o) {
	m_bPaste = false;
	int id = o->getID();
	UMLWidgetListIt it( m_WidgetList );
	UMLWidget *obj;

	while ((obj = it.current()) != 0 ) {
		++it;
		if(obj -> getID() != id)
			continue;
		removeWidget(obj);

		// Following lines temporarily commented out due to crashes.
		// I will look into this  -- okellogg  2003-09-02
		/* Update list; removing a widget will also delete the associations
		 * connected to it; so we have to update the list, because other
		 * widgets might be already deleted
		QObjectList * currentList = queryList("UMLWidget");
		QObjectListIt OldListIt(*l);

		while ((obj = (UMLWidget*)OldListIt.current()) != 0)
		{
			++OldListIt;
			if (!currentList->contains(obj))
			{
				l->remove(obj);
			}
		}
		it.toFirst();
		delete currentList;
		 */
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::contentsDragEnterEvent(QDragEnterEvent *e) {
	UMLListViewItemDataList list;
	if(!UMLDrag::decodeClip3(e, list)) {
		return;
	}
	UMLListViewItemDataListIt it(list);

	UMLListViewItemData* data = it.current();

	ListView_Type lvtype = data -> getType();

	Diagram_Type diagramType = getType();

	UMLObject* temp = 0;
	//if dragging diagram - set false
	switch( lvtype ) {
		case lvt_UseCase_Diagram:
		case lvt_Sequence_Diagram:
		case lvt_Class_Diagram:
		case lvt_Collaboration_Diagram:
		case lvt_State_Diagram:
		case lvt_Activity_Diagram:
			e->accept(false);
			return;
		default:
			break;
	}
	//can't drag anything onto state/activity diagrams
	if( diagramType == dt_State || diagramType == dt_Activity) {
		e->accept(false);
		return;
	}
	//make sure can find UMLObject
	if( !(temp = m_pDoc->findUMLObject(data -> getID()) ) ) {
		kdDebug() << " object not found" << endl;
		e->accept(false);
		return;
	}
	//make sure dragging item onto correct diagram
	// concept - class,seq,coll diagram
	// actor,usecase - usecase diagram
	UMLObject_Type ot = temp->getBaseType();
	if(diagramType == dt_UseCase && (ot != ot_Actor && ot != ot_UseCase) ) {
		e->accept(false);
		return;
	}
	if((diagramType == dt_Sequence || diagramType == dt_Class ||
	    diagramType == dt_Collaboration) &&
	   (ot != ot_Class && ot != ot_Package && ot != ot_Interface) ) {
		e->accept(false);
		return;
	}
	if (diagramType == dt_Deployment &&
	    (ot != ot_Interface && ot != ot_Component && ot != ot_Class && ot != ot_Node)) {
		e->accept(false);
		return;
	}
	if (diagramType == dt_Component &&
	    (ot != ot_Interface && ot != ot_Component && ot != ot_Artifact)) {
		e->accept(false);
		return;
	}
	if((diagramType == dt_UseCase || diagramType == dt_Class ||
	    diagramType == dt_Component || diagramType == dt_Deployment)
	   && widgetOnDiagram(data->getID()) ) {
		e->accept(false);
		return;
	}
	e->accept(true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::contentsDropEvent(QDropEvent *e) {
	UMLListViewItemDataList list;
	if( !UMLDrag::decodeClip3(e, list) ) {
		return;
	}

	UMLListViewItemDataListIt it(list);
	UMLListViewItemData* data = it.current();
	ListView_Type lvtype = data->getType();
	UMLObject* o = 0;
	if(lvtype >= lvt_UseCase_Diagram && lvtype <= lvt_Sequence_Diagram) {
		return;
	}
	if( !( o = m_pDoc->findUMLObject(data->getID()) ) ) {
		kdDebug() << " object not found" << endl;
		return;
	}
	m_bCreateObject = true;
	m_Pos = e->pos();

	slotObjectCreated(o);

	m_pDoc -> setModified(true);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
ObjectWidget * UMLView::onWidgetLine( QPoint point ) {
	SeqLineWidget * pLine = 0;
	for( pLine = m_SeqLineList.first(); pLine; pLine = m_SeqLineList.next() ) {
		if( pLine -> onWidget( point ) ) {
			return pLine -> getObjectWidget();
		}
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::checkMessages(ObjectWidget * w) {
	if(getType() != dt_Sequence)
		return;

	MessageWidgetListIt it( m_MessageList );
	MessageWidget *obj;
	while ( (obj = it.current()) != 0 ) {
		++it;
		if(! obj -> contains(w))
			continue;
		//make sure message doesn't have any associations
		removeAssociations(obj);
		obj -> cleanup();
		//make sure not in selected list
		m_SelectedList.remove(obj);
		m_MessageList.remove(obj);
		delete obj;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
bool UMLView::widgetOnDiagram(int id) {
	UMLWidget *obj;

	UMLWidgetListIt it( m_WidgetList );
	while ( (obj = it.current()) != 0 ) {
		++it;
		if(id == obj -> getID())
			return true;
	}

	MessageWidgetListIt mit( m_MessageList );
	while ( (obj = (UMLWidget*)mit.current()) != 0 ) {
		++mit;
		if(id == obj -> getID())
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::contentsMouseMoveEvent(QMouseEvent* ome) {
        //Autoscroll
	if (m_bMouseButtonPressed) {
		int vx = ome->x();
		int vy = ome->y();
		int contsX = contentsX();
		int contsY = contentsY();
		int visw = visibleWidth();
		int vish = visibleHeight();
		int dtr = visw - (vx-contsX);
		int dtb = vish - (vy-contsY);
		int dtt =  (vy-contsY);
		int dtl =  (vx-contsX);
		if (dtr < 30) scrollBy(30-dtr,0);
		if (dtb < 30) scrollBy(0,30-dtb);
		if (dtl < 30) scrollBy(-(30-dtl),0);
		if (dtt < 30) scrollBy(0,-(30-dtt));
	}


	QMouseEvent *me = new QMouseEvent(QEvent::MouseMove,
					inverseWorldMatrix().map(ome->pos()),
					ome->button(),
					ome->state());

	m_LineToPos = me->pos();
	if( m_pFirstSelectedWidget ) {
		if( m_pAssocLine ) {
			QPoint sp = m_pAssocLine -> startPoint();
			m_pAssocLine -> setPoints( sp.x(), sp.y(), me->x(), me->y() );
		}
		return;
	}
	if(m_bDrawRect) {

		if( m_SelectionRect.count() == 4) {

			QCanvasLine * line = m_SelectionRect.at( 0 );
			line -> setPoints( m_Pos.x(), m_Pos.y(), me->x(), m_Pos.y() );

			line = m_SelectionRect.at( 1 );
			line -> setPoints( me->x(), m_Pos.y(), me->x(), me->y() );

			line = m_SelectionRect.at( 2 );
			line -> setPoints( me->x(), me->y(), m_Pos.x(), me->y() );

			line = m_SelectionRect.at( 3 );
			line -> setPoints( m_Pos.x(), me->y(), m_Pos.x(), m_Pos.y() );

			selectWidgets();
		}
	}

	allocateMouseMoveEvent(me);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
UMLWidget * UMLView::findWidget( int id, bool allowClassForObject ) {
	UMLWidget *obj, *objWidget = NULL;

	UMLWidgetListIt it( m_WidgetList );
	while ( (obj = it.current()) != 0 ) {
		++it;
		if( obj -> getBaseType() == wt_Object ) {
			if( static_cast<ObjectWidget *>( obj ) ->
			        getLocalID() == id ) {

				return obj;
			}
			if (allowClassForObject && obj->getID() == id)
				objWidget = (UMLWidget*) obj;
		} else if( obj -> getID() == id ) {
			return obj;
		}
	}
	if (objWidget)
		return objWidget;

	MessageWidgetListIt mit( m_MessageList );
	while ( (obj = (UMLWidget*)mit.current()) != 0 ) {
		++mit;
		if( obj -> getID() == id )
			return obj;
	}

	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
AssociationWidget * UMLView::findAssocWidget( int id ) {
	AssociationWidget *obj;
	AssociationWidgetListIt it( m_AssociationList );
	while ( (obj = it.current()) != 0 ) {
		++it;
		UMLAssociation* umlassoc = obj -> getAssociation();
		if ( umlassoc && umlassoc->getID() == id ) {
			return obj;
		}
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::removeWidget(UMLWidget * o) {
	if(!o)
		return;
	removeAssociations(o);

	UMLWidget_Type t = o->getBaseType();
	if(getType() == dt_Sequence && t == wt_Object)
		checkMessages( static_cast<ObjectWidget*>(o) );

	if( m_pOnWidget == o ) {
		m_pDoc -> getDocWindow() -> updateDocumentation( true );
		m_pOnWidget = 0;
	}

	o -> cleanup();
	m_SelectedList.remove(o);
	disconnect( this, SIGNAL( sigRemovePopupMenu() ), o, SLOT( slotRemovePopupMenu() ) );
	disconnect( this, SIGNAL( sigClearAllSelected() ), o, SLOT( slotClearAllSelected() ) );
	disconnect( this, SIGNAL(sigColorChanged(int)), o, SLOT(slotColorChanged(int)));
	m_WidgetList.remove(o);
	delete o;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::setFillColor(QColor color) {
	m_Options.uiState.fillColor = color;
	emit sigColorChanged( getID() );
	canvas()->setAllChanged();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::setLineColor(QColor color) {
	m_Options.uiState.lineColor = color;
	emit sigColorChanged( getID() );
	emit sigLineColorChanged( color );
	canvas() -> setAllChanged();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::contentsMouseDoubleClickEvent(QMouseEvent* ome) {

	QMouseEvent *me = new QMouseEvent(QEvent::MouseButtonDblClick,inverseWorldMatrix().map(ome->pos()),
					  ome->button(),ome->state());
	if ( allocateMouseDoubleClickEvent(me) ) {
		return;
	}
	clearSelected();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
QRect UMLView::getDiagramRect() {
	int startx, starty, endx, endy;
	startx = starty = INT_MAX;
	endx = endy = 0;
	UMLWidgetListIt it( m_WidgetList );
	UMLWidget *obj;
	while ( (obj = it.current()) != 0 ) {
		++it;
		if (! obj->isVisible())
			continue;
		int objEndX = static_cast<int>(obj -> x()) + obj -> width();
		int objEndY = static_cast<int>(obj -> y()) + obj -> height();
		int objStartX = static_cast<int>(obj -> x());
		int objStartY = static_cast<int>(obj -> y());
		if (startx >= objStartX)
			startx = objStartX;
		if (starty >= objStartY)
			starty = objStartY;
		if(endx <= objEndX)
			endx = objEndX;
		if(endy <= objEndY)
			endy = objEndY;
	}
	//if seq. diagram, make sure print all of the lines
	if(getType() == dt_Sequence ) {
		SeqLineWidget * pLine = 0;
		for( pLine = m_SeqLineList.first(); pLine; pLine = m_SeqLineList.next() ) {
			int y = pLine -> getObjectWidget() -> getEndLineY();
			endy = endy < y?y:endy;
		}

	}

	/* now we need another look at the associations, because they are no
	 * UMLWidgets */
	AssociationWidgetListIt assoc_it (m_AssociationList);
	AssociationWidget * assoc_obj;
	QRect rect;

	while ((assoc_obj = assoc_it.current()) != 0)
	{
		/* get the rectangle around all segments of the assoc */
		rect = assoc_obj->getAssocLineRectangle();

		if (startx >= rect.x())
			startx = rect.x();
		if (starty >= rect.y())
			starty = rect.y();
		if (endx <= rect.x() + rect.width())
			endx = rect.x() + rect.width();
		if (endy <= rect.y() + rect.height())
			endy = rect.y() + rect.height();
		++assoc_it; // next assoc
	}

	return QRect(startx, starty,  endx - startx, endy - starty);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::setSelected(UMLWidget * w, QMouseEvent * /*me*/) {
	//only add if wasn't in list

	if(!m_SelectedList.remove(w))
		m_SelectedList.append(w);
	int count = m_SelectedList.count();
	//only call once - if we select more, no need to keep clearing  window

	// if count == 1, widget will update the doc window with their data when selected
	if( count == 2 )
		updateDocumentation( true );//clear doc window
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::clearSelected() {
	m_SelectedList.clear();
	emit sigClearAllSelected();
	//m_pDoc -> enableCutCopy(false);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::moveSelected(UMLWidget * w, int x, int y) {
	QMouseEvent me(QMouseEvent::MouseMove, QPoint(x,y), LeftButton, ShiftButton);
	UMLWidget * temp = 0;
	//loop through list and move all widgets
	//don't move the widget that started call
	for(temp=(UMLWidget *)m_SelectedList.first();temp;temp=(UMLWidget *)m_SelectedList.next())
		if(temp != w)
			temp -> mouseMoveEvent(&me);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::selectionUseFillColor(bool useFC) {
	UMLWidget * temp = 0;
	for(temp=(UMLWidget *)m_SelectedList.first();temp;temp=(UMLWidget *)m_SelectedList.next())
		temp -> setUseFillColour(useFC);
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::selectionSetFont( QFont font )
{
	UMLWidget * temp = 0;
	for(temp=(UMLWidget *)m_SelectedList.first();temp;temp=(UMLWidget *)m_SelectedList.next())
		temp -> setFont( font );
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::selectionSetLineColor( QColor color )
{
	UMLWidget * temp = 0;
	for(temp=(UMLWidget *) m_SelectedList.first();
				temp;
					temp=(UMLWidget *)m_SelectedList.next()) {
		temp -> setLineColour( color );
		temp -> setUsesDiagramLineColour(false);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::selectionSetFillColor( QColor color )
{
	UMLWidget * temp = 0;
	for(temp=(UMLWidget *) m_SelectedList.first();
				temp;
					temp=(UMLWidget *)m_SelectedList.next()) {
		temp -> setFillColour( color );
		temp -> setUsesDiagramFillColour(false);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::selectionToggleShow(int sel)
{
	UMLWidget * temp = 0;

	// loop through all selected items
	for(temp=(UMLWidget *) m_SelectedList.first();
				temp;
					temp=(UMLWidget *)m_SelectedList.next()) {
		// toggle the show setting sel
		switch (sel)
		{
			// some setting are only avaible for class, some for interface and some
			// for both
		   case ListPopupMenu::mt_Show_Attributes_Selection:
				if (temp -> getBaseType() == wt_Class) {
					(static_cast<ClassWidget *> (temp)) -> toggleShowAtts();
				}
				break;
		   case ListPopupMenu::mt_Show_Operations_Selection:
				if (temp -> getBaseType() == wt_Class) {
					(static_cast<ClassWidget*> (temp)) -> toggleShowOps();
				} else if (temp -> getBaseType() == wt_Interface) {
					(static_cast<InterfaceWidget*> (temp)) -> toggleShowOps();
				}
				break;
		   case ListPopupMenu::mt_Scope_Selection:
				if (temp -> getBaseType() == wt_Class) {
					(static_cast<ClassWidget *> (temp)) -> toggleShowScope();
				} else if (temp -> getBaseType() == wt_Interface) {
					(static_cast<InterfaceWidget *> (temp)) -> toggleShowScope();
				}
				break;
		   case ListPopupMenu::mt_DrawAsCircle_Selection:
				if (temp -> getBaseType() == wt_Interface) {
					(static_cast<InterfaceWidget *> (temp)) -> toggleDrawAsCircle();
				}
				break;
		   case ListPopupMenu::mt_Show_Operation_Signature_Selection:
				if (temp -> getBaseType() == wt_Class) {
					(static_cast<ClassWidget *> (temp)) -> toggleShowOpSigs();
				} else if (temp -> getBaseType() == wt_Interface) {
					(static_cast<InterfaceWidget *> (temp)) -> toggleShowOpSigs();
				}
				break;
		   case ListPopupMenu::mt_Show_Attribute_Signature_Selection:
				if (temp -> getBaseType() == wt_Class) {
					(static_cast<ClassWidget *> (temp)) -> toggleShowAttSigs();
				}
				break;
	    	case ListPopupMenu::mt_Show_Packages_Selection:
				if (temp -> getBaseType() == wt_Class) {
					(static_cast<ClassWidget *> (temp)) -> toggleShowPackage();
				} else if (temp -> getBaseType() == wt_Interface) {
					(static_cast<InterfaceWidget *> (temp)) -> toggleShowPackage();
				}
				break;
	    	case ListPopupMenu::mt_Show_Stereotypes_Selection:
				if (temp -> getBaseType() == wt_Class) {
					(static_cast<ClassWidget *> (temp)) -> toggleShowStereotype();
				}
				break;
			default: break;
		} // switch (sel)
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::deleteSelection()
{
	/*
		Don't delete text widget that are connect to associations as these will
		be cleaned up by the associations.
	*/
	UMLWidget * temp = 0;
	for(temp=(UMLWidget *) m_SelectedList.first();
				temp;
					temp=(UMLWidget *)m_SelectedList.next())
	{
		if( temp -> getBaseType() == wt_Text &&
			((FloatingText *)temp) -> getRole() != tr_Floating )
		{
			temp -> hide();
		} else {
			removeWidget(temp);
		}
	}

	//now delete any selected associations
	AssociationWidgetListIt assoc_it( m_AssociationList );
	AssociationWidget* assocwidget = 0;
	while((assocwidget=assoc_it.current())) {
		++assoc_it;
		if( assocwidget-> getSelected() )
			removeAssoc(assocwidget);
	}

	/* we also have to remove selected messages from sequence diagrams */
	MessageWidget * cur_msgWgt;

	/* loop through all messages and check the selection state */
	for (cur_msgWgt = m_MessageList.first(); cur_msgWgt;
				cur_msgWgt = m_MessageList.next())
	{
		if (cur_msgWgt->getSelected() == true)
		{
			removeWidget(cur_msgWgt); // remove msg, because it is selected
		}
	}

	// sometimes we miss one widget, so call this function again to remove it as
	// well
	if (m_SelectedList.count() != 0)
		deleteSelection();

	//make sure list empty - it should be anyway, just a check.
	m_SelectedList.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::selectAll() {
	m_Pos.setX(0);
	m_Pos.setY(0);
	m_LineToPos.setX(canvas()->width());
	m_LineToPos.setY(canvas()->height());
	selectWidgets();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::contentsMousePressEvent(QMouseEvent* ome) {
	m_bMouseButtonPressed = true;
	QMouseEvent *me = new QMouseEvent(QEvent::MouseButtonPress,
					  inverseWorldMatrix().map(ome->pos()),
					  ome->button(),
					  ome->state());
	int x, y;
	if( m_pAssocLine ) {
		delete m_pAssocLine;
		m_pAssocLine = 0;
	}
	viewport()->setMouseTracking(true);
	emit sigRemovePopupMenu();

	//bit of a kludge to allow you to click over sequence diagram messages when
	//adding a new message
	if ( m_CurrentCursor != WorkToolBar::tbb_Seq_Message_Synchronous &&
	     m_CurrentCursor != WorkToolBar::tbb_Seq_Message_Asynchronous && allocateMousePressEvent(me) ) {
		return;
	}


	x = me->x();
	y = me->y();
	m_Pos.setX( x );
	m_Pos.setY( y );
	m_LineToPos.setX( x );
	m_LineToPos.setY( y );
	if(m_bPaste) {
		m_bPaste = false;
		//Execute m_bPaste action when pasting widget from another diagram
		//This needs to be changed to use UMLClipboard
		//clipboard -> m_bPaste(this, pos);
	}
	clearSelected();

	if(m_CurrentCursor != WorkToolBar::tbb_Arrow)
		return;
	for (int i = 0; i < 4; i++) {	//four lines needed for rect.
		QCanvasLine* line = new QCanvasLine( canvas() );
		line->setPoints(x, y, x, y);
		line->setPen( QPen(QColor("grey"), 0, DotLine) );
		line->setVisible(true);
		line->setZ(100);
		m_SelectionRect.append(line);
	}
	m_bDrawRect = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::makeSelected (UMLWidget * uw) {
	if (uw == NULL)
		return;
	uw -> setSelected(true);
	m_SelectedList.remove(uw);  // make sure not in there
	m_SelectedList.append(uw);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::selectWidgetsOfAssoc (AssociationWidget * a) {
	if (!a)
		return;
	a -> setSelected(true);
	//select the two widgets
	makeSelected( a->getWidgetA() );
	makeSelected( a->getWidgetB() );
	//select all the text
	makeSelected( a->getMultiAWidget() );
	makeSelected( a->getMultiBWidget() );
	makeSelected( a->getRoleAWidget() );
	makeSelected( a->getRoleBWidget() );
	makeSelected( a->getChangeWidgetA() );
	makeSelected( a->getChangeWidgetB() );
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::selectWidgets() {
	clearSelected();

	QRect rect;
	int px = m_Pos.x();
	int py = m_Pos.y();
	int qx = m_LineToPos.x();
	int qy = m_LineToPos.y();
	if(px <= qx) {
		rect.setLeft(px);
		rect.setRight(qx);
	} else {
		rect.setLeft(qx);
		rect.setRight(px);
	}
	if(py <= qy) {
		rect.setTop(py);
		rect.setBottom(qy);
	} else {
		rect.setTop(qy);
		rect.setBottom(py);
	}
	UMLWidgetListIt it(m_WidgetList);
	UMLWidget * temp = NULL;
	while ( (temp = it.current()) != 0 ) {
		int x = temp -> getX();
		int y =  temp -> getY();
		int w = temp -> getWidth();
		int h = temp -> getHeight();
		QRect rect2(x, y, w, h);
		++it;
		//see if any part of widget is in the rectangle
		//made of points pos and m_LineToPos
		if( !rect.intersects(rect2) )
			continue;
		//if it is text that is part of an association then select the association
		//and the objects that are connected to it.
		if(temp -> getBaseType() == wt_Text && ((FloatingText *)temp) -> getRole() != tr_Floating ) {


			int t = ((FloatingText *)temp) -> getRole();
			if( t == tr_Seq_Message ) {
				MessageWidget * mw = (MessageWidget *)((FloatingText *)temp) -> getMessage();
				makeSelected( mw );
				makeSelected( mw->getWidgetA() );
				makeSelected( mw->getWidgetB() );
			} else {
				AssociationWidget * a = static_cast<AssociationWidget *>( (static_cast<FloatingText *>(temp)) -> getAssoc() );
				selectWidgetsOfAssoc( a );
			}//end else
		}//end if text
		else if(temp -> getBaseType() == wt_Message) {
			UMLWidget * ow = ((MessageWidget *)temp) -> getWidgetA();
			makeSelected( ow );
			ow = ((MessageWidget *)temp) -> getWidgetB();
			makeSelected( ow );
		}
		if(temp -> isVisible()) {
			makeSelected( temp );
		}
	}
	selectAssociations( true );

	//now do the same for the messagewidgets
	MessageWidgetListIt itw( m_MessageList );
	MessageWidget *w = 0;
	while ( (w = itw.current()) != 0 ) {
		++itw;
		if( w -> getWidgetA() -> getSelected() &&
		        w -> getWidgetB() -> getSelected() ) {
			makeSelected( w );
		}//end if
	}//end while
}
////////////////////////////////////////////////////////////////////////////////////////////////////
void  UMLView::getDiagram(const QRect &rect, QPixmap & diagram) {
	QPixmap pixmap(rect.x() + rect.width(), rect.y() + rect.height());
	QPainter painter(&pixmap);
	getDiagram(canvas()->rect(),painter);
	bitBlt(&diagram, QPoint(0, 0), &pixmap, rect);
}

void  UMLView::getDiagram(const QRect &area, QPainter & painter) {
	//unselect all before grab
	UMLWidget* temp = 0;
	for (temp=(UMLWidget *)m_SelectedList.first();temp;temp=(UMLWidget *)m_SelectedList.next()) {
		temp -> setSelected(false);
	}
	if (m_pMoveAssoc) {
		m_pMoveAssoc -> setSelected( false );
	}

	// we don't want to get the grid
	bool showSnapGrid = getShowSnapGrid();
	setShowSnapGrid(false);

	canvas()->drawArea(area, &painter);

	setShowSnapGrid(showSnapGrid);

	canvas()->setAllChanged();
	//select again
	for (temp=(UMLWidget *)m_SelectedList.first();temp;temp=(UMLWidget *)m_SelectedList.next()) {
		temp -> setSelected( true );
	}
	if (m_pMoveAssoc) {
		m_pMoveAssoc -> setSelected( true );
	}
	return;
}

void UMLView::updateNoteWidgets() {
	AssociationWidget * a = 0;

	AssociationWidgetListIt assoc_it( m_AssociationList );
	while((a = assoc_it.current())) {
		++assoc_it;
		if(a->getAssocType() != at_Anchor)
			continue;
		UMLWidget *wa = a->getWidgetA();
		UMLWidget *wb = a->getWidgetB();
		UMLWidget *destination= 0, *source = 0;
		bool copyText = false;
		if(wa->getBaseType() == wt_Note) {
			kdDebug() << "A note" << endl;
			source = wb;
			destination = wa;
			copyText = true;
		} else if(wb->getBaseType() == wt_Note) {
			kdDebug() << "B note" << endl;
			source = wa;
			destination = wb;
			copyText = true;
		}

		if(copyText == true && ((NoteWidget*)destination)->getLinkState() == true
		        && source->getUMLObject()->getDoc() != NULL) {
			((NoteWidget*)destination)->setDoc(source->getUMLObject()->getDoc());

		}
	}
}

QString imageTypeToMimeType(QString imagetype) {
	if (QString("BMP") == imagetype) return "image/x-bmp";
	if (QString("JPEG") == imagetype) return "image/jpeg";
	if (QString("PBM") == imagetype) return "image/x-portable-bitmap";
	if (QString("PGM") == imagetype) return "image/x-portable-greymap";
	if (QString("PNG") == imagetype) return "image/png";
	if (QString("PPM") == imagetype) return "image/x-portable-pixmap";
	if (QString("XBM") == imagetype) return "image/x-xbm";
	if (QString("XPM") == imagetype) return "image/x-xpm";
	if (QString("EPS") == imagetype) return "image/x-eps";
	return QString::null;
}

QString mimeTypeToImageType(QString mimetype) {
	if (QString("image/x-bmp") == mimetype) return "BMP";
	if (QString("image/jpeg") == mimetype) return "JPEG";
	if (QString("image/x-portable-bitmap") == mimetype) return "PBM";
	if (QString("image/x-portable-greymap") == mimetype) return "PGM";
	if (QString("image/png") == mimetype) return "PNG";
	if (QString("image/x-portable-pixmap") == mimetype) return "PPM";
	if (QString("image/x-xbm") == mimetype) return "XBM";
	if (QString("image/x-xpm") == mimetype) return "XPM";
	if (QString("image/x-eps") == mimetype) return "EPS";
	return QString::null;
}

void UMLView::fixEPS(QString filename, QRect rect) {
	// now open the file and make a correct eps out of it
	QFile epsfile(filename);
	QString fileContent;
	if (epsfile.open(IO_ReadOnly )) {
		// read
		QTextStream ts(&epsfile);
		fileContent = ts.read();
		epsfile.close();

		// write new content to file
		if (epsfile.open(IO_WriteOnly | IO_Truncate)) {
			// read information
			QRegExp rx("%%BoundingBox:\\s*(-?[\\d\\.]+)\\s*(-?[\\d\\.]+)\\s*(-?[\\d\\.]+)\\s*(-?[\\d\\.]+)");
			int pos = rx.search(fileContent);
			float left = rx.cap(1).toFloat();
			float top = rx.cap(4).toFloat();

			// modify content
			fileContent.replace(pos,rx.cap(0).length(),
					    QString("%%BoundingBox: %1 %2 %3 %4").arg(left).arg(top-rect.height()).arg(left+rect.width()).arg(top));

			ts << fileContent;
			epsfile.close();
		}
	}
}

void UMLView::printToFile(QString filename,bool isEPS) {
	// print the image to a normal postscript file,
	// do not clip so that everything ends up in the file
	// regardless of "paper size"

	// because we want to work with postscript
	// user-coordinates, set to the resolution
	// of the printer (which should be 72dpi here)
	QPrinter *printer = new QPrinter(QPrinter::PrinterResolution);
	printer->setOutputToFile(true);
	printer->setOutputFileName(filename);

	// do not call printer.setup(); because we want no user
	// interaction here
	QPainter *painter = new QPainter(printer);

	// make sure the widget sizes will be according to the
	// actually used printer font, important for getDiagramRect()
	// and the actual painting
	forceUpdateWidgetFontMetrics(painter);

	QRect rect = getDiagramRect();
	painter->translate(-rect.x(),-rect.y());
	getDiagram(rect,*painter);

	// delete painter and printer before we try to open and fix the file
	delete painter;
	delete printer;
	if (isEPS) fixEPS(filename,rect);
	// next painting will most probably be to a different device (i.e. the screen)
	forceUpdateWidgetFontMetrics(0);

}

void UMLView::exportImage() {
	UMLApp *app = UMLApp::app();
	QStringList fmt = QImage::outputFormatList();
	QString imageMimetype = "image/png";

	// get all supported mimetypes
	QStringList mimetypes;
	// special image types that are handled separately
	mimetypes.append("image/x-eps");
	// "normal" image types that are present
	QString m;
	QStringList::Iterator it;
	for( it = fmt.begin(); it != fmt.end(); ++it ) {
		m = imageTypeToMimeType(*it);
		if (!m.isNull()) mimetypes.append(m);
	}

	// configure & show the file dialog
	KFileDialog fileDialog(QString::null, QString::null, this,
			       ":export-image",true);
	fileDialog.setCaption(i18n("Save As"));
	fileDialog.setOperationMode(KFileDialog::Saving);
	if (app)
		imageMimetype = app->getImageMimetype();
	fileDialog.setMimeFilter(mimetypes, imageMimetype);

	// set a sensible default filename
	if (m_ImageURL.isEmpty()) {
	  fileDialog.setSelection(getName()+"."+mimeTypeToImageType(imageMimetype).lower());
	} else {
	  fileDialog.setURL(m_ImageURL);
	  fileDialog.setSelection(m_ImageURL.fileName());
	}
	fileDialog.exec();

	if (fileDialog.selectedURL().isEmpty())
		return;
	// save
	imageMimetype = fileDialog.currentMimeFilter();
	if (app) app->setImageMimetype(imageMimetype);
	m_ImageURL = fileDialog.selectedURL();

	QString s;
	KTempFile tmpfile;

	if (m_ImageURL.isLocalFile()) {
		QString file = m_ImageURL.path(-1);
		QFileInfo info(file);
		QString ext = info.extension(false);
		QString extDef = mimeTypeToImageType(imageMimetype).lower();
		if(ext != extDef) {
			m_ImageURL.setFileName(m_ImageURL.fileName() + "."+extDef);
		}
		s = m_ImageURL.path();
		info = QFileInfo(s);
		if (info.exists())
		{
			int want_save = KMessageBox::questionYesNo(0, i18n("The selected file %1 exists.\nDo you want to overwrite it?").arg(m_ImageURL.fileName()),
								i18n("File Already Exists"),
								i18n("Yes"), i18n("No"));
			if (want_save == KMessageBox::No)
				// another possibility would be to show the save dlg again
				return;
		}
	} else {
		s = tmpfile.name();
	}

	QRect rect = getDiagramRect();
	if (rect.isEmpty()) {
		KMessageBox::sorry(0, i18n("Can not save an empty diagram"),
		                   i18n("Diagram Save Error!"));
	} else {
		//  eps requested
		if (imageMimetype == "image/x-eps") {
			printToFile(s,true);
		}else{
			QPixmap diagram(rect.width(), rect.height());
			getDiagram(rect, diagram);
			diagram.save(s, mimeTypeToImageType(imageMimetype).ascii());
		}
		if (!m_ImageURL.isLocalFile()) {
			if(!KIO::NetAccess::upload(tmpfile.name(), m_ImageURL)) {
				KMessageBox::error(0,
				                   i18n("There was a problem saving file: %1").arg(m_ImageURL.path()),
				                   i18n("Save Error"));
			}
			tmpfile.unlink();
		} //!isLocalFile
	} //rect.isEmpty()
}//exportImage()

void UMLView::slotActivate() {
	m_pDoc->changeCurrentView(getID());
}

UMLObjectList* UMLView::getUMLObjects() {
	int type;
	UMLObjectList* list = new UMLObjectList;
	list->setAutoDelete(FALSE);

	UMLWidgetListIt it( m_WidgetList );

	UMLWidget *obj;
	while ( (obj = it.current()) != 0 ) {
		++it;
		type = obj -> getBaseType();
		switch( type ) //use switch for easy future expansion
		{
			case wt_Actor:
			case wt_Class:
			case wt_Package:
			case wt_Component:
			case wt_Node:
			case wt_Artifact:
			case wt_UseCase:
			case wt_Object:
				list->append( obj->getUMLObject() );
				break;
		}
	}
	return list;
}

bool UMLView::activate() {
	UMLWidgetListIt it( m_WidgetList );
	UMLWidget *obj;

	//Activate Regular widgets then activate  messages
	while ( (obj = it.current()) != 0 ) {
		++it;
		//If this UMLWidget is already activated or is a MessageWidget then skip it
		if(obj->isActivated() || obj->getBaseType() == wt_Message)
			continue;
		if(!obj->activate())
			continue;
		obj -> setVisible( true );
	}//end while

	MessageWidgetListIt it2( m_MessageList );
	//Activate Message widgets
	while ( (obj = (UMLWidget*)it2.current()) != 0 ) {
		++it2;
		//If this MessageWidget is already activated then skip it
		if(obj->isActivated())
			continue;

		if(!obj->activate(m_pDoc->getChangeLog())) {
			kdDebug() << "Couldn't activate message widget" << endl;
			continue;
		}
		obj -> setVisible( true );

	}//end while

	//Activate All associationswidgets
	AssociationWidgetListIt assoc_it( m_AssociationList );
	AssociationWidget *assocwidget;
	//first get total count
	while((assocwidget = assoc_it.current())) {
		++assoc_it;
		if( assocwidget->isActivated() )
			continue;
		assocwidget->activate();
		if( m_PastePoint.x() != 0 ) {
			int x = m_PastePoint.x() - m_Pos.x();
			int y = m_PastePoint.y() - m_Pos.y();
			assocwidget -> moveEntireAssoc( x, y );
		}
	}//end while
	return true;
}


bool UMLView::getSelectedWidgets(UMLWidgetList&WidgetList)
{
	UMLWidget * temp = 0;
	int type;
	for(temp=(UMLWidget *)m_SelectedList.first();temp;temp=(UMLWidget *)m_SelectedList.next()) {
		type = temp->getBaseType();
		if (type == wt_Text) {
			if( ((FloatingText*)temp)->getRole() == tr_Floating ) {
				WidgetList.append(temp);
			}
		} else {
			WidgetList.append(temp);
		}
	}//end for
	return true;
}

bool UMLView::getSelectedAssocs(AssociationWidgetList & assocWidgetList) {
	AssociationWidgetListIt assoc_it( m_AssociationList );
	AssociationWidget* assocwidget = 0;
	while((assocwidget=assoc_it.current())) {
		++assoc_it;
		if( assocwidget -> getSelected() )
			assocWidgetList.append(assocwidget);
	}
	return true;
}

bool UMLView::addWidget( UMLWidget * pWidget ) {
	if( !pWidget ) {
		return false;
	}
	UMLWidget_Type type = pWidget->getBaseType();
	kdDebug() << "UMLView::addWidget called for basetype " << type << endl;
	IDChangeLog * log = m_pDoc -> getChangeLog();
	if( !log || !m_pIDChangesLog) {
		kdDebug()<<"A log is not open"<<endl;
		return false;

	}
	if( pWidget -> getX() < m_Pos.x() )
		m_Pos.setX( pWidget -> getX() );
	if( pWidget -> getY() < m_Pos.y() )
		m_Pos.setY( pWidget -> getY() );

	//see if we need a new id to match object
	switch( type ) {

		case wt_Class:
		case wt_Package:
		case wt_Component:
		case wt_Node:
		case wt_Artifact:
		case wt_Interface:
		case wt_Actor:
		case wt_UseCase:
			{
				int newID = log->findNewID( pWidget -> getID() );
				if( newID == -1 )
					return false;
				pWidget -> setID( newID );
				UMLObject * pObject = m_pDoc -> findUMLObject( newID );
				if( !pObject ) {
					kdDebug() << "addWidget: Can't find UMLObject for id "
						  << newID << endl;
					return false;
				}
				pWidget -> setUMLObject( pObject );
				//make sure it doesn't already exist.
				if( findWidget( newID ) )
					return true;//don't stop paste just because widget found.
				m_WidgetList.append( pWidget );
			}
			break;

		case wt_Message:
		case wt_Note:
		case wt_Box:
		case wt_Text:
		case wt_State:
		case wt_Activity:
			{
				int newID = m_pDoc->assignNewID( pWidget->getID() );
				pWidget->setID(newID);
				if (type != wt_Message) {
					m_WidgetList.append( pWidget );
					return true;
				}
				// CHECK
				// Handling of wt_Message:
				MessageWidget *pMessage = static_cast<MessageWidget *>( pWidget );
				if (pMessage == NULL) {
					kdDebug() << "UMLView::addWidget(): pMessage is NULL" << endl;
					return false;
				}
				ObjectWidget *objWidgetA = pMessage -> getWidgetA();
				ObjectWidget *objWidgetB = pMessage -> getWidgetB();
				int waID = objWidgetA -> getLocalID();
				int wbID = objWidgetB -> getLocalID();
				int newWAID = m_pIDChangesLog ->findNewID( waID );
				int newWBID = m_pIDChangesLog ->findNewID( wbID );
				if( newWAID == -1 || newWBID == -1 ) {
					kdDebug() << "Error with ids : " << newWAID << " " << newWBID << endl;
					return false;
				}
				// Assumption here is that the A/B objectwidgets and the textwidget
				// are pristine in the sense that we may freely change their local IDs.
				objWidgetA -> setLocalID( newWAID );
				objWidgetB -> setLocalID( newWBID );
				FloatingText *ft = pMessage->getFloatingText();
				if (ft == NULL)
					kdDebug() << "UMLView::addWidget: FloatingText of Message is NULL" << endl;
				else if (ft->getID() == -1)
					ft->setID( m_pDoc->getUniqueID() );
				else {
					int newTextID = m_pDoc->assignNewID( ft->getID() );
					ft->setID( newTextID );
				}
				m_MessageList.append( pMessage );
			}
			break;

		case wt_Object:
			{
				ObjectWidget* pObjectWidget = static_cast<ObjectWidget*>(pWidget);
				if (pObjectWidget == NULL) {
					kdDebug() << "UMLView::addWidget(): pObjectWidget is NULL" << endl;
					return false;
				}
				int newID = log->findNewID( pWidget -> getID() );
				if (newID == -1) {
					return false;
				}
				pObjectWidget -> setID( newID );
				int nNewLocalID = getLocalID();
				int nOldLocalID = pObjectWidget -> getLocalID();
				m_pIDChangesLog->addIDChange( nOldLocalID, nNewLocalID );
				pObjectWidget -> setLocalID( nNewLocalID );
				UMLObject *pObject = m_pDoc -> findUMLObject( newID );
				if( !pObject ) {
					kdDebug() << "addWidget::Can't find UMLObject" << endl;
					return false;
				}
				pWidget -> setUMLObject( pObject );
				m_WidgetList.append( pWidget );
			}
			break;

		default:
			kdDebug() << "Trying to add an invalid widget type" << endl;
			return false;
			break;
	}

	return true;
}

bool UMLView::addAssociation( AssociationWidget* pAssoc ) {
	if(!pAssoc)
		return false;

	IDChangeLog * log = m_pDoc -> getChangeLog();

	if( !log )
		return false;

/* What is this code doing? Somebody please enlighten me  --okellogg
	int ida = -1, idb = -1;
	Association_Type type = pAssoc -> getAssocType();
	IDChangeLog* localLog = getLocalIDChangeLog();
	if( getType() == dt_Collaboration || getType() == dt_Sequence ) {
		//check local log first
		ida = localLog->findNewID( pAssoc->getWidgetAID() );
		idb = localLog->findNewID( pAssoc->getWidgetBID() );
		//if either is still not found and assoc type is anchor
		//we are probably linking to a notewidet - else an error
		if( ida == -1 && type == at_Anchor )
			ida = log->findNewID(pAssoc->getWidgetAID());
		if( idb == -1 && type == at_Anchor )
			idb = log->findNewID(pAssoc->getWidgetBID());
	} else {
		ida = log->findNewID(pAssoc->getWidgetAID());
		idb = log->findNewID(pAssoc->getWidgetBID());
	}
	if(ida == -1 || idb == -1) {
		return false;
	}
	pAssoc->setWidgetAID(ida);
	pAssoc->setWidgetBID(idb);
 */

	UMLWidget * m_pWidgetA = findWidget(pAssoc->getWidgetAID());
	UMLWidget * m_pWidgetB = findWidget(pAssoc->getWidgetBID());
	//make sure valid widget ids

	if(!m_pWidgetA || !m_pWidgetB)
		return false;

	//make sure valid
	if( !AssocRules::allowAssociation( pAssoc -> getAssocType(), m_pWidgetA, m_pWidgetB ) ) {
		return true;//in a paste we still need to be true
	}
	//make sure there isn't already the same assoc
	AssociationWidgetListIt assoc_it( m_AssociationList );

	AssociationWidget* assocwidget = 0;
	while((assocwidget=assoc_it.current())) {
		++assoc_it;
		if( *pAssoc == *assocwidget )
			return true;
	}

	addAssocInViewAndDoc(pAssoc);
	return true;
}


void UMLView::addAssocInViewAndDoc(AssociationWidget* a) {
	UMLAssociation *umla = a->getAssociation();

	// append in view
	m_AssociationList.append(a);

	// append in document
	getDocument() -> addAssociation (umla);

	FloatingText *pNameWidget = a->getNameWidget();
	if (pNameWidget != NULL && pNameWidget->getID() == -1) {
		pNameWidget->setID( umla->getID() );
	}
}

bool UMLView::activateAfterLoad(bool bUseLog) {
	bool status = true;
	if ( !m_bActivated ) {
		if( bUseLog ) {
			beginPartialWidgetPaste();
		}

		//now activate them all
		status = activate();

		if( bUseLog ) {
			endPartialWidgetPaste();
		}
		resizeCanvasToItems();
		setZoom( getZoom() );
	}//end if active
	if(status) {
		m_bActivated = true;
	}
	return true;
}

void UMLView::beginPartialWidgetPaste() {
	delete m_pIDChangesLog;
	m_pIDChangesLog = 0;

	m_pIDChangesLog = new IDChangeLog();
	m_bPaste = true;
}

void UMLView::endPartialWidgetPaste() {
	delete    m_pIDChangesLog;
	m_pIDChangesLog = 0;

	m_bPaste = false;
}

IDChangeLog* UMLView::getLocalIDChangeLog() {
	return m_pIDChangesLog;
}

void UMLView::removeAssoc(AssociationWidget* pAssoc) {
	if(!pAssoc)
		return;
	if( pAssoc == m_pMoveAssoc ) {
		m_pDoc -> getDocWindow() -> updateDocumentation( true );
		m_pMoveAssoc = 0;
	}
	removeAssocInViewAndDoc(pAssoc, true);
}

void UMLView::removeAssocInViewAndDoc(AssociationWidget* a, bool /* deleteLater */) {
	if(!a)
		return;
	// Remove the association from the UMLDoc.
	// NO! now done by the association->cleanup.
	// m_pDoc->removeAssociation(a->getAssociation());

	// Remove the association in this view.
	a->cleanup();
	m_AssociationList.remove(a);
/*	if (deleteLater) {
		a->deleteLater();
	}
	else
		delete a;
 */
}

bool UMLView::setAssoc(UMLWidget *pWidget) {
	Association_Type type = convert_TBB_AT(m_CurrentCursor);
	m_bDrawRect = false;
	m_SelectionRect.clear();
	//if this we are not concerned here so return
	if(m_CurrentCursor < WorkToolBar::tbb_Generalization || m_CurrentCursor > WorkToolBar::tbb_Anchor) {
		return false;
	}
	clearSelected();

	if(!m_pFirstSelectedWidget) {
		if( !AssocRules::allowAssociation( type, pWidget ) ) {
			KMessageBox::error(0, i18n("Incorrect use of associations."), i18n("Association Error"));
			return false;
		}
		//set up position
		QPoint pos;
		pos.setX(pWidget -> getX() + (pWidget->getWidth() / 2));

		pos.setY(pWidget -> getY() + (pWidget->getHeight() / 2));
		setPos(pos);
		m_LineToPos = pos;
		m_pFirstSelectedWidget = pWidget;
		viewport() -> setMouseTracking( true );
		if( m_pAssocLine )
			delete m_pAssocLine;
		m_pAssocLine = 0;
		m_pAssocLine = new QCanvasLine( canvas() );
		m_pAssocLine -> setPoints( pos.x(), pos.y(), pos.x(), pos.y() );
		m_pAssocLine -> setPen( QPen( getLineColor(), 0, DashLine ) );

		m_pAssocLine -> setVisible( true );

		return true;
	}
	// If we get here we have a FirstSelectedWidget.
	// The following reassignment is just to make things clearer.
	UMLWidget* widgetA = m_pFirstSelectedWidget;
	UMLWidget* widgetB = pWidget;
	UMLWidget_Type at = widgetA -> getBaseType();
	bool valid = true;
	if (type == at_Generalization)
		type = AssocRules::isGeneralisationOrRealisation(widgetA, widgetB);
	if (widgetA == widgetB)
		valid = AssocRules::allowSelf( type, at );
	else
		valid =  AssocRules::allowAssociation( type, widgetA, widgetB );
	if( valid ) {
		AssociationWidget *temp = new AssociationWidget(this, widgetA, type, widgetB);
		addAssocInViewAndDoc(temp);
	} else {
		KMessageBox::error(0, i18n("Incorrect use of associations."), i18n("Association Error"));
	}
	m_pFirstSelectedWidget = 0;

	if( m_pAssocLine ) {
		delete m_pAssocLine;
		m_pAssocLine = 0;
	}
	return valid;
}

/** Removes all the associations related to Widget */
void UMLView::removeAssociations(UMLWidget* Widget) {

	AssociationWidgetListIt assoc_it(m_AssociationList);
	AssociationWidget* assocwidget = 0;
	while((assocwidget=assoc_it.current())) {
		++assoc_it;
		if(assocwidget->contains(Widget)) {
			removeAssoc(assocwidget);
		}
	}
}

void UMLView::selectAssociations(bool bSelect) {
	AssociationWidgetListIt assoc_it(m_AssociationList);
	AssociationWidget* assocwidget = 0;
	while((assocwidget=assoc_it.current())) {
		++assoc_it;
		if(bSelect &&
		  assocwidget->getWidgetA() && assocwidget->getWidgetA()->getSelected() &&
		  assocwidget->getWidgetB() && assocwidget->getWidgetB()->getSelected() ) {
			assocwidget->setSelected(true);
		} else {
			assocwidget->setSelected(false);
		}
	}//end while
}

void UMLView::getWidgetAssocs(UMLObject* Obj, AssociationWidgetList & Associations) {
	if( ! Obj )
		return;

	AssociationWidgetListIt assoc_it(m_AssociationList);
	AssociationWidget * assocwidget;
	while((assocwidget = assoc_it.current())) {
		if(assocwidget->getWidgetA()->getUMLObject() == Obj
		        || assocwidget->getWidgetB()->getUMLObject() == Obj)
			Associations.append(assocwidget);
		++assoc_it;
	}//end while
}

void UMLView::closeEvent ( QCloseEvent * e ) {
	QWidget::closeEvent(e);
}

void UMLView::removeAllAssociations() {
	//Remove All associations
	AssociationWidgetListIt assoc_it(m_AssociationList);
	AssociationWidget* assocwidget = 0;
	while((assocwidget=assoc_it.current()))
	{
		++assoc_it;
		removeAssocInViewAndDoc(assocwidget);
	}
	m_AssociationList.clear();
}


void UMLView::removeAllWidgets() {
	// Remove widgets.
	UMLWidgetListIt it( m_WidgetList );
	UMLWidget * temp = 0;
	while ( (temp = it.current()) != 0 ) {
		++it;
		// if( !( temp -> getBaseType() == wt_Text && ((FloatingText *)temp)-> getRole() != tr_Floating ) ) {
			removeWidget( temp );
		// }
	}
	m_WidgetList.clear();
}


WorkToolBar::ToolBar_Buttons UMLView::getCurrentCursor() const {
	return m_CurrentCursor;
}

Uml::Association_Type UMLView::convert_TBB_AT(WorkToolBar::ToolBar_Buttons tbb) {
	Association_Type at = at_Unknown;
	switch(tbb) {
		case WorkToolBar::tbb_Anchor:
			at = at_Anchor;
			break;

		case WorkToolBar::tbb_Association:
			at = at_Association;
			break;

		case WorkToolBar::tbb_UniAssociation:
			at = at_UniAssociation;

			break;

		case WorkToolBar::tbb_Generalization:
			at = at_Generalization;
			break;

		case WorkToolBar::tbb_Composition:
			at = at_Composition;
			break;

		case WorkToolBar::tbb_Aggregation:
			at = at_Aggregation;
			break;

		case WorkToolBar::tbb_Dependency:
			at = at_Dependency;
			break;

		case WorkToolBar::tbb_Seq_Message_Synchronous:
		case WorkToolBar::tbb_Seq_Message_Asynchronous:
			at = at_Seq_Message;
			break;

		case WorkToolBar::tbb_Coll_Message:
			at = at_Coll_Message;
			break;

		case WorkToolBar::tbb_State_Transition:
			at = at_State;
			break;

		case WorkToolBar::tbb_Activity_Transition:
			at = at_Activity;
			break;

	        default:
			break;
	}
	return at;
}

Uml::UMLObject_Type UMLView::convert_TBB_OT(WorkToolBar::ToolBar_Buttons tbb) {
	UMLObject_Type ot = ot_UMLObject;
	switch(tbb) {
		case WorkToolBar::tbb_Actor:
			ot = ot_Actor;
			break;

		case WorkToolBar::tbb_UseCase:
			ot = ot_UseCase;
			break;

		case WorkToolBar::tbb_Class:
			ot = ot_Class;
			break;

		case WorkToolBar::tbb_Package:
			ot = ot_Package;
			break;

		case WorkToolBar::tbb_Component:
			ot = ot_Component;
			break;

		case WorkToolBar::tbb_Node:
			ot = ot_Node;
			break;

		case WorkToolBar::tbb_Artifact:
			ot = ot_Artifact;
			break;

		case WorkToolBar::tbb_Interface:
			ot = ot_Interface;
			break;
		default:
			break;
	}
	return ot;
}

bool UMLView::allocateMousePressEvent(QMouseEvent * me) {
	m_pMoveAssoc = 0;
	m_pOnWidget = 0;

	UMLWidget* backup = 0;
	UMLWidget* boxBackup = 0;

	// Check widgets.
	UMLWidgetListIt it( m_WidgetList );
	UMLWidget* obj = 0;
	while ( (obj = it.current()) != 0 ) {
		++it;
		if( !obj->isVisible() || !obj->onWidget(me->pos()) )
			continue;
		//Give text object priority,
		//they can easily get into a position where
		//you can't touch them.
		//Give Boxes lowest priority, we want to be able to move things that
		//are on top of them.
		if( obj -> getBaseType() == wt_Text ) {
			m_pOnWidget = obj;
			obj ->  mousePressEvent( me );
			return true;
		} else if (obj->getBaseType() == wt_Box) {
			boxBackup = obj;
		} else {
			backup = obj;
		}
	}//end while
	//if backup is set then let it have the event
	if(backup) {
		backup -> mousePressEvent( me );
		m_pOnWidget = backup;
		return true;
	}

	// Check messages.
	MessageWidgetListIt mit( m_MessageList );
	obj = 0;
	while ((obj = (UMLWidget*)mit.current()) != 0) {
		if (obj->isVisible() && obj->onWidget(me->pos())) {
			m_pOnWidget = obj;
			obj ->  mousePressEvent( me );
			return true;
		}
		++mit;
	}

	// Boxes have lower priority.
	if (boxBackup) {
		boxBackup -> mousePressEvent( me );
		m_pOnWidget = boxBackup;
		return true;
	}

	// Check associations.
	AssociationWidgetListIt assoc_it(m_AssociationList);
	AssociationWidget* assocwidget = 0;
	while((assocwidget=assoc_it.current())) {
		if( assocwidget -> onAssociation( me -> pos() )) {
			assocwidget->mousePressEvent(me);
			m_pMoveAssoc = assocwidget;
			return true;
		}
		++assoc_it;
	}
	m_pMoveAssoc = 0;
	m_pOnWidget = 0;
	return false;
}

bool UMLView::allocateMouseReleaseEvent(QMouseEvent * me) {
	//values will already be set through press event.
	//may not be over it, but should still get the event.

	//cursor tests are to stop this being wrongly used when
	//adding a message to a sequence diagram when the object is selected
	if( m_pOnWidget &&
	    !(m_CurrentCursor == WorkToolBar::tbb_Seq_Message_Synchronous ||
	      m_CurrentCursor == WorkToolBar::tbb_Seq_Message_Asynchronous) ) {
		m_pOnWidget -> mouseReleaseEvent( me );
		return true;
	}

	if( m_pMoveAssoc ) {
		m_pMoveAssoc -> mouseReleaseEvent( me );
		return true;
	}

	m_pOnWidget = 0;
	m_pMoveAssoc = 0;
	return false;
}

bool UMLView::allocateMouseDoubleClickEvent(QMouseEvent * me) {
	//values will already be set through press and release events
	if( m_pOnWidget && m_pOnWidget -> onWidget( me -> pos() )) {
		m_pOnWidget -> mouseDoubleClickEvent( me );
		return true;
	}
	if( m_pMoveAssoc && m_pMoveAssoc -> onAssociation( me -> pos() )) {
		m_pMoveAssoc -> mouseDoubleClickEvent( me );
		return true;
	}
	m_pOnWidget = 0;
	m_pMoveAssoc = 0;
	return false;
}

bool UMLView::allocateMouseMoveEvent(QMouseEvent * me) {
	//tracking may have been set by someone else
	//only move if we set it.  We only want it if
	//left mouse button down.
	if(m_pOnWidget) {
		m_pOnWidget -> mouseMoveEvent( me );
		return true;
	}
	if(m_pMoveAssoc) {
		m_pMoveAssoc -> mouseMoveEvent( me );
		return true;
	}
	return false;
}



void UMLView::showDocumentation( UMLObject * object, bool overwrite ) {
	m_pDoc -> getDocWindow() -> showDocumentation( object, overwrite );
}

void UMLView::showDocumentation( UMLWidget * widget, bool overwrite ) {
	m_pDoc -> getDocWindow() -> showDocumentation( widget, overwrite );
}

void UMLView::showDocumentation( AssociationWidget * widget, bool overwrite ) {
	m_pDoc -> getDocWindow() -> showDocumentation( widget, overwrite );
}

void UMLView::updateDocumentation( bool clear ) {
	m_pDoc -> getDocWindow() -> updateDocumentation( clear );
}

void UMLView::createAutoAssociations( UMLWidget * widget ) {
	if( !widget )
		return;
	AssociationWidget * docData = 0;

	UMLWidget * widgetA = 0;
	UMLWidget * widgetB = 0;

	AssociationWidgetList list;

	m_pDoc -> getAssciationListAllViews( this, widget -> getUMLObject(), list );
	AssociationWidgetListIt it( list );

	while( ( docData = it.current() ) ) {
		++it;
		if( docData -> getAssocType() == at_Anchor )
			continue;
		//see if both widgets are on this diagram
		//if yes - create an association
		widgetA = widget;
		widgetB = findWidget( docData -> getWidgetBID() );
		if( docData -> getWidgetAID() != widget -> getID() ) {
			widgetA = findWidget( docData -> getWidgetAID() );
			widgetB = widget;
		}

		if( widgetA && widgetB ) {
			AssociationWidget * temp = new AssociationWidget( this, widgetA ,
			                            docData -> getAssocType(), widgetB );
			addAssocInViewAndDoc( temp );
		}
	}//end while docAssoc
}

void UMLView::copyAsImage(QPixmap*& pix) {

	int px, py, qx, qy;
	int x, y, x1, y1;
	//get the smallest rect holding the diagram
	QRect rect = getDiagramRect();
	QPixmap diagram( rect.width(), rect.height() );

	//only draw what is selected
	m_bDrawSelectedOnly = true;
	selectAssociations(true);
	getDiagram(rect, diagram);

	//now get the selection cut
	//first get the smallest rect holding the widgets
	px = py = qx = qy = -1;
	UMLWidget * temp = 0;

	for(temp=(UMLWidget *)m_SelectedList.first();temp;temp=(UMLWidget *)m_SelectedList.next()) {
		x = temp -> getX();
		y = temp -> getY();
		x1 = x + temp -> width() - 1;
		y1 = y + temp -> height() - 1;
		if(px == -1 || x < px) {
			px = x;
		}
		if(py == -1 || y < py) {
			py = y;
		}
		if(qx == -1 || x1 > qx) {
			qx = x1;
		}
		if(qy == -1 || y1 > qy) {
			qy = y1;
		}
	}

	//also take into account any text lines in assocs or messages
	AssociationWidget *assocwidget;
	AssociationWidgetListIt assoc_it(m_AssociationList);

	//get each type of associations
	//This needs to be reimplemented to increase the rectangle
	//if a part of any association is not included
	while((assocwidget = assoc_it.current()))
	{
		++assoc_it;
		if(assocwidget->getSelected())
		{
			temp = const_cast<FloatingText*>(assocwidget->getMultiAWidget());
			if(temp && temp->isVisible()) {
				x = temp->getX();
				y = temp->getY();
				x1 = x + temp->getWidth() - 1;
				y1 = y + temp->getHeight() - 1;

				if (px == -1 || x < px) {
					px = x;
				}
				if (py == -1 || y < py) {
					py = y;
				}
				if (qx == -1 || x1 > qx) {
					qx = x1;
				}
				if (qy == -1 || y1 > qy) {
					qy = y1;
				}
			}//end if temp

			temp = const_cast<FloatingText*>(assocwidget->getMultiBWidget());

			if(temp && temp->isVisible()) {
				x = temp->getX();
				y = temp->getY();
				x1 = x + temp->getWidth() - 1;
				y1 = y + temp->getHeight() - 1;

				if (px == -1 || x < px) {
					px = x;
				}
				if (py == -1 || y < py) {
					py = y;
				}
				if (qx == -1 || x1 > qx) {
					qx = x1;
				}
				if (qy == -1 || y1 > qy) {
					qy = y1;
				}
			}//end if temp

			temp = const_cast<FloatingText*>(assocwidget->getRoleAWidget());
			if(temp && temp -> isVisible()) {
				x = temp->getX();
				y = temp->getY();
				x1 = x + temp->getWidth() - 1;
				y1 = y + temp->getHeight() - 1;

				if (px == -1 || x < px) {
					px = x;
				}
				if (py == -1 || y < py) {
					py = y;
				}
				if (qx == -1 || x1 > qx) {
					qx = x1;
				}
				if (qy == -1 || y1 > qy) {
					qy = y1;
				}
			}//end if temp

			temp = const_cast<FloatingText*>(assocwidget->getRoleBWidget());
			if(temp && temp -> isVisible()) {
				x = temp->getX();
				y = temp->getY();
				x1 = x + temp->getWidth() - 1;
				y1 = y + temp->getHeight() - 1;
				if (px == -1 || x < px) {
					px = x;
				}
				if (py == -1 || y < py) {
					py = y;
				}
				if (qx == -1 || x1 > qx) {
					qx = x1;
				}
				if (qy == -1 || y1 > qy) {
					qy = y1;
				}

			}//end if temp

                        temp = const_cast<FloatingText*>(assocwidget->getChangeWidgetA());
                        if(temp && temp->isVisible()) {
                                x = temp->getX();
                                y = temp->getY();
                                x1 = x + temp->getWidth() - 1;
                                y1 = y + temp->getHeight() - 1;

                                if (px == -1 || x < px) {
                                        px = x;
                                }
                                if (py == -1 || y < py) {
                                        py = y;
                                }
                                if (qx == -1 || x1 > qx) {
                                        qx = x1;
                                }
                                if (qy == -1 || y1 > qy) {
                                        qy = y1;
                                }
                        }//end if temp

                        temp = const_cast<FloatingText*>(assocwidget->getChangeWidgetB());
                        if(temp && temp->isVisible()) {
                                x = temp->getX();
                                y = temp->getY();
                                x1 = x + temp->getWidth() - 1;
                                y1 = y + temp->getHeight() - 1;

                                if (px == -1 || x < px) {
                                        px = x;
                                }
                                if (py == -1 || y < py) {
                                        py = y;
                                }
                                if (qx == -1 || x1 > qx) {
                                        qx = x1;
                                }
                                if (qy == -1 || y1 > qy) {
                                        qy = y1;
                                }
                        }//end if temp

		}//end if selected
	}//end while

	QRect imageRect;  //area with respect to getDiagramRect()
	                  //i.e. all widgets on the canvas.  Was previously with
	                  //repect to whole canvas

	imageRect.setLeft( px - rect.left() );
	imageRect.setTop( py - rect.top() );
	imageRect.setRight( qx - rect.left() );
	imageRect.setBottom( qy - rect.top() );

	pix = new QPixmap(imageRect.width(), imageRect.height());
	bitBlt(pix, QPoint(0, 0), &diagram, imageRect);
	m_bDrawSelectedOnly = false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void UMLView::setMenu() {
	slotRemovePopupMenu();
	ListPopupMenu::Menu_Type menu = ListPopupMenu::mt_Undefined;
	switch( getType() ) {
		case dt_Class:
			menu = ListPopupMenu::mt_On_Class_Diagram;
			break;

		case dt_UseCase:
			menu = ListPopupMenu::mt_On_UseCase_Diagram;
			break;

		case dt_Sequence:
			menu = ListPopupMenu::mt_On_Sequence_Diagram;
			break;

		case dt_Collaboration:
			menu = ListPopupMenu::mt_On_Collaboration_Diagram;
			break;

		case dt_State:
			menu = ListPopupMenu::mt_On_State_Diagram;
			break;

		case dt_Activity:
			menu = ListPopupMenu::mt_On_Activity_Diagram;
			break;

		case dt_Component:
			menu = ListPopupMenu::mt_On_Component_Diagram;
			break;

		case dt_Deployment:
			menu = ListPopupMenu::mt_On_Deployment_Diagram;
			break;

		default:
			kdWarning() << "setMenu() called on unknown diagram type" << endl;
			menu = ListPopupMenu::mt_Undefined;
			break;
	}//end switch
	if( menu != ListPopupMenu::mt_Undefined ) {
		m_pMenu = new ListPopupMenu(this, menu, this);
		connect(m_pMenu, SIGNAL(activated(int)), this, SLOT(slotMenuSelection(int)));
		m_pMenu->popup( mapToGlobal( contentsToViewport(worldMatrix().map(m_Pos)) ) );
	}
}

void UMLView::slotRemovePopupMenu() {
	if(m_pMenu) {
		disconnect(m_pMenu, SIGNAL(activated(int)), this, SLOT(slotMenuSelection(int)));
		delete m_pMenu;
		m_pMenu = 0;
	}
}

void UMLView::slotMenuSelection(int sel) {
	FloatingText * ft = 0;
	StateWidget * state = 0;
	ActivityWidget * activity = 0;
	bool ok = false;
	QString name = "";

	switch( (ListPopupMenu::Menu_Type)sel ) {
		case ListPopupMenu::mt_Undo:
			m_pDoc->loadUndoData();
			break;

		case ListPopupMenu::mt_Redo:
			m_pDoc->loadRedoData();
			break;

		case ListPopupMenu::mt_Clear:
			clearDiagram();
			break;

		case ListPopupMenu::mt_Export_Image:
			exportImage();
			break;

		case ListPopupMenu::mt_FloatText:
			ft = new FloatingText(this, tr_Floating, "");
			ft -> changeTextDlg();
			//if no text entered delete
			if(!FloatingText::isTextValid(ft -> getText()))

				delete ft;
			else {
				ft->setX( m_Pos.x() );
				ft->setY( m_Pos.y() );
				ft->setVisible( true );
				ft->setID(m_pDoc -> getUniqueID());
				ft->setActivated();
			}
			break;

		case ListPopupMenu::mt_UseCase:
			m_bCreateObject = true;
			m_pDoc->createUMLObject( ot_UseCase );
			break;

		case ListPopupMenu::mt_Actor:
			m_bCreateObject = true;
			m_pDoc->createUMLObject( ot_Actor );
			break;

		case ListPopupMenu::mt_Class:
		case ListPopupMenu::mt_Object:
			m_bCreateObject = true;
			m_pDoc->createUMLObject( ot_Class);
			break;

		case ListPopupMenu::mt_Package:
			m_bCreateObject = true;
			m_pDoc->createUMLObject(ot_Package);
			break;

		case ListPopupMenu::mt_Component:
			m_bCreateObject = true;
			m_pDoc->createUMLObject(ot_Component);
			break;

		case ListPopupMenu::mt_Node:
			m_bCreateObject = true;
			m_pDoc->createUMLObject(ot_Node);
			break;

		case ListPopupMenu::mt_Artifact:
			m_bCreateObject = true;
			m_pDoc->createUMLObject(ot_Artifact);
			break;

		case ListPopupMenu::mt_Interface:
			m_bCreateObject = true;
			m_pDoc->createUMLObject(ot_Interface);
			break;

		case ListPopupMenu::mt_Paste:
			m_PastePoint = m_Pos;
			m_Pos.setX( 2000 );
			m_Pos.setY( 2000 );
			m_pDoc -> editPaste();

			m_PastePoint.setX( 0 );
			m_PastePoint.setY( 0 );
			break;

		case ListPopupMenu::mt_Initial_State:
			state = new StateWidget( this , StateWidget::Initial );
			state -> setID( m_pDoc -> getUniqueID() );//needed for associations
			state -> setX( m_Pos.x() );
			state -> setY ( m_Pos.y() );
			state -> setVisible( true );
			state -> setActivated();
			break;

		case ListPopupMenu::mt_End_State:
			state = new StateWidget( this , StateWidget::End );
			state -> setID( m_pDoc -> getUniqueID() );//needed for associations
			state -> setX( m_Pos.x() );
			state -> setY ( m_Pos.y() );
			state -> setVisible( true );
			state -> setActivated();
			break;

		case ListPopupMenu::mt_State:
			name = KLineEditDlg::getText( i18n("Enter State Name"), i18n("Enter the name of the new state:"), i18n("new state"), &ok );
			if( ok ) {
				state = new StateWidget( this , StateWidget::Normal );
				state -> setName( name );
				state -> setID( m_pDoc -> getUniqueID() );//needed for associations
				state -> setX( m_Pos.x() );
				state -> setY ( m_Pos.y() );
				state -> setVisible( true );
				state -> setActivated();
			}
			break;

		case ListPopupMenu::mt_Initial_Activity:
			activity = new ActivityWidget( this , ActivityWidget::Initial );
			activity -> setID( m_pDoc -> getUniqueID() );//needed for associations
			activity -> setX( m_Pos.x() );
			activity -> setY ( m_Pos.y() );
			activity -> setVisible( true );
			activity -> setActivated();
			break;


		case ListPopupMenu::mt_End_Activity:
			activity = new ActivityWidget( this , ActivityWidget::End );
			activity -> setID( m_pDoc -> getUniqueID() );//needed for associations
			activity -> setX( m_Pos.x() );
			activity -> setY ( m_Pos.y() );
			activity -> setVisible( true );
			activity -> setActivated();
			break;

		case ListPopupMenu::mt_Branch:
			activity = new ActivityWidget( this , ActivityWidget::Branch );
			activity -> setID( m_pDoc -> getUniqueID() );//needed for associations
			activity -> setX( m_Pos.x() );
			activity -> setY ( m_Pos.y() );
			activity -> setVisible( true );
			activity -> setActivated();
			break;

		case ListPopupMenu::mt_Activity:
			name = KLineEditDlg::getText( i18n("Enter Activity Name"), i18n("Enter the name of the new activity:"), i18n("new activity"), &ok );
			if( ok ) {
				activity = new ActivityWidget( this , ActivityWidget::Normal );
				activity -> setName( name );
				activity -> setID( m_pDoc -> getUniqueID() );//needed for associations
				activity -> setX( m_Pos.x() );
				activity -> setY ( m_Pos.y() );
				activity -> setVisible( true );
				activity -> setActivated();
			}
			break;

		case ListPopupMenu::mt_SnapToGrid:
			toggleSnapToGrid();
			break;

		case ListPopupMenu::mt_ShowSnapGrid:
			toggleShowGrid();
			break;

		case ListPopupMenu::mt_Properties:
			showPropDialog();
			break;

		default:
			break;
	}
}

void UMLView::slotCutSuccessful() {
	if( m_bStartedCut ) {
		deleteSelection();
		m_bStartedCut = false;
	}
}

void UMLView::slotShowView() {
	m_pDoc -> changeCurrentView( getID() );
}

QPoint UMLView::getPastePoint() {
	QPoint point = m_PastePoint;
	point.setX( point.x() - m_Pos.x() );
	point.setY( point.y() - m_Pos.y() );
	return point;
}

bool UMLView::showPropDialog() {
	UMLViewDialog dlg( this, this );
	if( dlg.exec() ) {
		return true;
	}
	return false;
}


void UMLView::setFont( QFont font ) {
	m_Options.uiState.font = font;
}

void UMLView::setClassWidgetOptions( ClassOptionsPage * page ) {
	UMLWidget * pWidget = 0;
	UMLWidgetListIt wit( m_WidgetList );
	while ( (pWidget = wit.current()) != 0 ) {
		++wit;
		if( pWidget -> getBaseType() == Uml::wt_Class ) {
			page -> setWidget( static_cast<ClassWidget *>( pWidget ) );
			page -> updateUMLWidget();
		}
	}
}


void UMLView::checkSelections() {
	UMLWidget * pWA = 0, * pWB = 0, * pTemp = 0;
	//check messages
	for(pTemp=(UMLWidget *)m_SelectedList.first();pTemp;pTemp=(UMLWidget *)m_SelectedList.next()) {
		if( pTemp->getBaseType() == wt_Message && pTemp -> getSelected() ) {
			MessageWidget * pMessage = static_cast<MessageWidget *>( pTemp );
			pWA = pMessage -> getWidgetA();
			pWB = pMessage -> getWidgetB();
			if( !pWA -> getSelected() ) {
				pWA -> setSelectedFlag( true );
				m_SelectedList.append( pWA );
			}
			if( !pWB -> getSelected() ) {
				pWB -> setSelectedFlag( true );
				m_SelectedList.append( pWB );
			}
		}//end if
	}//end for
	//check Associations
	AssociationWidgetListIt it(m_AssociationList);
	AssociationWidget * pAssoc = 0;
	while((pAssoc = it.current())) {
		++it;
		if( pAssoc -> getSelected() ) {
			pWA = pAssoc -> getWidgetA();
			pWB = pAssoc -> getWidgetB();
			if( !pWA -> getSelected() ) {
				pWA -> setSelectedFlag( true );
				m_SelectedList.append( pWA );
			}
			if( !pWB -> getSelected() ) {
				pWB -> setSelectedFlag( true );
				m_SelectedList.append( pWB );
			}
		}//end if
	}//end while
}

bool UMLView::checkUniqueSelection()
{
	// if there are no selected items, we return true
	if (m_SelectedList.count() <= 0)
		return true;

	// get the first item and its base type
	UMLWidget * pTemp = (UMLWidget *) m_SelectedList.first();
	UMLWidget_Type tmpType = pTemp -> getBaseType();

	// check all selected items, if they have the same BaseType
	for ( pTemp = (UMLWidget *) m_SelectedList.first();
				pTemp;
					pTemp = (UMLWidget *) m_SelectedList.next() ) {
		if( pTemp->getBaseType() != tmpType)
		{
			return false; // the base types are different, the list is not unique
		}
	} // for ( through all selected items )

	return true; // selected items are unique
}

void UMLView::clearDiagram() {
	if( KMessageBox::Yes == KMessageBox::warningYesNo( this, i18n("You are about to delete "
								       "the entire diagram.\nAre you sure?"),
							    i18n("Delete Diagram?") ) ) {
		removeAllWidgets();
	}
}

void UMLView::toggleSnapToGrid() {
	setSnapToGrid( !getSnapToGrid() );
}

void UMLView::toggleSnapComponentSizeToGrid() {
	setSnapComponentSizeToGrid( !getSnapComponentSizeToGrid() );
}

void UMLView::toggleShowGrid() {
	setShowSnapGrid( !getShowSnapGrid() );
}

void UMLView::setSnapToGrid(bool bSnap) {
	m_bUseSnapToGrid = bSnap;
	emit sigSnapToGridToggled( getSnapToGrid() );
}

void UMLView::setSnapComponentSizeToGrid(bool bSnap) {
	m_bUseSnapComponentSizeToGrid = bSnap;
	updateComponentSizes();
	emit sigSnapComponentSizeToGridToggled( getSnapComponentSizeToGrid() );
}

void UMLView::setShowSnapGrid(bool bShow) {
	m_bShowSnapGrid = bShow;
	canvas()->setAllChanged();
	emit sigShowGridToggled( getShowSnapGrid() );
}

void UMLView::setZoom(int zoom) {
	if (zoom < 10) {
		zoom = 10;
	} else if (zoom > 500) {
		zoom = 500;
	}

	QWMatrix wm;
	wm.scale(zoom/100.0,zoom/100.0);
	setWorldMatrix(wm);

	m_nZoom = currentZoom();
	resizeCanvasToItems();
}

int UMLView::currentZoom() {
	return (int)(worldMatrix().m11()*100.0);
}

void UMLView::zoomIn() {
	QWMatrix wm = worldMatrix();
	wm.scale(1.5,1.5); // adjust zooming step here
	setZoom( (int)(wm.m11()*100.0) );
}

void UMLView::zoomOut() {
	QWMatrix wm = worldMatrix();
	wm.scale(2.0/3.0, 2.0/3.0); //adjust zooming step here
	setZoom( (int)(wm.m11()*100.0) );
}

void UMLView::fileLoaded() {
	setZoom( getZoom() );
	resizeCanvasToItems();
}

void UMLView::setCanvasSize(int width, int height) {
	setCanvasWidth(width);
	setCanvasHeight(height);
	canvas()->resize(width, height);
}

void UMLView::resizeCanvasToItems() {
	QRect canvasSize = getDiagramRect();
	int canvasWidth = canvasSize.right() + 5;
	int canvasHeight = canvasSize.bottom() + 5;

	//Find out the bottom right visible pixel and size to at least that
	int contentsX, contentsY;
	int contentsWMX, contentsWMY;
	viewportToContents(viewport()->width(), viewport()->height(), contentsX, contentsY);
	inverseWorldMatrix().map(contentsX, contentsY, &contentsWMX, &contentsWMY);

	if (canvasWidth < contentsWMX) {
		canvasWidth = contentsWMX;
	}

	if (canvasHeight < contentsWMY) {
		canvasHeight = contentsWMY;
	}

	setCanvasSize(canvasWidth, canvasHeight);
}

void UMLView::show() {
	QWidget::show();
	resizeCanvasToItems();
}

void UMLView::updateComponentSizes() {
	// update sizes of all components
	UMLWidgetListIt it( m_WidgetList );
	UMLWidget *obj;
	while ( (obj=(UMLWidget*)it.current()) != 0 ) {
		++it;
		obj->updateComponentSize();
	}
}

/**
 * Force the widget font metrics to be updated next time
 * the widgets are drawn.
 * This is necessary because the widget size might depend on the
 * font metrics and the font metrics might change for different
 * QPainter, i.e. font metrics for Display font and Printer font are
 * usually different.
 * Call this when you change the QPainter.
 */
void UMLView::forceUpdateWidgetFontMetrics(QPainter * painter) {
	UMLWidgetListIt it( m_WidgetList );
	UMLWidget *obj;

	while ((obj = it.current()) != 0 ) {
		++it;
		obj->forceUpdateFontMetrics(painter);
	}
}

bool UMLView::saveToXMI( QDomDocument & qDoc, QDomElement & qElement ) {
	QDomElement viewElement = qDoc.createElement( "diagram" );
	viewElement.setAttribute( "xmi.id", m_nID );
	viewElement.setAttribute( "name", m_Name );
	viewElement.setAttribute( "type", m_Type );
	viewElement.setAttribute( "documentation", m_Documentation );
	//optionstate uistate
	viewElement.setAttribute( "fillcolor", m_Options.uiState.fillColor.name() );
	viewElement.setAttribute( "linecolor", m_Options.uiState.lineColor.name() );
	viewElement.setAttribute( "usefillcolor", m_Options.uiState.useFillColor );
	viewElement.setAttribute( "font", m_Options.uiState.font.toString() );
	//optionstate classstate
	viewElement.setAttribute( "showattsig", m_Options.classState.showAttSig );
	viewElement.setAttribute( "showatts", m_Options.classState.showAtts);
	viewElement.setAttribute( "showopsig", m_Options.classState.showOpSig );
	viewElement.setAttribute( "showops", m_Options.classState.showOps );
	viewElement.setAttribute( "showpackage", m_Options.classState.showPackage );
	viewElement.setAttribute( "showscope", m_Options.classState.showScope );
	viewElement.setAttribute( "showstereotype", m_Options.classState.showStereoType );
	//misc
	viewElement.setAttribute( "localid", m_nLocalID );
	viewElement.setAttribute( "showgrid", m_bShowSnapGrid );
	viewElement.setAttribute( "snapgrid", m_bUseSnapToGrid );
	viewElement.setAttribute( "snapcsgrid", m_bUseSnapComponentSizeToGrid );
	viewElement.setAttribute( "snapx", m_nSnapX );
	viewElement.setAttribute( "snapy", m_nSnapY );
	viewElement.setAttribute( "zoom", m_nZoom );
	viewElement.setAttribute( "canvasheight", m_nCanvasHeight );
	viewElement.setAttribute( "canvaswidth", m_nCanvasWidth );
	//now save all the widgets
	UMLWidget * widget = 0;
	UMLWidgetListIt w_it( m_WidgetList );
	QDomElement widgetElement = qDoc.createElement( "widgets" );
	while( ( widget = w_it.current() ) ) {
		++w_it;
		widget -> saveToXMI( qDoc, widgetElement );
	}
	viewElement.appendChild( widgetElement );
	//now save the message widgets
	MessageWidgetListIt m_it( m_MessageList );
	QDomElement messageElement = qDoc.createElement( "messages" );
	while( ( widget = m_it.current() ) ) {
		++m_it;
		widget -> saveToXMI( qDoc, messageElement );
	}
	viewElement.appendChild( messageElement );
	//now save the associations
	QDomElement assocElement = qDoc.createElement( "associations" );
	if ( m_AssociationList.count() ) {
		// We guard against ( m_AssociationList.count() == 0 ) because
		// this code could be reached as follows:
		//  ^  UMLView::saveToXMI()
		//  ^  UMLDoc::saveToXMI()
		//  ^  UMLDoc::addToUndoStack()
		//  ^  UMLDoc::setModified()
		//  ^  UMLDoc::createDiagram()
		//  ^  UMLDoc::newDocument()
		//  ^  UMLApp::newDocument()
		//  ^  main()
		//
		AssociationWidgetListIt a_it( m_AssociationList );
		AssociationWidget * assoc = 0;
		while( ( assoc = a_it.current() ) ) {
			++a_it;
			assoc -> saveToXMI( qDoc, assocElement );
		}
		// kdDebug() << "UMLView::saveToXMI() saved "
		//	<< m_AssociationList.count() << " assocData." << endl;
	}
	viewElement.appendChild( assocElement );
	qElement.appendChild( viewElement );
	return true;
}

bool UMLView::loadFromXMI( QDomElement & qElement ) {
	QString id = qElement.attribute( "xmi.id", "-1" );
	m_nID = id.toInt();
	if( m_nID == -1 )
		return false;
	m_Name = qElement.attribute( "name", "" );
	QString type = qElement.attribute( "type", "-1" );
	m_Documentation = qElement.attribute( "documentation", "" );
	QString localid = qElement.attribute( "localid", "0" );
	//optionstate uistate
	QString font = qElement.attribute( "font", "" );
	if( !font.isEmpty() )
		m_Options.uiState.font.fromString( font );
	QString fillcolor = qElement.attribute( "fillcolor", "" );
	QString linecolor = qElement.attribute( "linecolor", "" );
	QString usefillcolor = qElement.attribute( "usefillcolor", "0" );
	m_Options.uiState.useFillColor = (bool)usefillcolor.toInt();
	//optionstate classstate
	QString temp = qElement.attribute( "showattsig", "0" );
	m_Options.classState.showAttSig = (bool)temp.toInt();
	temp = qElement.attribute( "showatts", "0" );
	m_Options.classState.showAtts = (bool)temp.toInt();
	temp = qElement.attribute( "showopsig", "0" );
	m_Options.classState.showOpSig = (bool)temp.toInt();
	temp = qElement.attribute( "showops", "0" );
	m_Options.classState.showOps = (bool)temp.toInt();
	temp = qElement.attribute( "showpackage", "0" );
	m_Options.classState.showPackage = (bool)temp.toInt();
	temp = qElement.attribute( "showscope", "0" );
	m_Options.classState.showScope = (bool)temp.toInt();
	temp = qElement.attribute( "showstereotype", "0" );
	m_Options.classState.showStereoType = (bool)temp.toInt();
	//misc
	QString showgrid = qElement.attribute( "showgrid", "0" );
	m_bShowSnapGrid = (bool)showgrid.toInt();

	QString snapgrid = qElement.attribute( "snapgrid", "0" );
	m_bUseSnapToGrid = (bool)snapgrid.toInt();

	QString snapcsgrid = qElement.attribute( "snapcsgrid", "0" );
	m_bUseSnapComponentSizeToGrid = (bool)snapcsgrid.toInt();

	QString snapx = qElement.attribute( "snapx", "10" );
	m_nSnapX = snapx.toInt();

	QString snapy = qElement.attribute( "snapy", "10" );
	m_nSnapY = snapy.toInt();

	QString zoom = qElement.attribute( "zoom", "100" );
	m_nZoom = zoom.toInt();

	QString height = qElement.attribute( "canvasheight", QString("%1").arg(UMLView::defaultCanvasSize) );
	m_nCanvasHeight = height.toInt();

	QString width = qElement.attribute( "canvaswidth", QString("%1").arg(UMLView::defaultCanvasSize) );
	m_nCanvasWidth = width.toInt();

	m_Type = (Uml::Diagram_Type)type.toInt();
	if( !fillcolor.isEmpty() )
		m_Options.uiState.fillColor = QColor( fillcolor );
	if( !linecolor.isEmpty() )
		m_Options.uiState.lineColor = QColor( linecolor );
	m_nLocalID = localid.toInt();
	//load the widgets
	QDomNode node = qElement.firstChild();
	QDomElement element = node.toElement();
	if( !element.isNull() && element.tagName() != "widgets" )
		return false;
	if( !loadWidgetsFromXMI( element ) ) {
		kdWarning() << "failed umlview load on widgets" << endl;
		return false;
	}

	//load the message widgets
	node = element.nextSibling();
	element = node.toElement();
	if( !element.isNull() && element.tagName() != "messages" )
		return false;
	if( !loadMessagesFromXMI( element ) ) {
		kdWarning() << "failed umlview load on messages" << endl;
		return false;
	}

	//load the associations
	node = element.nextSibling();
	element = node.toElement();
	if( !element.isNull() && element.tagName() != "associations" )
		return false;
	if( !loadAssociationsFromXMI( element ) ) {
		kdWarning() << "failed umlview load on associations" << endl;
		return false;
	}
	return true;
}

bool UMLView::loadWidgetsFromXMI( QDomElement & qElement ) {
	UMLWidget* widget = 0;
	QDomNode node = qElement.firstChild();
	QDomElement widgetElement = node.toElement();
	while( !widgetElement.isNull() ) {
		widget = loadWidgetFromXMI(widgetElement);
		if (!widget) {
			return false;
		}
		m_WidgetList.append( widget );
		node = widgetElement.nextSibling();
		widgetElement = node.toElement();
	}

	return true;
}

UMLWidget* UMLView::loadWidgetFromXMI(QDomElement& widgetElement) {
	UMLWidget* widget = 0;
	QString tag = widgetElement.tagName();
	if (tag == "UML:ActorWidget") {
		widget = new ActorWidget(this);
	} else if (tag == "UML:UseCaseWidget") {
		widget = new UseCaseWidget(this);
// Have ConceptWidget for backwards compatability
	} else if (tag == "UML:ClassWidget" || tag == "UML:ConceptWidget") {
		widget = new ClassWidget(this);
	} else if (tag == "packagewidget") {
		widget = new PackageWidget(this);
	} else if (tag == "componentwidget") {
		widget = new ComponentWidget(this);
	} else if (tag == "nodewidget") {
		widget = new NodeWidget(this);
	} else if (tag == "artifactwidget") {
		widget = new ArtifactWidget(this);
	} else if (tag == "interfacewidget") {
		widget = new InterfaceWidget(this);
	} else if (tag == "UML:StateWidget") {
		widget = new StateWidget(this);
	} else if (tag == "UML:NoteWidget") {
		widget = new NoteWidget(this);
	} else if (tag == "boxwidget") {
		widget = new BoxWidget(this);
	} else if (tag == "UML:FloatingTextWidget") {
		widget = new FloatingText(this);
	} else if (tag == "UML:ObjectWidget") {
		widget = new ObjectWidget(this);
	} else if (tag == "UML:ActivityWidget") {
		widget = new ActivityWidget(this);
	} else {
		kdWarning() << "Trying to create an unknown widget:" << tag << endl;
		return 0;
	}
	if (!widget->loadFromXMI(widgetElement)) {
		widget->cleanup();
		delete widget;
		return 0;
	}
	if (m_pDoc == NULL) {
		kdDebug() << "UMLView::loadWidgetFromXMI(): m_pDoc is NULL" << endl;
		return 0;
	}
	UMLObject *o = m_pDoc->findUMLObject(widget -> getID());
	if (o)
		widget->setUMLObject( o );
	return widget;
}

bool UMLView::loadMessagesFromXMI( QDomElement & qElement ) {
	MessageWidget * message = 0;
	QDomNode node = qElement.firstChild();
	QDomElement messageElement = node.toElement();
	while( !messageElement.isNull() ) {
		if( messageElement.tagName() == "UML:MessageWidget" ) {
			message = new MessageWidget(this, sequence_message_asynchronous);
			if( !message -> loadFromXMI( messageElement ) ) {
				delete message;
				return false;
			}
			m_MessageList.append( message );
		}
		node = messageElement.nextSibling();
		messageElement = node.toElement();
	}
	return true;
}

bool UMLView::loadAssociationsFromXMI( QDomElement & qElement ) {
	QDomNode node = qElement.firstChild();
	QDomElement assocElement = node.toElement();
	while( !assocElement.isNull() ) {
		if( assocElement.tagName() == "UML:AssocWidget" ) {
			AssociationWidget *assoc = new AssociationWidget(this);
			if( !assoc->loadFromXMI( assocElement ) ) {
				assoc->cleanup();
				delete assoc;
				/* return false;
				   Returning false here is a little harsh when the
				   rest of the diagram might load okay.
				 */
			} else {
				m_AssociationList.append( assoc );
			}
		}
		node = assocElement.nextSibling();
		assocElement = node.toElement();
	}
	return true;
}


#include "umlview.moc"
