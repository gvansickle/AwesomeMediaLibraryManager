#include "SettingsPageDatabase.h"
#include "ui_SettingsPageDatabase.h"

SettingsPageDatabase::SettingsPageDatabase(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SettingsPageDatabase)
{
	ui->setupUi(this);
}

SettingsPageDatabase::~SettingsPageDatabase()
{
	delete ui;
}
