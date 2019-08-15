#ifndef SETTINGSPAGEDATABASE_H
#define SETTINGSPAGEDATABASE_H

#include <QWidget>

namespace Ui {
class SettingsPageDatabase;
}

class SettingsPageDatabase : public QWidget
{
	Q_OBJECT

public:
	explicit SettingsPageDatabase(QWidget *parent = nullptr);
	~SettingsPageDatabase();

private:
	Ui::SettingsPageDatabase *ui;
};

#endif // SETTINGSPAGEDATABASE_H
