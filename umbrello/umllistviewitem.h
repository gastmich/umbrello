/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef UMLLISTVIEWITEM_H
#define UMLLISTVIEWITEM_H

#include "umllistviewitemdata.h"
#include <qlistview.h>

class UMLListView;

/**
 *	Items used by the class @ref UMLListView.  This is need as the type
 *	and object information is required to be stored.
 *
 *	@short	Items used by @ref UMLListView.
 *	@author Paul Hensgen	<phensgen@techie.com>
 *	@version	1.0
 *	@see	UMLListView
 */

class UMLListViewItem : public QListViewItem {
public:
	/**
	 *	Sets up an instance.
	 *
	 *	@param	parent	The parent to this instance.
	 *	@param	name	The name of this instance.
	 *	@param	t	The type of this instance.
	 *	@param	o	The object it represents.
	 */
	UMLListViewItem(UMLListView * parent, QString name, Uml::ListView_Type t, UMLObject*o=0);

	/**
	 *	Sets up an instance.
	 *
	 *	@param	parent	The parent to this instance.
	 *	@param	name	The name of this instance.
	 *	@param	t	The type of this instance.
	 *	@param	o	The object it represents.
	 */
	UMLListViewItem(UMLListViewItem * parent, QString name, Uml::ListView_Type t, UMLObject*o=0);

	/**
	 *	Sets up an instance.
	 *
	 *	@param	parent	The parent to this instance.
	 *	@param	name	The name of this instance.
	 *	@param	t	The type of this instance.
	 *	@param	id	The id of this instance.
	 */
	UMLListViewItem(UMLListViewItem * parent, QString name, Uml::ListView_Type t, int id);

	/**
	 *	Standard deconstructor.
	 */
	~UMLListViewItem();

	/**
	 *	Returns the type this instance represents.
	 *
	 *	@return	The type this instance represents.
	 */
	Uml::ListView_Type getType() const;

	/**
	 *	Returns the id this class represents.
	 *
	 *	@return	The id this class represents.
	 */
	int getID();

	/**
	 *	Return the UMLObject associated with this instance.
	 *
	 *	@return	The object this class represents.
	 */
	UMLObject * getUMLObject() {
		return m_Data.getUMLObject();
	}

	/**
	 *	Updates the representation of the object.
	 */
	void updateObject();

	/**
	 * Write property of UMLListViewItemData data.
	 */
	virtual void setdata(UMLListViewItemData& _newVal);

	/**
	 * Read property of UMLListViewItemData data.
	 */
	virtual UMLListViewItemData* getdata();

	/**
	 *   Updates the icon on a folder.
	 */
	void updateFolder();

	/**
	 *   Overrides default method.
	 *   Will call default method but also makes sure correct icon is shown.
	 */
	void setOpen( bool open );

	/**
	 *   Changes the current text and updates the tooltip.
	 */
	void setText( QString text );

	/**
	 *   Sets if the item is in the middle of being created.
	 */
	void setCreating( bool creating ) {
		m_bCreating = creating;
	}

	/**
	 *     Overrides default method to make public.
	 */
	void cancelRename( int col );

	/**
	 *	Overrides the default sorting to sort by item type.
	 */
	virtual int compare(QListViewItem *other, int col, bool ascending) const;

protected:

	/**
	 *   This function is called if the user presses Enter during in-place renaming
	 *  of the item in column col, reimplemented from QlistViewItem
	 */
	void okRename( int col );

	/**
	 * Contains the uml information for a UMLListViewItem
	 */
	UMLListViewItemData m_Data;

	/**
	 *   This list view all the instance of this class are displayed on.
	 */
	static UMLListView * s_pListView;

	/**
	 *   Flag used to set the state of creating.
	 */
	bool m_bCreating;
};

#endif
