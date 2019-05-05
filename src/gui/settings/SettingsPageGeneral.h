#ifndef SETTINGSPAGEGENERAL_H
#define SETTINGSPAGEGENERAL_H

#include <QWidget>

namespace Ui {
class SettingsPageGeneral;
}

class SettingsPageGeneral : public QWidget
{
	Q_OBJECT

public:
	explicit SettingsPageGeneral(QWidget *parent = nullptr);
	~SettingsPageGeneral() override;

private:
	Ui::SettingsPageGeneral *ui;
};

#endif // SETTINGSPAGEGENERAL_H
