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

#include "NetAwareFileDialogGtk3.h"

#include <QWindow>
#include <QEventLoop>

#include <utils/DebugHelpers.h>

/// @todo
//#include <private/qguiapplication_p.h>

#warning "MOVE TO CMAKE"
#define HAVE_GTKMM

#ifdef HAVE_GTKMM
#include "NetAwareFileDialogGtk3.h"
#include <QtX11Extras/QX11Info>
#include <glib-object.h>
#include <gtkmm.h>
#include <gtkmm/filechooserdialog.h>
//#include <gdk/gdk.h>
#include <gdk/gdkx.h>
//#include <gdk/x11/gdkx11window.h>
#endif // HAVE_GTKMM

#if 0
class QGtk3Dialog : public QWindow
{
    Q_OBJECT

public:
    QGtk3Dialog(GtkWidget *gtkWidget);
    ~QGtk3Dialog();

    GtkDialog *gtkDialog() const;

    void exec();
    bool show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent);
    void hide();

Q_SIGNALS:
    void accept();
    void reject();

protected:
    static void onResponse(QGtk3Dialog *dialog, int response);

private Q_SLOTS:
    void onParentWindowDestroyed();

private:
    GtkWidget *gtkWidget;
};

QGtk3Dialog::QGtk3Dialog(GtkWidget *gtkWidget) : gtkWidget(gtkWidget)
{
    g_signal_connect_swapped(G_OBJECT(gtkWidget), "response", G_CALLBACK(onResponse), this);
    g_signal_connect(G_OBJECT(gtkWidget), "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
}

QGtk3Dialog::~QGtk3Dialog()
{
    gtk_clipboard_store(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD));
    gtk_widget_destroy(gtkWidget);
}

GtkDialog *QGtk3Dialog::gtkDialog() const
{
    return GTK_DIALOG(gtkWidget);
}

void QGtk3Dialog::exec()
{
    if (modality() == Qt::ApplicationModal) {
        // block input to the whole app, including other GTK dialogs
        gtk_dialog_run(gtkDialog());
    } else {
        // block input to the window, allow input to other GTK dialogs
        QEventLoop loop;
        connect(this, SIGNAL(accept()), &loop, SLOT(quit()));
        connect(this, SIGNAL(reject()), &loop, SLOT(quit()));
        loop.exec();
    }
}

bool QGtk3Dialog::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    if (parent) {
        connect(parent, &QWindow::destroyed, this, &QGtk3Dialog::onParentWindowDestroyed,
            Qt::UniqueConnection);
    }
    setParent(parent);
    setFlags(flags);
    setModality(modality);

    gtk_widget_realize(gtkWidget); // creates X window

    GdkWindow *gdkWindow = gtk_widget_get_window(gtkWidget);
#ifdef GDK_WINDOWING_X11
    if (parent) {
        GdkDisplay *gdkDisplay = gdk_window_get_display(gdkWindow);
        if (GDK_IS_X11_DISPLAY (gdkDisplay)) {
            XSetTransientForHint(gdk_x11_display_get_xdisplay(gdkDisplay),
                                gdk_x11_window_get_xid(gdkWindow),
                                parent->winId());
        }
    }
#endif

    if (modality != Qt::NonModal)
    {
        gdk_window_set_modal_hint(gdkWindow, true);
        // See https://code.woboq.org/qt5/qtbase/src/gui/kernel/qguiapplication.cpp.html#_ZN22QGuiApplicationPrivate15showModalWindowEP7QWindow
//        QGuiApplicationPrivate::showModalWindow(this);
        qWarning() << "SHOWLD BE CALLING QGuiApplicationPrivate::showModalWindow(this);";
    }

    gtk_widget_show(gtkWidget);
    gdk_window_focus(gdkWindow, GDK_CURRENT_TIME);
    return true;
}

void QGtk3Dialog::hide()
{
//    QGuiApplicationPrivate::hideModalWindow(this);
    qWarning() << "SHOWLD BE CALLING QGuiApplicationPrivate::hideModalWindow(this);";
    gtk_widget_hide(gtkWidget);
}

void QGtk3Dialog::onResponse(QGtk3Dialog *dialog, int response)
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

void QGtk3Dialog::onParentWindowDestroyed()
{
    // The QGtk3*DialogHelper classes own this object. Make sure the parent doesn't delete it.
    setParent(0);
}




NetAwareFileDialogGtk3::NetAwareFileDialogGtk3()
{
	// TODO Auto-generated constructor stub

}
#endif ////
// m_dlgHelper = static_cast<QPlatformFileDialogHelper*>(QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(QPlatformTheme::FileDialog));

#include "NetAwareFileDialogGtk3.moc"
