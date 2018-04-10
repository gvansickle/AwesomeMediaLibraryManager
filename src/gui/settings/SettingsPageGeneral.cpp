#include "SettingsPageGeneral.h"
#include "ui_SettingsPageGeneral.h"

SettingsPageGeneral::SettingsPageGeneral(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SettingsPageGeneral)
{
	ui->setupUi(this);
}

SettingsPageGeneral::~SettingsPageGeneral()
{
	delete ui;
}
