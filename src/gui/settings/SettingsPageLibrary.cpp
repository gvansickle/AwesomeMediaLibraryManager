#include "SettingsPageLibrary.h"
#include "ui_SettingsPageLibrary.h"

SettingsPageLibrary::SettingsPageLibrary(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SettingsPageLibrary)
{
	ui->setupUi(this);
}

SettingsPageLibrary::~SettingsPageLibrary()
{
	delete ui;
}
