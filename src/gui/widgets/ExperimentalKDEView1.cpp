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
