#ifndef SCANRESULTSTABLEMODEL_H
#define SCANRESULTSTABLEMODEL_H

#include <QObject>

#include "EnhancedAbstractTableModel.h"
#include <logic/DirScanResult.h>

class ScanResultsTableModel : public EnhancedAbstractTableModel
{
	Q_OBJECT

	using BASE_CLASS = EnhancedAbstractTableModel;

public:
	explicit ScanResultsTableModel(QObject *parent = nullptr);

	virtual void appendRow(DirScanResult dsr);
};

#endif // SCANRESULTSTABLEMODEL_H
