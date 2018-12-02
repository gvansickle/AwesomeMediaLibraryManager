#include "ExperimentalKDEView1.h"
#include "ui_ExperimentalKDEView1.h"



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

bool ExperimentalKDEView1::setModel(ScanResultsTreeModel* model)
{
	auto view = ui->m_top_level_tree_view;

	// Put the URLs in column 0.
	m_column_remapper_proxy = QSharedPointer<KRearrangeColumnsProxyModel>::create(this);
	m_column_remapper_proxy->setSourceModel(model);
	QVector<int> remapping;
	remapping << 1;
	m_column_remapper_proxy->setSourceColumns(remapping);

	m_cat_proxy_model = QSharedPointer<KCategorizedSortFilterProxyModel>::create(this);

	m_cat_proxy_model->setSourceModel(m_column_remapper_proxy.get());
	m_cat_proxy_model->setCategorizedModel(true);
	Q_ASSERT(0 == m_cat_proxy_model->sortColumn());

	M_MESSAGE("THIS DOESN'T WORK FOR SOME REASON");

	view->setModel(m_cat_proxy_model.get());
//	view->setCategoryDrawer(m_cat_drawer.get());
//	view->setAlternatingBlockColors(true);
//	view->setCollapsibleBlocks(false);
//	view->setCategorySpacing(24);

	return true;
}
