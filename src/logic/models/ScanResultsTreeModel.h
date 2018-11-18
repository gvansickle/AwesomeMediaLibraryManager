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

#ifndef SCANRESULTSTREEMODEL_H
#define SCANRESULTSTREEMODEL_H

#include "AbstractTreeModel.h"

// Qt5
#include <QUrl>

// Ours
#include "ScanResultsTreeModelItem.h"
class AbstractTreeModelHeaderItem;

class ScanResultsTreeModel : public AbstractTreeModel
{
    using BASE_CLASS = AbstractTreeModel;

	Q_OBJECT

public:
    ScanResultsTreeModel(const QStringList &headers, const QString &data,
                         QObject *parent = nullptr);
    ~ScanResultsTreeModel() override = default;

	/**
	 * Append a vector of AbstractTreeModelItem's as children of @p parent.
	 */
	bool appendItems(QVector<AbstractTreeModelItem*> new_items, const QModelIndex &parent = QModelIndex()) override;

	/// @name Serialization
	/// @{

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	/// @}

protected:
	QString getXmlStreamName() const override { return "AMLMScanResults"; };
	QString getXmlStreamVersion() const override { return "0.1"; };

	/// Create a new root node.
	AbstractTreeModelHeaderItem* make_root_node(QVector<QVariant> rootData) override;

    QUrl m_base_directory;

};

#endif // SCANRESULTSTREEMODEL_H
