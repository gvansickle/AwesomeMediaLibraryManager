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

	virtual QVariant getData(int row, int col, int role = Qt::DisplayRole) const;

	using rowtype = QVariantList;
	QVector<rowtype> m_scan_results;
};

#endif // SCANRESULTSTABLEMODEL_H
