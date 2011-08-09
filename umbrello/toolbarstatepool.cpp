/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   copyright (C) 2004-2009                                               *
 *   Umbrello UML Modeller Authors <uml-devel@uml.sf.net>                  *
 ***************************************************************************/

// own header
#include "toolbarstatepool.h"

//new canvas
#include "soc-umbrello-2011/umlview.h"
#include "soc-umbrello-2011/umlscene.h"

/**
 * Destroys this ToolBarStatePool.
 */
ToolBarStatePool::~ToolBarStatePool()
{
}

/**
 * Sets the current button and inits the tool.
 * If the current button is the same to the button to set, the tool isn't
 * initialized.
 *
 * @param button The button to set.
 */
void ToolBarStatePool::setButton(const WorkToolBar::ToolBar_Buttons &button)
{
    if (button != m_ToolBarButton) {
        m_ToolBarButton = button;

        init(); // Go back to the initial state.
    }
}

/**
 * Returns the current button.
 *
 * @return The current button.
 */
WorkToolBar::ToolBar_Buttons ToolBarStatePool::getButton() const
{
    return m_ToolBarButton;
}

/**
 * Creates a new ToolBarStatePool.
 * Protected to avoid classes other than derived to create objects of this
 * class.
 *
 * @param umlView The UMLView to use.
 */
ToolBarStatePool::ToolBarStatePool(UMLView *umlView)
  : ToolBarState(umlView)
{
    m_ToolBarButton = WorkToolBar::tbb_Arrow;
}

#ifdef SOC2011
ToolBarStatePool::ToolBarStatePool(QGV::UMLView *umlView)
  : ToolBarState(umlView)
{
    m_ToolBarButton = WorkToolBar::tbb_Arrow;
}

#endif

#include "toolbarstatepool.moc"
