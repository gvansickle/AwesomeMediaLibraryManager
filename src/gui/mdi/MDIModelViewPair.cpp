/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of AwesomeMediaLibraryManager.
 *
 * AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @file
 */

#include "MDIModelViewPair.h"

#include <QAbstractItemModel>
#include "gui/MDITreeViewBase.h"

void MDIModelViewPair::appendView(QPointer<MDITreeViewBase> view)
{
	if (!m_view.isNull())
	{
		qFatal("MDIModelViewPair::appendView: Already have a view.");
	}
	if (m_model_stack.empty())
	{
		qFatal("MDIModelViewPair::appendView: No model.");
	}
	m_view = view;

	m_view->setModel(m_model_stack.back());
}

bool MDIModelViewPair::hasModel() const
{
	return !m_model_stack.empty();
}

bool MDIModelViewPair::hasView() const
{
	return m_view;
}

bool MDIModelViewPair::hasModelAndView() const
{
	return (!m_model_stack.empty()) && m_view;
}
