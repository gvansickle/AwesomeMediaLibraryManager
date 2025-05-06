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

#ifndef SRC_GUI_HELPERS_GTK3DIALOGHELPER_H_
#define SRC_GUI_HELPERS_GTK3DIALOGHELPER_H_

/// @file

#include <config.h>

#if HAVE_GTKMM01 == 1

#include <QWindow>

//#include <gtk/gtk.h>
//#include <gtkmm.h>
typedef struct _GtkDialog GtkDialog;
typedef struct _GtkWidget GtkWidget;

/**
 * Heavily based on the Qt5 QGtk3Dialog class from the gtk3 platform theme:
 * https://github.com/qt/qtbase/blob/dev/src/plugins/platformthemes/gtk3/qgtk3dialoghelpers.cpp
 * https://code.woboq.org/qt5/qtbase/src/plugins/platformthemes/gtk3/
 * https://code.woboq.org/qt5/qtbase/src/plugins/platformthemes/gtk3/qgtk3dialoghelpers.cpp.html
 */
class Gtk3DialogHelper : public QWindow
{
    Q_OBJECT

    using BASE_CLASS = QWindow;

public:
    Gtk3DialogHelper(GtkWidget *m_gtkWidget);
    ~Gtk3DialogHelper();

    GtkDialog *gtkDialog() const;

    void exec();
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent);
    void hide();

Q_SIGNALS:
    void accept();
    void reject();

protected:
    static void onResponse(Gtk3DialogHelper *dialog, int response);

private Q_SLOTS:
    void onParentWindowDestroyed();

private:
    GtkWidget *m_gtkWidget;
};

#endif // HAVE_GTKMM01 == 1

#endif /* SRC_GUI_HELPERS_GTK3DIALOGHELPER_H_ */
