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
 * @file AbstractTreeModelHeaderItem.h
 */
#ifndef SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_
#define SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_

// Qt5
#include <QVector>
#include <QVariant>

// Ours
#include "AbstractTreeModelItem.h"
#include "AbstractHeaderSection.h"
#include "ScanResultsTreeModelItem.h"


/**
 *
 */
class AbstractTreeModelHeaderItem: public AbstractTreeModelItem
{
public:
	explicit AbstractTreeModelHeaderItem(QVector<QVariant> x = QVector<QVariant>(),
	                                     AbstractTreeModelItem *parentItem = nullptr);
	 ~AbstractTreeModelHeaderItem() override;

	/**
	 * Write this item and any children to the given QXmlStreamWriter.
	 * Override this in derived classes to do the right thing.
	 * @returns true
	 */
	bool writeItemAndChildren(QXmlStreamWriter* writer) const override;

	/// @name Serialization
	/// @{

	QVariant toVariant() const override;

	void fromVariant(const QVariant& variant) override;

	/// @}

protected:

	ScanResultsTreeModelItem* create_default_constructed_child_item(AbstractTreeModelItem *parent = nullptr) override;
};

#endif /* SRC_LOGIC_MODELS_ABSTRACTTREEMODELHEADERITEM_H_ */
