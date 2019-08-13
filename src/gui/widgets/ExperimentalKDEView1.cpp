#include "ExperimentalKDEView1.h"
#include "ui_ExperimentalKDEView1.h"

#include <utils/ConnectHelpers.h>
#include <proxymodels/ModelHelpers.h>

ExperimentalKDEView1::ExperimentalKDEView1(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ExperimentalKDEView1)
{
	ui->setupUi(this);
}

ExperimentalKDEView1::~ExperimentalKDEView1()
{
	delete ui;
}

bool ExperimentalKDEView1::setModel(AbstractTreeModel* model)
{
	auto view = ui->m_top_level_tree_view;

	Q_CHECK_PTR(model);

	// Put the URLs in column 0.
	m_column_remapper_proxy = QSharedPointer<KRearrangeColumnsProxyModel>::create(this);
	m_column_remapper_proxy->setSourceModel(model);
	QVector<int> remapping;
	remapping << 1;
	m_column_remapper_proxy->setSourceColumns(remapping);

	M_MESSAGE("I can't get the proxy model or view to work");
	m_cat_proxy_model = QSharedPointer<KCategorizedSortFilterProxyModel>::create(this);
	m_cat_proxy_model->setSourceModel(m_column_remapper_proxy.get());
	m_cat_proxy_model->setCategorizedModel(true);
	Q_ASSERT(0 == m_cat_proxy_model->sortColumn());


	view->setModel(model);
//	view->setModel(m_cat_proxy_model.get());
//	view->setCategoryDrawer(m_cat_drawer.get());
//	view->setAlternatingBlockColors(true);
//	view->setCollapsibleBlocks(false);
//	view->setCategorySpacing(24);

	view->expandAll();

	// Hook up Just-In-Time item expansion.
	connect_jit_item_expansion(ui->m_top_level_tree_view->model(), ui->m_top_level_tree_view, this);

	return true;
}
