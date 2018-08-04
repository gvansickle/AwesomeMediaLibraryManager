#include "EnhancedAbstractListModel.h"

EnhancedAbstractListModel::EnhancedAbstractListModel(QObject *parent)
	: QAbstractListModel(parent)
{
}

QVariant EnhancedAbstractListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	// FIXME: Implement me!
}

bool EnhancedAbstractListModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
	if (value != headerData(section, orientation, role))
	{
		// FIXME: Implement me!
		Q_EMIT headerDataChanged(orientation, section, section);
		return true;
	}
	return false;
}

int EnhancedAbstractListModel::rowCount(const QModelIndex &parent) const
{
	// For list models only the root node (an invalid parent) should return the list's size. For all
	// other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
	if (parent.isValid())
		return 0;

	// FIXME: Implement me!
}

QVariant EnhancedAbstractListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	// FIXME: Implement me!
	return QVariant();
}

bool EnhancedAbstractListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (data(index, role) != value) {
		// FIXME: Implement me!
		Q_EMIT dataChanged(index, index, QVector<int>() << role);
		return true;
	}
	return false;
}

Qt::ItemFlags EnhancedAbstractListModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	return Qt::ItemIsEditable; // FIXME: Implement me!
}

bool EnhancedAbstractListModel::insertRows(int row, int count, const QModelIndex &parent)
{
	beginInsertRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endInsertRows();
}

bool EnhancedAbstractListModel::removeRows(int row, int count, const QModelIndex &parent)
{
	beginRemoveRows(parent, row, row + count - 1);
	// FIXME: Implement me!
	endRemoveRows();
}
