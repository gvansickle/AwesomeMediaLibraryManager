/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef GUI_MDI_MDIVIEWPAIRMODEL_H
#define GUI_MDI_MDIVIEWPAIRMODEL_H

#include <QPointer>

class MDITreeViewBase;
class LibraryModel;
class QAbstractItemModel;

class MDIModelViewPair
{
public:
	QPointer<QAbstractItemModel> m_model { nullptr };
	QPointer<MDITreeViewBase> m_view { nullptr };

    bool m_model_was_existing { false };
    bool m_view_was_existing { false };

	template <typename T>
	void setModel(T* derived_model_ptr)
	{
		m_model = derived_model_ptr;
	}

	template <typename T>
	void setModel(QPointer<T> derived_model_ptr)
	{
		m_model = derived_model_ptr;
	}

	bool hasModel() const { return m_model; }
	bool hasView() const { return m_view; } ///< This really shouldn't ever be the case if there's no model.
	bool hasModelAndView() const { return m_model && m_view; }
};

#endif //GUI_MDI_MDIVIEWPAIRMODEL_H
