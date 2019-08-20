#include "CollectionView.h"
#include "ui_CollectionView.h"

// Qt5
#include <QTimer>
#include <QDebug>

// Ours
#include <proxymodels/ModelHelpers.h>
//#include <logic/models/AbstractTreeModel.h>
#include <logic/models/treemodel.h>


CollectionView::CollectionView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CollectionView)
{
    ui->setupUi(this);
}

CollectionView::~CollectionView()
{
    delete ui;
}

void CollectionView::setMainModel2(ScanResultsTableModel* model)
{
	auto view = ui->treeView;
	view->setModel(model);
}

void CollectionView::setPane2Model(TreeModel* model)
{
	auto right_tree_view_atmi = ui->m_right_treeView;
	right_tree_view_atmi->setModel(model);
	for (int column = 0; column < model->columnCount(); ++column)
	{
		right_tree_view_atmi->resizeColumnToContents(column);
	}

	// Hook up Just-In-Time item expansion.
	connect_jit_item_expansion(right_tree_view_atmi->model(), right_tree_view_atmi, this);
}


