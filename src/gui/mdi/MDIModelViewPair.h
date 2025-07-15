/*
 * Copyright 2017, 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
/**
 * @file
 */

// Std C++.
#include <vector>
#include <concepts>

// Qt
#include <QAbstractProxyModel>
#include <QPointer>
class QAbstractItemModel;
class MDITreeViewBase;

// Ours
#include <utils/DebugHelpers.h>


/**
 * Object which holds:
 * - A model
 * - A stack of proxy models on top of the model
 * - A view on top of the proxy models
 */
class MDIModelViewPair
{

public:

	template <typename T>
        requires (std::convertible_to<T, QAbstractItemModel*> && !std::convertible_to<T, QAbstractProxyModel*>)
    void appendModel(T model)
	{
		// For now anyway, this should only push a model to the bottom of the {proxy}model stack.
		if (!m_model_stack.empty())
		{
            qCr() << "Model wasn't the first item pushed onto the modelstack";
		}

		m_model_stack.emplace_back(model);
	}

	/**
	 * Append a proxy model to the top of the model stack.
     * Note that @a proxymodel is setSourceModel()'d to the current top {proxy}model in the stack, but no other
	 * connections are made.
	 * @tparam T
	 * @param proxymodel
	 */
	template <typename T>
        requires (std::convertible_to<T, QAbstractProxyModel*>)
    void appendProxyModel(T proxymodel)
	{
		if (m_model_stack.empty())
		{
            qCr() << "No model to connect ProxyModel to";
		}

		m_model_stack.emplace_back(proxymodel);

		proxymodel->setSourceModel(m_model_stack.begin()->data());
	}

	void appendView(QPointer<MDITreeViewBase> view);

	bool hasModel() const;
	bool hasView() const;
	bool hasModelAndView() const;

	QPointer<MDITreeViewBase> getView() const { return m_view; }
    QPointer<QAbstractItemModel> getTopModel() const { return m_model_stack.back(); }

	/**
	 * @todo Seems like this should go away eventually.
	 * @return
	 */
    QPointer<QAbstractItemModel> getRootModel() const { return m_model_stack.front(); }

	QPointer<QAbstractItemModel> getProxyAt(int proxymodel) const;

private:
	std::vector<QPointer<QAbstractItemModel>> m_model_stack;
	QPointer<MDITreeViewBase> m_view {nullptr};

public: /// @todo TEMP, MAKE PRIVATE
	bool m_model_was_existing {false};
	bool m_view_was_existing {false};
};

#endif //GUI_MDI_MDIVIEWPAIRMODEL_H
