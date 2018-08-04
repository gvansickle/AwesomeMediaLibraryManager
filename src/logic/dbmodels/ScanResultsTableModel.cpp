#include "ScanResultsTableModel.h"

#include <utils/StringHelpers.h>
#include <utils/DebugHelpers.h>

ScanResultsTableModel::ScanResultsTableModel(QObject *parent) : BASE_CLASS(parent)
{
	setHeaderData(0, Qt::Horizontal, "Column 0");
	setHeaderData(1, Qt::Horizontal, "Column 1");
	setHeaderData(2, Qt::Horizontal, "Column 2");

}

int ScanResultsTableModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		return 0;
	}

	return m_scan_results.size();
}

int ScanResultsTableModel::columnCount(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		return 0;
	}

	return 3;
}

void ScanResultsTableModel::appendRow(DirScanResult dsr)
{
	qDbo() << "APPENDING:" << dsr;
	beginInsertRows(QModelIndex(), rowCount(), rowCount()+1);

	QVariantList row;
	row.append(QVariant::fromValue(dsr.getDirProps()));
	row.append(QVariant::fromValue(dsr.getMediaExtUrl()));
	row.append(QVariant::fromValue(dsr.getSidecarCuesheetExtUrl()));

	m_scan_results.append(row);
	endInsertRows();
}

QVariant ScanResultsTableModel::getData(int row, int col, int role) const
{
	return m_scan_results[row][col];
}
