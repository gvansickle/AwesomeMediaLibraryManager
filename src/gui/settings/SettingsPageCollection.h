#ifndef SETTINGSPAGECOLLECTION_H
#define SETTINGSPAGECOLLECTION_H

#include <QWidget>

namespace Ui {
class SettingsPageCollection;
}

class SettingsPageCollection : public QWidget
{
	Q_OBJECT

public:
	explicit SettingsPageCollection(QWidget *parent = 0);
	~SettingsPageCollection();

private:
	Ui::SettingsPageCollection *ui;
};

#endif // SETTINGSPAGECOLLECTION_H
