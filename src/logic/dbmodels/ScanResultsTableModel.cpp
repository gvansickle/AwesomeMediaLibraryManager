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
	auto retval = m_scan_results[row][col];
//	qDbo() << "RETURNING:" << row << col << role << retval;
	return retval;
}
