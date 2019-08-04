#include "CollectionView.h"
#include "ui_CollectionView.h"

//#include <logic/models/AbstractTreeModel.h>
#include <logic/models/treemodel.h>

#include <QTimer>
#include <QDebug>

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
	auto view = ui->treeView_exp;
	view->setModel(model);
	for (int column = 0; column < model->columnCount(); ++column)
	{
		view->resizeColumnToContents(column);
	}
}


