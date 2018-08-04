#include "ScanResultsTableModel.h"

#include <utils/DebugHelpers.h>

ScanResultsTableModel::ScanResultsTableModel(QObject *parent) : BASE_CLASS(parent)
{

}

void ScanResultsTableModel::appendRow(DirScanResult dsr)
{
	qDbo() << "GOT HERE";
}
