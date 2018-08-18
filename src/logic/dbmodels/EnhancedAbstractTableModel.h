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

#ifndef ENHANCEDABSTRACTTABLEMODEL_H
#define ENHANCEDABSTRACTTABLEMODEL_H

#include <QAbstractTableModel>

class EnhancedAbstractTableModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	explicit EnhancedAbstractTableModel(QObject *parent = nullptr);

	// Header:
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

	bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;

	// Basic functionality:
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	// Editable:
	bool setData(const QModelIndex &index, const QVariant &value,
				 int role = Qt::EditRole) override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;

	// Add data:
	bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
	bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

	// Remove data:
	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
	bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

	/// @name Additional interfaces beyond the QAbstractItemModel interface.
	/// @{

	/**
	 * Append a row to the model.
	 * Equivalent to calling insertRows() and then setItemData(*, Qt::EditRole) on every column.
	 *
	 * @param row
	 * @param count
	 * @param parent
	 */
//	virtual void appendRows(int row, int count, const QModelIndex &parent, const std::vector<std::vector<const QVariant&> >& itemData);
	/// @}

protected:

//	virtual bool insertEmptyRows(int row, int count, const QModelIndex &parent = QModelIndex()) = 0;
//	virtual void setUnderlyingItemData(int row, int col, QVariant value) = 0;
	virtual QVariant getData(int row, int col, int role = Qt::DisplayRole) const = 0;

	/// Header data, indexed by section, role.
	QMultiMap<QPair<int, Qt::ItemDataRole>, QVariant> m_horizontal_header_data;

private:
};

#endif // ENHANCEDABSTRACTTABLEMODEL_H
