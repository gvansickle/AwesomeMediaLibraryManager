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

#ifndef SCANRESULTSTABLEMODEL_H
#define SCANRESULTSTABLEMODEL_H

// Std C++
//#include <deque>
//#include <vector>
//#include <string>

#include <QObject>
#include <QVector>
#include <QVariant>

#include "EnhancedAbstractTableModel.h"
#include <logic/DirScanResult.h>


class ScanResultsTableModel : public EnhancedAbstractTableModel
{
	Q_OBJECT

	using BASE_CLASS = EnhancedAbstractTableModel;

public:
	explicit ScanResultsTableModel(QObject *parent = nullptr);


	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;

	virtual void appendRow(DirScanResult dsr);

protected:

	QVariant getData(int row, int col, int role = Qt::DisplayRole) const override;

	using rowtype = QVariantList;
	QVector<rowtype> m_scan_results;
};

#endif // SCANRESULTSTABLEMODEL_H
