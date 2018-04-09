#include "SettingsPageCollection.h"
#include "ui_SettingsPageCollection.h"

SettingsPageCollection::SettingsPageCollection(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SettingsPageCollection)
{
	ui->setupUi(this);
}

SettingsPageCollection::~SettingsPageCollection()
{
	delete ui;
}
