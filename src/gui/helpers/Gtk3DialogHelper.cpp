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

#include <config.h>

#include "Gtk3DialogHelper.h"

#include <utils/DebugHelpers.h>

// We don't want any of this on Windows, or if we don't have GTK.
#if HAVE_GTKMM01 == 1

#include <QtGui/private/qguiapplication_p.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <QEventLoop>


Gtk3DialogHelper::Gtk3DialogHelper(GtkWidget *gtkWidget) : m_gtkWidget(gtkWidget)
{
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "response", G_CALLBACK(onResponse), this);
    g_signal_connect(G_OBJECT(gtkWidget), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
}

Gtk3DialogHelper::~Gtk3DialogHelper()
{
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    gtk_widget_destroy(m_gtkWidget);
}

GtkDialog* Gtk3DialogHelper::gtkDialog() const
{
    return GTK_DIALOG(m_gtkWidget);
}

void Gtk3DialogHelper::exec()
{
    if (modality() == Qt::ApplicationModal)
    {
        // block input to the whole app, including other GTK dialogs
        gtk_dialog_run(gtkDialog());
    }
    else
    {
        // block input to the window, allow input to other GTK dialogs
        QEventLoop loop;
        connect(this, SIGNAL(accept()), &loop, SLOT(quit()));
        connect(this, SIGNAL(reject()), &loop, SLOT(quit()));
        loop.exec();
    }
}

bool Gtk3DialogHelper::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    if (parent)
    {
        // Connection to let us know when the parent is destroyed.
        connect(parent, &QWindow::destroyed, this, &Gtk3DialogHelper::onParentWindowDestroyed,
            Qt::UniqueConnection);
    }

    setParent(parent);
    setFlags(flags);
    setModality(modality);

    // Create an X window so the rest of this has a window to act on.
    gtk_widget_realize(m_gtkWidget);

    GdkWindow *gdkWindow = gtk_widget_get_window(m_gtkWidget);

    if (parent)
    {
    	if(GDK_IS_X11_WINDOW(gdkWindow))
    	{
			GdkDisplay *gdkDisplay = gdk_window_get_display(gdkWindow);
			// Set this as a transient window for the parent.
			XSetTransientForHint(gdk_x11_display_get_xdisplay(gdkDisplay),
								gdk_x11_window_get_xid(gdkWindow),
								parent->winId());
    	}
    }

    if (modality != Qt::NonModal)
    {
        gdk_window_set_modal_hint(gdkWindow, true);
        // See https://code.woboq.org/qt5/qtbase/src/gui/kernel/qguiapplication.cpp.html#_ZN22QGuiApplicationPrivate15showModalWindowEP7QWindow
        QGuiApplicationPrivate::showModalWindow(this);
//        qWarning() << "SHOULD BE CALLING QGuiApplicationPrivate::showModalWindow(this);";
        /// @todo We'll try this for now.
//        BASE_CLASS::show();
    }

    gtk_widget_show(m_gtkWidget);
    gdk_window_focus(gdkWindow, GDK_CURRENT_TIME);

    return true;
}

void Gtk3DialogHelper::hide()
{
    QGuiApplicationPrivate::hideModalWindow(this);
    qWarning() << "SHOULD BE CALLING QGuiApplicationPrivate::hideModalWindow(this);";
//    BASE_CLASS::hide();
    gtk_widget_hide(m_gtkWidget);
}

void Gtk3DialogHelper::onResponse(Gtk3DialogHelper *dialog, int response)
{
    if (response == GTK_RESPONSE_OK)
    {
        Q_EMIT dialog->accept();
    }
    else
    {
        Q_EMIT dialog->reject();
    }
}

void Gtk3DialogHelper::onParentWindowDestroyed()
{
    // The QGtk3*DialogHelper classes own this object, make sure the parent doesn't delete it.
    setParent(0);
}

#endif // HAVE_GTKMM01 == 1

