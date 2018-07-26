#include "CollectionView.h"
#include "ui_CollectionView.h"

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
    ui->tableView->setModel(model);
    ui->tableView->setItemDelegate(new QSqlRelationalDelegate(ui->tableView));
}

QTableView *CollectionView::getTableView()
{
    return ui->tableView;
}
