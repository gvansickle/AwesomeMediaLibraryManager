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

#include "NetAwareFileDialogGtk3.h"

#include <QWindow>
#include <QEventLoop>

#include <utils/DebugHelpers.h>

/// @todo
///#include <private/qguiapplication_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformdialoghelper.h>

#if HAVE_GTKMM01
#include <gtk/gtk.h>

//#include "NetAwareFileDialogGtk3.h"
//#include <QtX11Extras/QX11Info>
//#include <glib-object.h>
//#include <gtkmm.h>
//#include <gtkmm/filechooserdialog.h>
//#include <gdk/gdk.h>
//#include <gdk/gdkx.h>
//#include <gdk/x11/gdkx11window.h>
#endif // HAVE_GTKMM01

#if 1



NetAwareFileDialogGtk3::NetAwareFileDialogGtk3()
{
    // See https://developer.gnome.org/gtk3/stable/GtkFileChooser.html
    d.reset(new Gtk3DialogHelper(gtk_file_chooser_dialog_new("", // Title
                                                      0,  // Parent
                                                      GTK_FILE_CHOOSER_ACTION_OPEN, // The open or save mode.
                                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                      GTK_STOCK_OK, GTK_RESPONSE_OK, NULL)));

    // Connect up accept/reject signals.
    connect(d.data(), SIGNAL(accept()), this, SLOT(onAccepted()));
    connect(d.data(), SIGNAL(reject()), this, SIGNAL(reject()));

    g_signal_connect(GTK_FILE_CHOOSER(d->gtkDialog()), "selection-changed", G_CALLBACK(onSelectionChanged), this);
    g_signal_connect_swapped(GTK_FILE_CHOOSER(d->gtkDialog()), "current-folder-changed", G_CALLBACK(onCurrentFolderChanged), this);
    g_signal_connect_swapped(GTK_FILE_CHOOSER(d->gtkDialog()), "notify::filter", G_CALLBACK(onFilterChanged), this);
}

NetAwareFileDialogGtk3::~NetAwareFileDialogGtk3()
{
    // Nothing to do.
}

bool NetAwareFileDialogGtk3::show(Qt::WindowFlags flags, Qt::WindowModality modality, QWindow *parent)
{
    m_dir.clear();
    m_selection.clear();

    applyOptions();

    return d->show(flags, modality, parent);
}

void NetAwareFileDialogGtk3::exec()
{
    d->exec();
}

void NetAwareFileDialogGtk3::hide()
{
    // We have to capture the current selections before the GtkFileChooserDialog is hidden.
    m_dir = directory();
    m_selection = selectedFiles();

    d->hide();
}

bool NetAwareFileDialogGtk3::defaultNameFilterDisables() const
{
    return false;
}

void NetAwareFileDialogGtk3::setDirectory(const QUrl &directory)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(gtkDialog), qUtf8Printable(directory.toLocalFile()));
}

QUrl NetAwareFileDialogGtk3::directory() const
{
    // While GtkFileChooserDialog is hidden, gtk_file_chooser_get_current_folder()
    // returns a bogus value -> return the cached value before hiding
    if (!m_dir.isEmpty())
    {
        return m_dir;
    }

    QString ret;
    GtkDialog *gtkDialog = d->gtkDialog();
    gchar *folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(gtkDialog));
    if (folder)
    {
        ret = QString::fromUtf8(folder);
        g_free(folder);
    }
    return QUrl::fromLocalFile(ret);
}

void NetAwareFileDialogGtk3::selectFile(const QUrl &filename)
{
    setFileChooserAction();
    selectFileInternal(filename);
}

