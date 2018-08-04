#include "ScanResultsTableModel.h"

#include <utils/StringHelpers.h>
#include <utils/DebugHelpers.h>

ScanResultsTableModel::ScanResultsTableModel(QObject *parent) : BASE_CLASS(parent)
{

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
