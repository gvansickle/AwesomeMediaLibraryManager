#ifndef EXPERIMENTALKDEVIEW1_H
#define EXPERIMENTALKDEVIEW1_H

// Qt
#include <QWidget>
#include <QSharedPointer>

// KF5
#if 0 // KF5
#include <KItemViews/KCategorizedSortFilterProxyModel>
#include <KItemViews/KCategoryDrawer>
#include <KItemModels/KRearrangeColumnsProxyModel>
#elif 1 // KF6
#include <KF6/KItemViews/KCategorizedSortFilterProxyModel>
#include <KF6/KItemViews/KCategoryDrawer>
#include <KF6/KItemModels/KRearrangeColumnsProxyModel>
#endif

// Ours.
#include <logic/models/ScanResultsTreeModel.h>
#include <logic/models/AbstractTreeModel.h>


namespace Ui
{
class ExperimentalKDEView1;
}

class ExperimentalKDEView1 : public QWidget
{
	Q_OBJECT

public:
	explicit ExperimentalKDEView1(QWidget *parent = nullptr);
	~ExperimentalKDEView1() override;

//	bool setModel(ScanResultsTreeModel* model);
	bool setModel(AbstractTreeModel* model);


private:
	Ui::ExperimentalKDEView1 *ui;

	QSharedPointer<KRearrangeColumnsProxyModel> m_column_remapper_proxy {nullptr};
	QSharedPointer<KCategorizedSortFilterProxyModel> m_cat_proxy_model {nullptr};
	QSharedPointer<KCategoryDrawer> m_cat_drawer {nullptr};
};

#endif // EXPERIMENTALKDEVIEW1_H