QList<QUrl> NetAwareFileDialogGtk3::selectedFiles() const
{
    // While GtkFileChooserDialog is hidden, gtk_file_chooser_get_filenames()
    // returns a bogus value -> return the cached value before hiding
    if (!m_selection.isEmpty())
    {
        return m_selection;
    }
    QList<QUrl> selection;
    GtkDialog *gtkDialog = d->gtkDialog();
    GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(gtkDialog));
    for (GSList *it  = filenames; it; it = it->next)
    {
        selection += QUrl::fromLocalFile(QString::fromUtf8((const char*)it->data));
    }
    g_slist_free(filenames);
    return selection;
}

void NetAwareFileDialogGtk3::setFilter()
{
    applyOptions();
}

void NetAwareFileDialogGtk3::selectNameFilter(const QString &filter)
{
    GtkFileFilter *gtkFilter = m_filters.value(filter);
    if (gtkFilter)
    {
        GtkDialog *gtkDialog = d->gtkDialog();
        gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(gtkDialog), gtkFilter);
    }
}

QString NetAwareFileDialogGtk3::selectedNameFilter() const
{
    GtkDialog *gtkDialog = d->gtkDialog();
    GtkFileFilter *gtkFilter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(gtkDialog));
    return m_filterNames.value(gtkFilter);
}

void NetAwareFileDialogGtk3::onAccepted()
{
    Q_EMIT accept();
}

void NetAwareFileDialogGtk3::onSelectionChanged(GtkDialog *gtkDialog, NetAwareFileDialogGtk3 *helper)
{
    QString selection;
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtkDialog));
    if (filename)
    {
        selection = QString::fromUtf8(filename);
        g_free(filename);
    }

    Q_EMIT helper->currentChanged(QUrl::fromLocalFile(selection));
}

void NetAwareFileDialogGtk3::onCurrentFolderChanged(NetAwareFileDialogGtk3 *dialog)
{
    Q_EMIT dialog->directoryEntered(dialog->directory());
}

void NetAwareFileDialogGtk3::onFilterChanged(NetAwareFileDialogGtk3 *dialog)
{
    Q_EMIT dialog->filterSelected(dialog->selectedNameFilter());
}

static GtkFileChooserAction gtkFileChooserAction(const QSharedPointer<QFileDialogOptions> &options)
{
    switch (options->fileMode()) {
    case QFileDialogOptions::AnyFile:
    case QFileDialogOptions::ExistingFile:
    case QFileDialogOptions::ExistingFiles:
        if (options->acceptMode() == QFileDialogOptions::AcceptOpen)
            return GTK_FILE_CHOOSER_ACTION_OPEN;
        else
            return GTK_FILE_CHOOSER_ACTION_SAVE;
    case QFileDialogOptions::Directory:
    case QFileDialogOptions::DirectoryOnly:
    default:
        if (options->acceptMode() == QFileDialogOptions::AcceptOpen)
            return GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
        else
            return GTK_FILE_CHOOSER_ACTION_CREATE_FOLDER;
    }
}

