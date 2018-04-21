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

#include <QObject>


//#include <qpa/qplatformdialoghelper.h>

/**
 * Heavily based on the Qt5 QGtk3*DialogHelpers:
 * https://code.woboq.org/qt5/qtbase/src/plugins/platformthemes/gtk3/
 * https://code.woboq.org/qt5/qtbase/src/plugins/platformthemes/gtk3/qgtk3dialoghelpers.cpp.html
 */

#if 0

class NetAwareFileDialogGtk3 : public QObject //QPlatformFileDialogHelper
{
    Q_OBJECT

//    using BASE_CLASS = QPlatformFileDialogHelper;

public:
	NetAwareFileDialogGtk3();
    ~NetAwareFileDialogGtk3();

//    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent) override;
//    void exec() override;
//    void hide() override;

//    bool defaultNameFilterDisables() const override;
//    void setDirectory(const QUrl &directory) override;
//    QUrl directory() const override;
//    void selectFile(const QUrl &filename) override;
//    QList<QUrl> selectedFiles() const override;
//    void setFilter() override;
//    void selectNameFilter(const QString &filter) override;
//    QString selectedNameFilter() const override;

//private Q_SLOTS:
//    void onAccepted();

//private:
//    static void onSelectionChanged(GtkDialog *dialog, QGtk3FileDialogHelper *helper);
//    static void onCurrentFolderChanged(QGtk3FileDialogHelper *helper);
//    static void onFilterChanged(QGtk3FileDialogHelper *helper);
//    void applyOptions();
//    void setNameFilters(const QStringList &filters);
//    void selectFileInternal(const QUrl &filename);
//    void setFileChooserAction();

//    QUrl _dir;
//    QList<QUrl> _selection;
//    QHash<QString, GtkFileFilter*> _filters;
//    QHash<GtkFileFilter*, QString> _filterNames;
//    QScopedPointer<QGtk3Dialog> d;

};
#endif

#endif /* SRC_GUI_HELPERS_NETAWAREFILEDIALOGGTK3_H_ */
