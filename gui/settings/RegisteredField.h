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

#ifndef AWESOMEMEDIALIBRARYMANAGER_REGISTEREDFIELD_H
#define AWESOMEMEDIALIBRARYMANAGER_REGISTEREDFIELD_H


#include <QString>
#include <QByteArray>
#include <QObject>
#include <QVariant>

class SettingsDialogPageBase;

class RegisteredField
{
public:
    RegisteredField() = default;
    RegisteredField(SettingsDialogPageBase *page, const QString &spec, QObject *object, const char *property,
                    const char *changedSignal);

    SettingsDialogPageBase* m_page;
    QString m_name;
    QObject *m_object;
    QByteArray m_property_name;
    QByteArray m_changed_signal;
    QVariant m_initial_value;
};


#endif //AWESOMEMEDIALIBRARYMANAGER_REGISTEREDFIELD_H
