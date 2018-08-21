/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef SRC_GUI_HELPERS_NETAWAREFILEDIALOGGTK3_H_
#define SRC_GUI_HELPERS_NETAWAREFILEDIALOGGTK3_H_

#include <config.h>

#if HAVE_GTKMM01

#include <QObject>
#include <QUrl>
#include <QScopedPointer>


#include "Gtk3DialogHelper.h"

///#include <qpa/qplatformdialoghelper.h>
#include <QtGui/qpa/qplatformdialoghelper.h>

typedef struct _GtkDialog GtkDialog;
typedef struct _GtkFileFilter GtkFileFilter;

/**
 * Heavily based on the Qt5 QGtk3FileDialogHelper:
 * https://github.com/qt/qtbase/blob/dev/src/plugins/platformthemes/gtk3/qgtk3dialoghelpers.cpp
 * https://code.woboq.org/qt5/qtbase/src/plugins/platformthemes/gtk3/
 * https://code.woboq.org/qt5/qtbase/src/plugins/platformthemes/gtk3/qgtk3dialoghelpers.cpp.html
 * ...and also QGnomePlatforms:
 * https://github.com/MartinBriza/QGnomePlatform/blob/master/src/qgtk3dialoghelpers.cpp
 */
class NetAwareFileDialogGtk3 : public QPlatformFileDialogHelper
{
    Q_OBJECT

    using BASE_CLASS = QPlatformFileDialogHelper;

#define NAFDGTK3_OVERRIDE override

//Q_SIGNALS:
//    void accept();
//    void currentChanged(const QUrl& selected_url);

public:
	NetAwareFileDialogGtk3();
    ~NetAwareFileDialogGtk3() override;

    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) NAFDGTK3_OVERRIDE;
    void exec() NAFDGTK3_OVERRIDE;
    void hide() NAFDGTK3_OVERRIDE;

    bool defaultNameFilterDisables() const NAFDGTK3_OVERRIDE;
    void setDirectory(const QUrl &directory) NAFDGTK3_OVERRIDE;
    QUrl directory() const NAFDGTK3_OVERRIDE;
    void selectFile(const QUrl &filename) NAFDGTK3_OVERRIDE;
    QList<QUrl> selectedFiles() const NAFDGTK3_OVERRIDE;
    void setFilter() NAFDGTK3_OVERRIDE;
    void selectNameFilter(const QString &filter) NAFDGTK3_OVERRIDE;
    QString selectedNameFilter() const NAFDGTK3_OVERRIDE;

private Q_SLOTS:
    void onAccepted();

private:
    static void onSelectionChanged(GtkDialog *gtkDialog, NetAwareFileDialogGtk3 *helper);
    static void onCurrentFolderChanged(NetAwareFileDialogGtk3 *dialog);
    static void onFilterChanged(NetAwareFileDialogGtk3 *dialog);

    void applyOptions();
    void setNameFilters(const QStringList &filters);
    void selectFileInternal(const QUrl &filename);
    void setFileChooserAction();

    QUrl m_dir;
    QList<QUrl> m_selection;
    QHash<QString, GtkFileFilter*> m_filters;
    QHash<GtkFileFilter*, QString> m_filterNames;
	QScopedPointer<Gtk3DialogHelper> d;

};

#endif // HAVE_GTKMM01 == 1

#endif /* SRC_GUI_HELPERS_NETAWAREFILEDIALOGGTK3_H_ */
