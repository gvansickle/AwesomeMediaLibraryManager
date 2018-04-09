#include "SettingsPageAppearance.h"
#include "ui_SettingsPageAppearance.h"

SettingsPageAppearance::SettingsPageAppearance(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SettingsPageAppearance)
{
	ui->setupUi(this);
}

SettingsPageAppearance::~SettingsPageAppearance()
{
	delete ui;
}
