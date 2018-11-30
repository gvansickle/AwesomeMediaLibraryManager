#ifndef EXPERIMENTALKDEVIEW1_H
#define EXPERIMENTALKDEVIEW1_H

// Qt5
#include <QWidget>
#include <QSharedPointer>

// KF5
#include <KItemViews/KCategorizedSortFilterProxyModel>
#include <KItemViews/KCategoryDrawer>
#include <KItemModels/KRearrangeColumnsProxyModel>

// Ours.
#include <logic/models/ScanResultsTreeModel.h>


namespace Ui
{
class ExperimentalKDEView1;
}

class ExperimentalKDEView1 : public QWidget
{
	Q_OBJECT

public:
	explicit ExperimentalKDEView1(QWidget *parent = nullptr);
	~ExperimentalKDEView1();

	bool setModel(ScanResultsTreeModel* model);

private:
	Ui::ExperimentalKDEView1 *ui;

	QSharedPointer<KRearrangeColumnsProxyModel> m_column_remapper_proxy {nullptr};
	QSharedPointer<KCategorizedSortFilterProxyModel> m_cat_proxy_model {nullptr};
	QSharedPointer<KCategoryDrawer> m_cat_drawer {nullptr};
};

#endif // EXPERIMENTALKDEVIEW1_H
