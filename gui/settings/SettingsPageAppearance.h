#ifndef SETTINGSPAGEAPPEARANCE_H
#define SETTINGSPAGEAPPEARANCE_H

#include <QWidget>

namespace Ui {
class SettingsPageAppearance;
}

class SettingsPageAppearance : public QWidget
{
	Q_OBJECT

public:
	explicit SettingsPageAppearance(QWidget *parent = nullptr);
	~SettingsPageAppearance();

private:
	Ui::SettingsPageAppearance *ui;
};

#endif // SETTINGSPAGEAPPEARANCE_H
