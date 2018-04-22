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
	explicit SettingsPageLibrary(QWidget *parent = 0);
	~SettingsPageLibrary();

private:
	Ui::SettingsPageLibrary *ui;
};

#endif // SETTINGSPAGELIBRARY_H
