#include "ExpTreeView.h"
#include "ui_ExpTreeView.h"

ExpTreeView::ExpTreeView(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ExpTreeView)
{
	ui->setupUi(this);
}

ExpTreeView::~ExpTreeView()
{
	delete ui;
}
