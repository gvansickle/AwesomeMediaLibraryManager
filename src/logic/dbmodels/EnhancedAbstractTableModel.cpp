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

#include "EnhancedAbstractTableModel.h"

EnhancedAbstractTableModel::EnhancedAbstractTableModel(QObject *parent)
	: QAbstractTableModel(parent)
{
}

QVariant EnhancedAbstractTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	/// @note Implemented. // FIXME: Implement me!
	QPair<int, Qt::ItemDataRole> section_role {section, static_cast<Qt::ItemDataRole>(role)};
	auto retval = m_horizontal_header_data.value(section_role, QVariant());
	return retval;
}

bool EnhancedAbstractTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	if (value != headerData(section, orientation, role))
	{
		/// @note Implemented // FIXME: Implement me!
		QPair<int, Qt::ItemDataRole> section_role {section, static_cast<Qt::ItemDataRole>(role)};
		if(m_horizontal_header_data.contains(section_role, value))
		{
			// multimap already has this same key/key/value, nothing to do.
			return false;
		}
		m_horizontal_header_data.replace(section_role, value);
		Q_EMIT headerDataChanged(orientation, section, section);
		return true;
	}
	return false;
}


int EnhancedAbstractTableModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	// FIXME: Implement me!
    return 0;
}

int EnhancedAbstractTableModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	// FIXME: Implement me!
    return 0;
}

QVariant EnhancedAbstractTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	/// @note Implemented. // FIXME: Implement me!

	return getData(index.row(), index.column(), role);

//	return QVariant();
}

bool EnhancedAbstractTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (data(index, role) != value)
	{
		// FIXME: Implement me!
		Q_EMIT dataChanged(index, index, QVector<int>() << role);
		return true;
	}
	return false;
}

Qt::ItemFlags EnhancedAbstractTableModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
	{
		return Qt::NoItemFlags;
	}

	return Qt::ItemIsEditable; // FIXME: Implement me!
}

bool EnhancedAbstractTableModel::insertRows(int row, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endInsertRows();
    return false;
}

bool EnhancedAbstractTableModel::insertColumns(int column, int count, const QModelIndex &parent)
{
	beginInsertColumns(parent, column, column + count - 1);
	// FIXME: Implement me!
	endInsertColumns();
    return false;
}

bool EnhancedAbstractTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
	beginRemoveRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endRemoveRows();

    return false;
}

bool EnhancedAbstractTableModel::removeColumns(int column, int count, const QModelIndex &parent)
{
	beginRemoveColumns(parent, column, column + count - 1);
	// FIXME: Implement me!
	endRemoveColumns();
    return false;
}

//void EnhancedAbstractTableModel::appendRows(int row, int count, const QModelIndex& parent,
//											const std::vector<std::vector<const QVariant&>>& itemData)
//{
//	beginInsertRows(parent, row, row + count - 1);

//	// DERIVED CLASS: Allocate empty rows.
//	this->insertEmptyRows(row, count, parent);

//	// setItemData() for each row.
//	int rindex = 0;
//	for(auto& this_row : itemData)
//	{
//		int colindex = 0;
//		for(const QVariant& this_col : this_row)
//		{
//			// DERIVED CLASS: Set Qt::EditRole ItemData.
//			this->setUnderlyingItemData(row+rindex, colindex, this_col);
//			colindex++;
//		}
//		rindex++;
//	}

//	endInsertRows();
//}
