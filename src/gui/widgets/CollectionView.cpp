#include "CollectionView.h"
#include "ui_CollectionView.h"

#include <logic/models/AbstractTreeModel.h>

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

void CollectionView::setMainModel(QSqlRelationalTableModel *model)
{
    auto view = ui->treeView;
    view->setModel(model);
	view->setItemDelegate(new QSqlRelationalDelegate(view));

	model->select();

	auto tmr = new QTimer(this);
	connect(tmr, &QTimer::timeout, view, [=](){
		qDebug() << "Trying to refresh";
//		model->submitAll();
		model->select();
//		view->selectAll();
	});
	tmr->start(1000);
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


