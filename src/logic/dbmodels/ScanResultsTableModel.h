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

	virtual void appendRow(DirScanResult dsr);

protected:

	using rowtype = QVariantList;
	QVector<rowtype> m_scan_results;
};

#endif // SCANRESULTSTABLEMODEL_H