void NetAwareFileDialogGtk3::applyOptions()
{
    GtkDialog *gtkDialog = d->gtkDialog();
    const QSharedPointer<QFileDialogOptions> &opts = options();

    gtk_window_set_title(GTK_WINDOW(gtkDialog), qUtf8Printable(opts->windowTitle()));
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(gtkDialog), true);

    setFileChooserAction();

    const bool selectMultiple = opts->fileMode() == QFileDialogOptions::ExistingFiles;
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(gtkDialog), selectMultiple);

    const bool confirmOverwrite = !opts->testOption(QFileDialogOptions::DontConfirmOverwrite);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(gtkDialog), confirmOverwrite);

    const bool readOnly = opts->testOption(QFileDialogOptions::ReadOnly);
    gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(gtkDialog), !readOnly);

    const QStringList nameFilters = opts->nameFilters();
    if (!nameFilters.isEmpty())
    {
        setNameFilters(nameFilters);
    }

    if (opts->initialDirectory().isLocalFile())
    {
        setDirectory(opts->initialDirectory());
    }

    for(const QUrl &filename : opts->initiallySelectedFiles())
    {
        selectFileInternal(filename);
    }

    const QString initialNameFilter = opts->initiallySelectedNameFilter();
    if (!initialNameFilter.isEmpty())
    {
        selectNameFilter(initialNameFilter);
    }

    GtkWidget *acceptButton = gtk_dialog_get_widget_for_response(gtkDialog, GTK_RESPONSE_OK);
    if (acceptButton)
    {
        if (opts->isLabelExplicitlySet(QFileDialogOptions::Accept))
        {
            gtk_button_set_label(GTK_BUTTON(acceptButton), qUtf8Printable(opts->labelText(QFileDialogOptions::Accept)));
        }
        else if (opts->acceptMode() == QFileDialogOptions::AcceptOpen)
        {
            gtk_button_set_label(GTK_BUTTON(acceptButton), GTK_STOCK_OPEN);// qUtf8Printable(QGtk3Theme::defaultStandardButtonText(QPlatformDialogHelper::Open)));
        }
        else
        {
            gtk_button_set_label(GTK_BUTTON(acceptButton), GTK_STOCK_SAVE); //qUtf8Printable(QGtk3Theme::defaultStandardButtonText(QPlatformDialogHelper::Save)));
        }
    }

    GtkWidget *rejectButton = gtk_dialog_get_widget_for_response(gtkDialog, GTK_RESPONSE_CANCEL);
    if (rejectButton)
    {
        if (opts->isLabelExplicitlySet(QFileDialogOptions::Reject))
        {
            gtk_button_set_label(GTK_BUTTON(rejectButton), qUtf8Printable(opts->labelText(QFileDialogOptions::Reject)));
        }
        else
        {
            gtk_button_set_label(GTK_BUTTON(rejectButton), GTK_STOCK_CANCEL); //qUtf8Printable(QGtk3Theme::defaultStandardButtonText(QPlatformDialogHelper::Cancel)));
        }
    }
}

void NetAwareFileDialogGtk3::setNameFilters(const QStringList &filters)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    for(GtkFileFilter *filter : m_filters)
    {
       gtk_file_chooser_remove_filter(GTK_FILE_CHOOSER(gtkDialog), filter);
    }

    m_filters.clear();
    m_filterNames.clear();

    for(const QString &filter : filters)
    {
       GtkFileFilter *gtkFilter = gtk_file_filter_new();
       const QString name = filter.left(filter.indexOf(QLatin1Char('(')));
       const QStringList extensions = cleanFilterList(filter);

       gtk_file_filter_set_name(gtkFilter, name.isEmpty() ? extensions.join(QStringLiteral(", ")).toUtf8() : name.toUtf8());
       for(const QString &ext : extensions)
       {
           gtk_file_filter_add_pattern(gtkFilter, ext.toUtf8());
       }

       gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(gtkDialog), gtkFilter);

       m_filters.insert(filter, gtkFilter);
       m_filterNames.insert(gtkFilter, filter);
    }
}

void NetAwareFileDialogGtk3::selectFileInternal(const QUrl &filename)
{
    GtkDialog *gtkDialog = d->gtkDialog();
    if (options()->acceptMode() == QFileDialogOptions::AcceptSave)
    {
        QFileInfo fi(filename.toLocalFile());
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(gtkDialog), qUtf8Printable(fi.path()));
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(gtkDialog), qUtf8Printable(fi.fileName()));
    }
    else
    {
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(gtkDialog), qUtf8Printable(filename.toLocalFile()));
    }
}

void NetAwareFileDialogGtk3::setFileChooserAction()
{
    GtkDialog *gtkDialog = d->gtkDialog();

    const GtkFileChooserAction action = gtkFileChooserAction(options());
    gtk_file_chooser_set_action(GTK_FILE_CHOOSER(gtkDialog), action);
}

#endif ////
// m_dlgHelper = static_cast<QPlatformFileDialogHelper*>(QGuiApplicationPrivate::platformTheme()->createPlatformDialogHelper(QPlatformTheme::FileDialog));

