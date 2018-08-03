#include "EnhancedAbstractTableModel.h"

EnhancedAbstractTableModel::EnhancedAbstractTableModel(QObject *parent)
	: QAbstractTableModel(parent)
{
}

QVariant EnhancedAbstractTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	// FIXME: Implement me!
}

bool EnhancedAbstractTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	if (value != headerData(section, orientation, role)) {
		// FIXME: Implement me!
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
}

int EnhancedAbstractTableModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	// FIXME: Implement me!
}

QVariant EnhancedAbstractTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	// FIXME: Implement me!
	return QVariant();
}

bool EnhancedAbstractTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (data(index, role) != value) {
		// FIXME: Implement me!
		Q_EMIT dataChanged(index, index, QVector<int>() << role);
		return true;
	}
	return false;
}

Qt::ItemFlags EnhancedAbstractTableModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	return Qt::ItemIsEditable; // FIXME: Implement me!
}

bool EnhancedAbstractTableModel::insertRows(int row, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endInsertRows();
}

bool EnhancedAbstractTableModel::insertColumns(int column, int count, const QModelIndex &parent)
{
	beginInsertColumns(parent, column, column + count - 1);
	// FIXME: Implement me!
	endInsertColumns();
}

bool EnhancedAbstractTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
	beginRemoveRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endRemoveRows();
}

bool EnhancedAbstractTableModel::removeColumns(int column, int count, const QModelIndex &parent)
{
	beginRemoveColumns(parent, column, column + count - 1);
	// FIXME: Implement me!
	endRemoveColumns();
}
