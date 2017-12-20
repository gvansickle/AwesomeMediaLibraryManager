/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
 *
 * This file is part of AwesomeMediaLibraryManager.
 *
 * AwesomeMediaLibraryManager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AwesomeMediaLibraryManager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AwesomeMediaLibraryManager.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOGBASE_H
#define AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOGBASE_H

#include <QDialog>
#include <QPointer>
#include <QDialogButtonBox>
#include <QStackedWidget>

class QWidget;

#include "SettingsDialogSideWidget.h"
#include "SettingsDialogPageBase.h"
#include "RegisteredField.h"

class SettingsDialogBase : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialogBase(QWidget *parent = nullptr, const Qt::WindowFlags &flags = 0);

    void addPage(SettingsDialogPageBase *page);

    void setField(const QString &name, const QVariant &value);
    QVariant field(const QString &name) const;

public slots:
    void changePage(QListWidgetItem *current, QListWidgetItem *previous);

    void onHelpRequested();

    void accept() override;

private:

    // SettingsPages are friended to make the field() mechanism easier.
    friend SettingsDialogPageBase;

    void addField(const RegisteredField& field);
    void registerField(const QString &name, QWidget *widget, const char *property = Q_NULLPTR, const char *changedSignal = Q_NULLPTR);

    QPointer<SettingsDialogSideWidget> m_contents_side_widget;

    QDialogButtonBox m_dialog_button_box;

    /// The stacked widget which will hold the pages.
    QStackedWidget m_page_stack_widget;

    /// The map of registeredField() names to values.
    QMap<QString, int> m_reg_field_index_map;
    QVector<RegisteredField> m_registered_fields;
};


#endif //AWESOMEMEDIALIBRARYMANAGER_SETTINGSDIALOGBASE_H
