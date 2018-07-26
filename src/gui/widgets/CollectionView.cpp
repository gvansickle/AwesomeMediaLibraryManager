#include "CollectionView.h"
#include "ui_CollectionView.h"

#include <logic/models/AbstractTreeModel.h>

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

void CollectionView::setMainModel(QSqlRelationalTableModel *model)
{
    auto view = ui->treeView;
    view->setModel(model);
	view->setItemDelegate(new QSqlRelationalDelegate(view));
}

void CollectionView::setPane2Model(AbstractTreeModel* model)
{
	auto view = ui->treeView_exp;
	view->setModel(model);
	for (int column = 0; column < model->columnCount(); ++column)
	{
		view->resizeColumnToContents(column);
	}
}


