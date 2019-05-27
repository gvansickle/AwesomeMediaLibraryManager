#ifndef SETTINGSPAGELIBRARY_H
#define SETTINGSPAGELIBRARY_H

#include <QWidget>

namespace Ui {
class SettingsPageLibrary;
}

class SettingsPageLibrary : public QWidget
{
	Q_OBJECT

public:
	explicit SettingsPageLibrary(QWidget *parent = nullptr);
	~SettingsPageLibrary() override;

private:
	Ui::SettingsPageLibrary *ui;
};

#endif // SETTINGSPAGELIBRARY_H
