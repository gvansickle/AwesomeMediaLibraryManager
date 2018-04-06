/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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


#include "MainWindow.h"

#include <KMainWindow>
#include <KHelpMenu>
#include <KToolBar>
#include <KToggleToolBarAction>
#include <KShortcutsDialog>
#include <KActionCollection>
#include <KActionMenu>
#include <KMessageBox>
#include <KAboutData>
#include <KSharedConfig>


#include "Experimental.h"
#include "FilterWidget.h"

#include "MDITreeViewBase.h"
#include "MDILibraryView.h"
#include "MDIPlaylistView.h"
#include "MDINowPlayingView.h"

// For KF5 KConfig infrastructure.
//#include <KConfigDialog>
#include <AMLMSettings.h>
#include <gui/settings/SettingsDialog.h>

#include <logic/LibraryModel.h>
#include <logic/PlaylistModel.h>

#include "gui/MDIArea.h"
#include "MetadataDockWidget.h"
#include "CollectionDockWidget.h"
#include "PlayerControls.h"

#include "logic/LibraryEntryMimeData.h"

#include "logic/LibrarySortFilterProxyModel.h"

#include "utils/ConnectHelpers.h"
#include "utils/ActionHelpers.h"

#include <QObject>

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QLabel>
#include <QMenuBar>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <Qt>
#include <QAction>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QDebug>
#include "utils/DebugHelpers.h"
#include <QMdiSubWindow>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include <QComboBox>
#include <QStyleFactory>
#include <QDirIterator>
#include <QClipboard>
#include <QSharedPointer>
#include <QStandardItem>


#include <functional>
#include <algorithm>
#include <type_traits>

#include <logic/MP2.h>
#include <utils/Theme.h>
#include <QtCore/QThread>
#include <QtWidgets/QWhatsThis>
#include <QMimeData>
#include "logic/LibraryEntryMimeData.h"

#include "gui/ActivityProgressWidget.h"
#include "AboutBox.h"
#include "logic/proxymodels/ModelChangeWatcher.h"

#include <gui/menus/ActionBundle.h>
#include <gui/menus/HelpMenu.h>
#include <KXmlGui/KEditToolBar>

#include "concurrency/ExtAsync.h"

//
// Note: The MDI portions of this file are very roughly based on the Qt5 MDI example,
// the MDI editor example here: http://www.informit.com/articles/article.aspx?p=1405543&seqNum=6, and counless
// other variations on the theme, with my own adaptations liberally applied throughout.
//

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags) : BASE_CLASS(parent, flags)
{
	// Name our MainWindow.
	setObjectName("MainWindow");

    // Name our GUI thread.
    QThread::currentThread()->setObjectName("GUIThread");
    qDebug() << "Current thread:" << QThread::currentThread()->objectName();

	init();
}

MainWindow::~MainWindow()
{

}

void MainWindow::init()
{
	// Get some standard paths.
	// App-specific cache directory.
	m_cachedir = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
	qInfo() << "App cache dir:" << m_cachedir;
	// App-specific directory where persistent application data can be stored.  On Windows, this is the roaming, not local, path.
	m_appdatadir = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
	qInfo() << "App data dir:" << m_appdatadir;

	readPreGUISettings();

	// Follow the system style for the Icon&/|Text setting for toolbar buttons.
	setToolButtonStyle(Qt::ToolButtonFollowStyle);

//	// KDE
//	setStandardToolBarMenuEnabled(true);


	// Set up our Theme/Style management and actions.
	Theme::initialize();
	m_actgroup_styles = Theme::getStylesActionGroup(this);
	m_act_styles_kaction_menu = qobject_cast<KActionMenu*>(m_actgroup_styles->parent());
	Q_CHECK_PTR(m_act_styles_kaction_menu);

	// doChangeStyle() if we need to.
	if(!AMLMSettings::widgetStyle().isEmpty()
			&& QString::compare(QApplication::style()->objectName(), AMLMSettings::widgetStyle(), Qt::CaseInsensitive) != 0)
	{
		// Initailize the different style.
		doChangeStyle();
	}


	/// Set "document mode" for the tab bar of tabbed dock widgets.
	setDocumentMode(true);
	setAnimated(true);
	setDockNestingEnabled(true);

M_WARNING("TODO: ifdef this to development only")
	m_experimental = new Experimental(this);

	// The list of LibraryModels.
	m_libmodels.clear();

	// The list of PlaylistModels.
	m_playlist_models.clear();

	// The media player.
	m_player = new MP2(this);

	m_controls = new PlayerControls(this);

	// Create the MDIArea and set it as the central widget.
	m_mdi_area = new MDIArea(this);
	setCentralWidget(m_mdi_area);

	// Connect the MDIArea subWindowActivated signal to a slot so we know when
	// the subwindow activation changes.
	connect(m_mdi_area, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);

	createActions();
	createMenus();
	createToolBars();
	createStatusBar();
	createDockWidgets();
	updateActionEnableStates();

	////// Connect up signals and slots.
	createConnections();

	setWindowTitle(qApp->applicationDisplayName());

	setUnifiedTitleAndToolBarOnMac(true);

	// KF5: Activate Autosave of toolbar/menubar/statusbar/window layout settings.
	// "Make sure you call this after all your *bars have been created."
	setAutoSaveSettings();

	// Send ourself a message to re-load the files we had open last time we were closed.
	QTimer::singleShot(0, this, &MainWindow::onStartup);
}

MainWindow* MainWindow::getInstance()
{
	// Search the qApp for the single MainWindow instance.
	for(auto widget : qApp->topLevelWidgets())
	{
		if(MainWindow* is_main_window = qobject_cast<MainWindow*>(widget))
		{
			// Found it.
			return is_main_window;
		}
	}

	Q_ASSERT_X(0, "getInstance", "Couldn't find a MainWindow instance");
	return nullptr;
}

/**
 * Called primarily when we get a subWindowActivated signal from the MDIArea.
 */
void MainWindow::updateActionEnableStates()
{
	// Do we have an active MDI child, and what is it?
	qDebug() << "childIsBaseClass:" << activeChildMDIView();

	auto childIsBaseClass = qobject_cast<MDITreeViewBase*>(activeChildMDIView());
	auto childIsPlaylist = qobject_cast<MDIPlaylistView*>(activeChildMDIView());
	auto childIsLibrary = qobject_cast<MDILibraryView*>(activeChildMDIView());

	qDebug() << "childIsBaseClass:" << childIsBaseClass;

	// Update file actions.
	m_saveLibraryAsAct->setEnabled(childIsLibrary);
	m_savePlaylistAct->setEnabled(childIsPlaylist);

	// Update the Window menu actions.
	m_act_close->setEnabled(childIsBaseClass);
	m_act_close_all->setEnabled(childIsBaseClass);
	m_windowTileAct->setEnabled(childIsBaseClass);
	m_windowCascadeAct->setEnabled(childIsBaseClass);
	m_act_window_list_separator->setVisible(childIsBaseClass);
	if(childIsBaseClass)
	{
		// Set the check next to this window's menu entry.
		childIsBaseClass->windowMenuAction()->setChecked(true);
	}

	updateActionEnableStates_Edit();
}

void MainWindow::updateActionEnableStates_Edit()
{
	if(activeChildMDIView())
	{
		qDebug() << "Active child:" << activeChildMDIView();

		// We have an active MDI child.  What is it?
		auto childIsPlaylist = qobject_cast<MDIPlaylistView*>(activeChildMDIView());
		auto childIsLibrary = qobject_cast<MDILibraryView*>(activeChildMDIView());
		auto childBaseClass = qobject_cast<MDITreeViewBase*>(activeChildMDIView());

		if(childBaseClass)
		{
			// Update edit actions.
			qDebug() << "Child inherits from MDITreeViewBase, updating edit actions enable state";

			// It's something that might have a selection.
			auto bcsm = childBaseClass->selectionModel();
			bool has_selection = bcsm && bcsm->hasSelection();
			// And may or may not contain items.
			auto bcmodel = childBaseClass->model();
			bool has_items = bcmodel && bcmodel->rowCount() > 0;
			// For paste, does the clipboard have anything in it we might be interested in?
			const QClipboard *clipboard = QApplication::clipboard();
			const QMimeData *mimeData = clipboard->mimeData();
			bool clipboard_has_contents = false;
			if(mimeData)
			{
				QStringList mimedata_formats = mimeData->formats();
				if(mimedata_formats.contains(g_additional_supported_mimetypes[0]))
				{
					clipboard_has_contents = true;
				}
			}


			// Can copy from any derived class if it has a selection.
			m_act_copy->setEnabled(has_selection);

			// We can only select all if it has some items.
			m_act_select_all->setEnabled(has_items);

			// A playlist can also cut and delete.
			auto mutating_actions = {m_act_cut, m_act_delete};
			for(auto act : mutating_actions)
			{
				act->setEnabled(childIsPlaylist && has_selection);
			}

			// We can paste into a Playlist regardless of selection.
			m_act_paste->setEnabled(childIsPlaylist && clipboard_has_contents);

			return;
		}
	}

	// No active MDI child, or not one that could have a selection or be pasted into.
	for(auto i : {m_act_copy, m_act_cut, m_act_paste, m_act_delete, m_act_select_all})
	{
		i->setDisabled(true);
	}
}

void MainWindow::createActions()
{
	// Get the actionCollection() once here and pass the pointer to the sub-CreateAction*()'s.
	KActionCollection *ac = actionCollection();

	//
	// File actions.
	//
    ////// Library actions.
    m_importLibAct = make_action(QIcon::fromTheme("folder-open"), "&Import library...", this,
                                QKeySequence("CTRL+SHIFT+O"),
                                "Add a library location");
	connect_trig(m_importLibAct, this, &MainWindow::importLib);

	m_saveLibraryAsAct = make_action(QIcon::fromTheme("folder-close"), "&Save library as...", this);

	m_removeDirFromLibrary = make_action(QIcon::fromTheme("edit-delete"), "Remove &Dir from library...", this);

    ////// Playlist actions.
	m_newPlaylistAct = make_action(QIcon::fromTheme("document-new"), "&New playlist...", this,
                                  QKeySequence::New,
                                  "Create a new playlist");
	connect_trig(m_newPlaylistAct, this, &MainWindow::newPlaylist);

	m_openPlaylistAct = make_action(QIcon::fromTheme("document-open"), "&Open playlist...", this,
                                QKeySequence::Open,
                                "Open an existing playlist");

	m_savePlaylistAct = make_action(QIcon::fromTheme("document-save"), "&Save playlist as...", this,
                                   QKeySequence::Save);

	connect_trig(m_savePlaylistAct, this, &MainWindow::savePlaylistAs);

	m_exitAction = make_action(QIcon::fromTheme("application-exit"), "E&xit", this,
                              QKeySequence::Quit,
                              "Exit application");
	connect_trig(m_exitAction, this, &MainWindow::close);

	//
	// Edit actions.
	//
	createActionsEdit();

	//
	// View actions.
	//
	createActionsView();

	createActionsTools();

	createActionsSettings(ac);

	//
    // Window actions.
	//
	m_tabs_or_subwindows_group = new QActionGroup(this);
	m_tabs_act = make_action(QIcon::fromTheme("tab-duplicate"), "Tabs", m_tabs_or_subwindows_group,
							 QKeySequence(), "Display as tabs");
	m_tabs_act->setCheckable(true);

	m_subwins_act = make_action(QIcon::fromTheme("window-duplicate"), "Subwindows", m_tabs_or_subwindows_group,
								QKeySequence(), "Display as subwindows");
	m_subwins_act->setCheckable(true);
	m_tabs_act->setChecked(true);
	connect(m_tabs_or_subwindows_group, &QActionGroup::triggered, this, &MainWindow::onChangeWindowMode);

	m_windowNextAct = make_action(QIcon::fromTheme("go-next"), "&Next", this,
                                 QKeySequence::NextChild);
	connect_trig(m_windowNextAct, this->m_mdi_area, &QMdiArea::activateNextSubWindow);

	m_windowPrevAct = make_action(QIcon::fromTheme("go-previous"), "&Previous", this,
                            QKeySequence::PreviousChild);
	connect_trig(m_windowPrevAct, this->m_mdi_area, &QMdiArea::activatePreviousSubWindow);


	m_windowCascadeAct = make_action(QIcon::fromTheme("window-cascade"), "Cascade", this);
	connect_trig(m_windowCascadeAct, this->m_mdi_area, &QMdiArea::cascadeSubWindows);

	m_windowTileAct = make_action(QIcon::fromTheme("window-tile"), "Tile", this);
	connect_trig(m_windowTileAct, this->m_mdi_area, &QMdiArea::tileSubWindows);

	m_act_close = make_action(QIcon::fromTheme("window-close"), "Cl&ose", this,
                            QKeySequence::Close,
                            "Close the active window");
//	connect_trig(m_act_close, this->m_mdi_area, &QMdiArea::closeActiveSubWindow);
	connect_trig(m_act_close, this, &MainWindow::onCloseSubwindow);

	m_act_close_all = make_action(QIcon::fromTheme("window-close-all"), "Close &All", this,
                              QKeySequence(),
                               "Close all the windows");
	connect_trig(m_act_close_all, this->m_mdi_area, &QMdiArea::closeAllSubWindows);

	m_act_window_list_separator = new QAction(this);
	m_act_window_list_separator->setText(tr("Window List"));
	m_act_window_list_separator->setSeparator(true);

	m_act_group_window = new QActionGroup(this);

    // Help actions.
	createActionsHelp(ac);

	/// Experimental actions
	m_experimentalAct = make_action(QIcon::fromTheme("edit-bomb"), "Experimental", this,
								   QKeySequence(), "Invoke experimental code - USE AT YOUR OWN RISK");
	connect_trig(m_experimentalAct, this, &MainWindow::doExperiment);

//M_WARNING("TODO: Experimental KDE")
	// Provides a menu entry that allows showing/hiding the toolbar(s)
//	setStandardToolBarMenuEnabled(true);

	// Provides a menu entry that allows showing/hiding the statusbar
//	createStandardStatusBarAction();

	// Standard 'Configure' menu actions
//	createSettingsActions();
/// @end
}

void MainWindow::createActionsEdit()
{
	// The cut/copy/paste action "sub-bundle".
	m_ab_cut_copy_paste_actions = new ActionBundle(this);

	// Specifying the ActionBundle as each QAction's parent automatically adds it to the bundle.
	m_act_cut = make_action(Theme::iconFromTheme("edit-cut"), tr("Cu&t"), m_ab_cut_copy_paste_actions, QKeySequence::Cut,
                                                    tr("Cut the current selection to the clipboard"));
	connect_trig(m_act_cut, this, &MainWindow::onCut);

	m_act_copy = make_action(Theme::iconFromTheme("edit-copy"), tr("&Copy"), m_ab_cut_copy_paste_actions, QKeySequence::Copy,
                                                     tr("Copy the current selection to the clipboard"));
    connect_trig(m_act_copy, this, &MainWindow::onCopy);

	m_act_paste = make_action(Theme::iconFromTheme("edit-paste"), tr("&Paste"), m_ab_cut_copy_paste_actions, QKeySequence::Paste,
                                                      tr("Paste the clipboard's contents into the current selection"));
	connect_trig(m_act_paste, this, &MainWindow::onPaste);

	// The action bundle containing the other edit actions.
	m_ab_extended_edit_actions = new ActionBundle(this);

	m_ab_extended_edit_actions->addSection(tr("Delete"));

	m_act_delete = make_action(Theme::iconFromTheme("edit-delete"), tr("&Delete"), m_ab_extended_edit_actions, QKeySequence::Delete,
                                                       tr("Delete this entry"));
    connect_trig(m_act_delete, this, &MainWindow::onDelete);

	m_ab_extended_edit_actions->addSection(tr("Selections"));

	m_act_select_all = make_action(Theme::iconFromTheme("edit-select-all"), tr("Select &All"), m_ab_extended_edit_actions,
                                                               QKeySequence::SelectAll, tr("Select all items in the current list"));
	connect_trig(m_act_select_all, this, &MainWindow::onSelectAll);
}

void MainWindow::createActionsView()
{
	m_ab_docks = new ActionBundle(this);

	m_act_lock_layout = make_action(Theme::iconFromTheme("emblem-locked"), tr("Lock layout"), this); // There's also an "emblem-unlocked"
	m_act_reset_layout = make_action(Theme::iconFromTheme("view-multiple-objects"), tr("Reset layout"), this);
	/// @todo These appear to be unreparentable, so we can't give them to an ActionBundle.
//	m_ab_docks->addAction(m_libraryDockWidget->toggleViewAction());
//	m_ab_docks->addAction(m_metadataDockWidget->toggleViewAction());

	m_act_ktog_show_tool_bar = new KToggleToolBarAction("FileToolbar", tr("Show File Toolbar"), actionCollection());
}

void MainWindow::createActionsTools()
{
	//
	// Tools actions.
	//

	m_rescanLibraryAct = make_action(QIcon::fromTheme("view-refresh"), tr("&Rescan libray..."), this,
									QKeySequence::Refresh);
	connect_trig(m_rescanLibraryAct, this, &MainWindow::onRescanLibrary);

	m_cancelRescanAct = make_action(Theme::iconFromTheme("process-stop"), tr("Cancel Rescan"), this,
									QKeySequence::Cancel);
	connect_trig(m_cancelRescanAct, this, &MainWindow::onCancelRescan);

	m_scanLibraryAction = make_action(QIcon::fromTheme("tools-check-spelling"), "Scan library", this,
							   QKeySequence(), "Scan library for problems");
}

void MainWindow::createActionsSettings(KActionCollection *ac)
{
#if HAVE_KF5

	// Styles KActionMenu menu.
	addAction(QStringLiteral("styles_menu"), m_act_styles_kaction_menu);
	connect(m_actgroup_styles, &QActionGroup::triggered, this, &MainWindow::onChangeStyle);

	// Show/hide menu bar.
	m_act_ktog_show_menu_bar = KStandardAction::showMenubar(this, &MainWindow::onShowMenuBar, ac);

	// Open the shortcut configuration dialog.
	m_act_shortcuts_dialog = KStandardAction::keyBindings(this, &MainWindow::onOpenShortcutDlg, ac);

	// Open the application preferences dialog.
	m_settingsAct = KStandardAction::preferences(this, &MainWindow::startSettingsDialog, ac);

	m_act_config_toolbars = KStandardAction::configureToolbars(this, &MainWindow::onConfigureToolbars, ac);

#else
	m_act_shortcuts_dialog = make_action(Theme::iconFromTheme(""),
									 tr("Edit Shortcuts..."), this);

	connect_trig(m_act_shortcuts_dialog, this, &MainWindow::onOpenShortcutDlg);

	m_settingsAct = make_action(QIcon::fromTheme("configure"), tr("Settings..."), this,
							   QKeySequence::Preferences, "Open the Settings dialog.");
	connect_trig(m_settingsAct, this, &MainWindow::startSettingsDialog);
#endif
}

void MainWindow::createActionsHelp(KActionCollection* ac)
{
#if HAVE_KF5
	// For KDE we use a derivation of KHelpMenu.
#else
	m_helpAct = make_action(Theme::iconFromTheme("help-contents"), tr("&Help"), this,
							QKeySequence::HelpContents,
							"Show help contents");
	m_helpAct->setDisabled(true); /// @todo No help yet.
	// Qt5 has a pre-fabbed "What's This" action which handles everything, we don't need to even add a handler or an icon.
	m_whatsThisAct = QWhatsThis::createAction(this);
	m_whatsThisAct->setStatusTip("Show more than a tooltip, less than full help on a GUI item");

	m_aboutAct = make_action(QIcon::fromTheme("help-about"), tr("&About"), this,
						   QKeySequence(),
							"Show the About box");
	connect_trig(m_aboutAct, this, &MainWindow::about);

	m_aboutQtAct = make_action(QIcon::fromTheme("help-about-qt"), tr("About &Qt"), this,
							 QKeySequence(),
							  "Show the Qt library's About box");
	connect_trig(m_aboutQtAct, qApp, &QApplication::aboutQt);
#endif
}

void MainWindow::addViewMenuActions(QMenu* menu)
{
M_WARNING("TODO");
	menu->setTitle(tr("&View"));

	m_act_lock_layout->setChecked(AMLMSettings::layoutIsLocked());
//	connect(m_act_lock_layout, &QAction::toggled, this, &MainWindow::setLayoutLocked);
	addAction("layout_locked", m_act_lock_layout);

	menu->addSeparator();

	// List doc widgets.

	// List toolbars.

}

void MainWindow::createMenus()
{
	m_fileMenu = menuBar()->addMenu(tr("&File"));

	m_fileMenu->addActions({//newFileAct,
						  m_fileMenu->addSection("Libraries"),
						  m_importLibAct,
						  m_saveLibraryAsAct,
						  m_fileMenu->addSection("Playlists"),
						  m_newPlaylistAct,
						  m_openPlaylistAct,
						  m_savePlaylistAct,
                          //saveAsAct,
						  m_fileMenu->addSeparator(),
                          //closeAct,
                          //closeAllAct,
                          //fileMenu.addSeparator(),
                          //printAct,
                          //actPrintPreview,
                          //fileMenu.addSeparator(),
						  m_exitAction});

	// Edit menu.
	m_menu_edit = menuBar()->addMenu(tr("&Edit"));
	// Cut/copy/paste
	m_ab_cut_copy_paste_actions->appendToMenu(m_menu_edit);
	// Delete/Select all.
	m_ab_extended_edit_actions->appendToMenu(m_menu_edit);
	// Let's see what this does, just for fun.
	m_menu_edit->setTearOffEnabled(true);

    // Create the View menu.
	m_viewMenu = menuBar()->addMenu(tr("&View"));
	m_ab_docks->appendToMenu(m_viewMenu);
	m_viewMenu->addActions({
							   m_act_lock_layout,
							   m_act_reset_layout,
							   m_act_ktog_show_tool_bar,
						   });

    // Tools menu.
	m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
	m_toolsMenu->addActions(
        {//scanLibraryAction,
		 m_toolsMenu->addSection("Rescans"),
		 m_rescanLibraryAct,
		 m_cancelRescanAct,
                });

	// Settings menu.
	m_menu_settings = menuBar()->addMenu(tr("&Settings"));
	m_menu_settings->addActions({
		m_act_ktog_show_menu_bar,
		m_toolsMenu->addSeparator(),
		m_act_styles_kaction_menu,
		m_toolsMenu->addSeparator(),
		m_settingsAct,
		m_act_shortcuts_dialog,
		m_act_config_toolbars
		});

    // Create the Window menu.
	m_menu_window = menuBar()->addMenu(tr("&Window"));
	m_menu_window->addActions({
		m_menu_window->addSection(tr("Close")),
		m_act_close,
		m_act_close_all,
		m_menu_window->addSection(tr("Arrange")),
		m_windowTileAct,
		m_windowCascadeAct,
		m_menu_window->addSection(tr("Subwindow Mode")),
		m_tabs_act,
		m_subwins_act,
		m_menu_window->addSection(tr("Navigation")),
		m_windowNextAct,
		m_windowPrevAct,
		m_act_window_list_separator
    });

    menuBar()->addSeparator();

	// Create the non-KDE help menu.
#ifndef HAVE_KF5
	m_helpMenu = menuBar()->addMenu("&Help");
	m_helpMenu->addSection("Help");
	m_helpMenu->addAction(m_helpAct);
	m_helpMenu->addAction(m_whatsThisAct);
	m_helpMenu->addSection("About");
	m_helpMenu->addAction(m_aboutAct);
	m_helpMenu->addAction(m_aboutQtAct);
#else
	// Create a help menu based on KF5 KHelpMenu.
	auto help_menu = new HelpMenu(this, KAboutData::applicationData());
	menuBar()->addMenu(help_menu->menu());
#endif // HAVE_KF5
}


void MainWindow::createToolBars()
{
#ifdef HAVE_KF5
	#define addToolBar(str) this->toolBar((str))
#else
	#define addToolBar(str) addToolBar((str))
#endif

	//
	// File
	//
	m_fileToolBar = addToolBar(tr("FileToolbar"));
	m_fileToolBar->setObjectName("FileToolbar");
	m_fileToolBar->addActions({m_importLibAct,
	                           m_rescanLibraryAct,
							   m_cancelRescanAct,
							 m_fileToolBar->addSeparator(),
							 m_newPlaylistAct,
							 m_openPlaylistAct,
							 m_savePlaylistAct});

	//
	// Edit
	//
	m_toolbar_edit = addToolBar(tr("Edit"));
	m_toolbar_edit->setObjectName("EditToolbar");
	// Only add the cut/copy/paste subset of actions to the toolbar.
	m_ab_cut_copy_paste_actions->appendToToolBar(m_toolbar_edit);

	//
	// Settings
	//
	m_settingsToolBar = addToolBar("Settings");
	m_settingsToolBar->setObjectName("SettingsToolbar");
	m_settingsToolBar->addAction(m_settingsAct);
	m_settingsToolBar->addAction(m_experimentalAct);

#ifndef HAVE_KF5
    // Create a combo box where the user can change the style.
	QComboBox* styleComboBox = new QComboBox;
	styleComboBox->addItems(QStyleFactory::keys());
    // Set it to the current style.
	QString cur_style = qApp->style()->objectName();
	styleComboBox->setCurrentIndex(styleComboBox->findText(cur_style, Qt::MatchFixedString));
	m_settingsToolBar->addWidget(styleComboBox);
	connect(styleComboBox, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::activated), this, &MainWindow::changeStyle);

    // Create a combo box with icon themes.
	QComboBox* iconComboBox = new QComboBox;
    iconComboBox->addItems(Theme::GetIconThemeNames());
	m_settingsToolBar->addWidget(iconComboBox);
	connect(iconComboBox, static_cast<void(QComboBox::*)(const QString&)>(&QComboBox::activated), this, &MainWindow::changeIconTheme);
#endif

    // Create another toolbar for the player controls.
	m_controlsToolbar = addToolBar("Player Controls");
	m_controlsToolbar->setObjectName("PlayerControlsToolbar");
	m_controlsToolbar->addWidget(m_controls);

	// Create a toolbar for filtering
	m_filterToolbar = addToolBar("Filter");
	m_filterToolbar->setObjectName("FilterToolbar");
	FilterWidget* fw = new FilterWidget;
	auto filterPatternLabel = new QLabel(tr("&Filter pattern:"));
	m_filterToolbar->addWidget(filterPatternLabel);
	m_filterToolbar->addWidget(fw);
	filterPatternLabel->setBuddy(fw);
}

void MainWindow::createStatusBar()
{
	m_activity_progress_widget = new ActivityProgressWidget(this);
	statusBar()->addPermanentWidget(m_activity_progress_widget);
	statusBar()->showMessage("Ready");
}

void MainWindow::createDockWidgets()
{
    // Create the Library/Playlist dock widget.
	m_collection_dock_widget = new CollectionDockWidget(tr("Media Sources"), this);
	addDockWidget(Qt::LeftDockWidgetArea, m_collection_dock_widget);

    // Create the metadata dock widget.
	m_metadataDockWidget = new MetadataDockWidget(tr("Metadata Explorer"), this);
	m_metadataDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, m_metadataDockWidget);
}

void MainWindow::initRootModels()
{
	// The Model of Model/View pairs.
	m_model_of_model_view_pairs = new QStandardItemModel(this);
	m_model_of_model_view_pairs->setObjectName("ModelOfModels");

	m_stditem_libraries = new QStandardItem(tr("Libraries"));
	m_stditem_libraries->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
	auto font = QFont(m_stditem_libraries->font());
	font.setBold(true);
	m_stditem_libraries->setFont(font);
	m_stditem_playlist_views = new QStandardItem(tr("Playlists"));
	m_stditem_playlist_views->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
	m_stditem_playlist_views->setFont(font);

	// Add top-level "category" items.
	m_model_of_model_view_pairs->/*invisibleRootItem()->*/appendRow(m_stditem_libraries);
	m_model_of_model_view_pairs->/*invisibleRootItem()->*/appendRow(m_stditem_playlist_views);
//	Q_ASSERT(m_stditem_libraries->parent() == m_model_of_model_view_pairs->invisibleRootItem());
//	Q_ASSERT(m_stditem_playlists->parent() == m_model_of_model_view_pairs->invisibleRootItem());

//	m_stditem_playlists->setChild(0,0, new QStandardItem("Hello"));

	// Set the Collection Doc widget model.
	m_collection_dock_widget->setModel(m_model_of_model_view_pairs);
}

void MainWindow::createConnections()
{
	/// @todo
	connect(qApp, &QApplication::focusChanged, this, &MainWindow::onFocusChanged);

    // Connect player controls up to player.
	connectPlayerAndControls(m_player, m_controls);

    // Connect with the CollectionDockWidget.
	connect(m_collection_dock_widget, &CollectionDockWidget::showLibraryModelSignal, this, &MainWindow::onShowLibrary);
	connect(m_collection_dock_widget, &CollectionDockWidget::removeLibModelFromLibSignal, this, &MainWindow::onRemoveDirFromLibrary);
	connect(m_collection_dock_widget, &CollectionDockWidget::activateSubwindow, this, &MainWindow::setActiveSubWindow);

	// Connect FilterWidget signals
	connect(m_filterToolbar->findChild<FilterWidget*>(), &FilterWidget::filterChanged, this, &MainWindow::onTextFilterChanged);

	updateConnections();
}

void MainWindow::connectPlayerAndControls(MP2 *player, PlayerControls *controls)
{
	// PlayerControls -> MP2 signals.
	connect(controls, &PlayerControls::play, player, &MP2::play);
	connect(controls, &PlayerControls::pause, player, &MP2::pause);
	connect(controls, &PlayerControls::stop, player, &MP2::stop);
	connect(controls, &PlayerControls::changeRepeat, player, &MP2::repeat);
	connect(controls, &PlayerControls::changeMuting, player, &MP2::setMuted);
	connect(controls, &PlayerControls::changeVolume, player, &MP2::setVolume);
	connect(controls, &PlayerControls::changeShuffle, player, &MP2::setShuffleMode);

	// MP2 -> PlayerControls signals.
	connect(player, &MP2::stateChanged, controls, &PlayerControls::setState);
	connect(player, &MP2::mutedChanged, controls, &PlayerControls::setMuted);
	connect(player, &MP2::volumeChanged, controls, &PlayerControls::setVolume);
	connect(player, &MP2::durationChanged2, controls, &PlayerControls::onDurationChanged);
	connect(player, &MP2::positionChanged2, controls, &PlayerControls::onPositionChanged);

	// Final setup.
	// Set volume control to the current player volume.
	controls->setVolume(player->volume());
}

/**
 * @note This actually connects the player to the playlist's QMediaPlaylist.  Should probably encapsulate this better.
 */
void MainWindow::connectPlayerAndPlaylistView(MP2 *player, MDIPlaylistView *playlist_view)
{
	/// @todo Hide qMediaPlaylist behind playlist_view?
	if(player->playlist() == playlist_view->getQMediaPlaylist())
	{
		qDebug() << "Already connected.";
	}
	else
	{
		QMediaPlaylist* qmp = playlist_view->getQMediaPlaylist();
		player->setPlaylist(qmp);
	}
}

void MainWindow::connectPlayerControlsAndPlaylistView(PlayerControls *m_controls, MDIPlaylistView *playlist_view)
{
	/// @note Qt::ConnectionType() cast here is due to the mixed flag/enum nature of the type.  Qt::UniqueConnection (0x80) can be bitwise-
	/// OR-ed in with any other connection type, which are 0,1,2,3.
	connect(m_controls, &PlayerControls::next, playlist_view, &MDIPlaylistView::next, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
	connect(m_controls, &PlayerControls::previous, playlist_view, &MDIPlaylistView::previous, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));

	// Connect play() signal-to-signal.
	connect(playlist_view, &MDIPlaylistView::play, m_controls, &PlayerControls::play, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
}

void MainWindow::connectLibraryModelToActivityProgressWidget(LibraryModel* lm, ActivityProgressWidget* apw)
{
	lm->connectProgressToActivityProgressWidget(apw);
}

void MainWindow::connectLibraryViewAndMainWindow(MDILibraryView *lv)
{
    qDebug() << "Connecting" << lv << "and" << this;
	connect(lv, &MDILibraryView::sendEntryToPlaylist, this, &MainWindow::onSendEntryToPlaylist);
	connect(lv, &MDILibraryView::sendToNowPlaying, this, &MainWindow::onSendToNowPlaying);

	connect(this, &MainWindow::settingsChanged, lv, &MDILibraryView::onSettingsChanged);
}

void MainWindow::connectPlaylistViewAndMainWindow(MDIPlaylistView* plv)
{
	qDebug() << "Connecting";
M_WARNING("TODO: connect");
	qDebug() << "Connected";
}

void MainWindow::connectNowPlayingViewAndMainWindow(MDINowPlayingView* now_playing_view)
{
    qDebug() << "Connecting";
	connect(this, &MainWindow::sendToNowPlaying, now_playing_view, &MDINowPlayingView::onSendToNowPlaying);
	connect(this, &MainWindow::settingsChanged, now_playing_view, &MDILibraryView::onSettingsChanged);


	connectPlayerAndPlaylistView(m_player, now_playing_view);
	connectPlayerControlsAndPlaylistView(m_controls, now_playing_view);
    qDebug() << "Connected";
}

void MainWindow::connectActiveMDITreeViewBaseAndMetadataDock(MDITreeViewBase* viewbase, MetadataDockWidget* metadata_dock_widget)
{
	metadata_dock_widget->connectToView(viewbase);
}


void MainWindow::updateConnections()
{
	qDebug() << "Updating connections";
	auto childIsMDITreeViewBase = qobject_cast<MDITreeViewBase*>(activeChildMDIView());
	auto childIsPlaylist = qobject_cast<MDIPlaylistView*>(activeChildMDIView());
	auto childIsLibrary = qobject_cast<MDILibraryView*>(activeChildMDIView());

    if(childIsMDITreeViewBase)
    {
//		qDebug() << "Updating connectons for activated window" << activeMdiChild()->windowTitle();

		// Connect the Metadata dock widget to the active child window's selectionModel().
		connectActiveMDITreeViewBaseAndMetadataDock(childIsMDITreeViewBase, m_metadataDockWidget);
    }
}

//////
////// MDI Management functions.
//////

bool MainWindow::maybeSaveOnClose()
{
	QStringList failures;
	for(auto child : m_mdi_area->subWindowList())
	{
		MDIPlaylistView* playlist_ptr= qobject_cast<MDIPlaylistView*>(child->widget());
		if(playlist_ptr == nullptr)
		{
			continue;
		}
		else if(playlist_ptr->isWindowModified())
		{
			bool retval = playlist_ptr->save();
			if(retval == false)
			{
				/// @todo Record the actual error.
				failures.append(playlist_ptr->getCurrentUrl().toString());
			}
		}
	}
	if(failures.size() > 0)
	{
		if(QMessageBox::warning(this, tr("Save Error"),
								tr("Failed to save %1\nQuit anyway?").arg(failures.join(',')), /// @todo % "\n\t".join(failures),
		QMessageBox::Yes|QMessageBox::No) == QMessageBox::No)
		{
			// Do not accept the close event.
			return false;
		}
	}
	return true;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	/// @note Per KF5 docs:
	/// https://api.kde.org/frameworks/kxmlgui/html/classKMainWindow.html#a2a4a27013543027fd70c707041068777
	/// "We recommend that you reimplement queryClose() rather than closeEvent(). If you do it anyway,
	/// ensure to call the base implementation to keep the feature of auto-saving window settings working."
	qDebug() << QString("Main Window received closeEvent.");

	KXmlGuiWindow::closeEvent(event);

	stopAllBackgroundThreads();

	// Save auto-save settings if settings have changed.
	if(settingsDirty() && autoSaveSettings())
	{
		saveAutoSaveSettings();
	}

	bool continue_with_close = maybeSaveOnClose();
	if(continue_with_close)
	{
		qDebug() << QString("Writing settings.");
		writeSettings();
		m_mdi_area->closeAllSubWindows();
		qDebug() << QString("Accepting close event.");
		event->accept();
	}
	else
	{
		qDebug() << QString("ignoring closeEvent.");
		event->ignore();
	}
}

/**
 * Note: Returns the QMdiSubWindow's widget(), not the QMdiSubWindow subwindow itself.
 */
MDITreeViewBase* MainWindow::activeChildMDIView()
{
	auto activeSubWindow = m_mdi_area->activeSubWindow();

    if(activeSubWindow)
    {
        return qobject_cast<MDITreeViewBase*>(activeSubWindow->widget());
    }
    return nullptr;
}

/**
 * Find any existing child MDI window associated with the given url.
 */
QMdiSubWindow* MainWindow::findSubWindow(QUrl url) const
{
	for(auto window : m_mdi_area->subWindowList())
	{
		auto widget = window->widget();
        qDebug() << "windowFilePath:" << widget << widget->windowFilePath();
		if(widget == nullptr)
		{
			qCritical() << "Mdi child has no widget.";
		}
		else if(widget->windowFilePath() == url.toString())
		{
            qDebug() << "Found subwindow with URL:" << url;
			return window;
		}
	}
    return nullptr;
}

MDITreeViewBase* MainWindow::findSubWindowView(QUrl url) const
{
    auto child_mdi_subwin = findSubWindow(url);

    if(child_mdi_subwin)
    {
        // Found a child window associated with the given URL.
        // Return the QMdiSubWindow's widget.
        auto view = qobject_cast<MDITreeViewBase*>(child_mdi_subwin->widget());
        if(!view)
        {
            qCritical() << "Found subwindow" << child_mdi_subwin
                        << "has no MDITreeViewBase-derived view as widget().  widget():" << child_mdi_subwin->widget();
            qFatal("findSubwindow() mechanism failed.");
            return nullptr; // Never returns, this is just to eliminate compiler warning.
        }
        else
        {
            return view;
        }
    }
    else
    {
        // No such child window found.
        return nullptr;
    }
}

MDIModelViewPair MainWindow::findSubWindowModelViewPair(QUrl url) const
{
    MDIModelViewPair retval;
    auto view = findSubWindowView(url);
    if(view)
    {
		// Found an existing View, which means it's attached to an existing Model.
        retval.m_view = view;
        retval.m_view_was_existing = true;
		retval.m_model = view->underlyingModel();
        retval.m_model_was_existing = true;
    }
	else
	{
		// No existing View, is there an existing Model open?
		/// @todo: Seems like there should be a cleaner way to handle this.
		for(auto lm : m_libmodels)
		{
			if(lm->getLibRootDir() == url)
			{
				qDebug() << "Found existing LibraryModel:" << lm;
				retval.m_model = lm;
				retval.m_model_was_existing = true;
			}
		}
		for(auto pm : m_playlist_models)
		{
			if(pm->getLibRootDir() == url)
			{
				qDebug() << "Found existing PlaylistModel:" << pm;
				retval.m_model = pm;
				retval.m_model_was_existing = true;
			}
		}
	}

	return retval;
}

void MainWindow::onFocusChanged(QWidget* old, QWidget* now)
{
//	qDebug() << "Keyboard focus has changed from" << old << "to" << now;
}

void MainWindow::view_is_closing(MDITreeViewBase* viewptr, QAbstractItemModel* modelptr)
{
//	qDebug() << "A child view is closing:" << viewptr << modelptr;

	auto playlist = qobject_cast<MDIPlaylistView*>(viewptr);
	auto nowplaying = qobject_cast<MDINowPlayingView*>(viewptr);
	if(nowplaying)
	{
		qDebug() << "Tried to close nowplaying view, ignoring:" << nowplaying;
	}
	else if(playlist)
	{
		qDebug() << "Closing playlist, deleting view from model-of-models";

		auto parentindex = m_model_of_model_view_pairs->indexFromItem(m_stditem_playlist_views);
		auto indexes_to_delete = m_model_of_model_view_pairs->match(parentindex, Qt::UserRole+1,
														QVariant::fromValue<MDITreeViewBase*>(viewptr), -1,
														Qt::MatchExactly | Qt::MatchRecursive);
		qDebug() << "Num indexes found:" << indexes_to_delete.size();

		for(auto i : indexes_to_delete)
		{
			m_model_of_model_view_pairs->removeRow(i.row(), parentindex);
		}

		//m_model_of_model_view_pairs->indexFromItem()

		m_playlist_models.erase(std::remove(m_playlist_models.begin(),
								  m_playlist_models.end(),
								  modelptr),
				   m_playlist_models.end());
	}
}

void MainWindow::addAction(const QString& action_name, QAction* action)
{
	actionCollection()->addAction(action_name, action);
	actionCollection()->setDefaultShortcut(action, action->shortcut());
}

//////
////// Top-level QSettings save/restore.
//////

void MainWindow::readPreGUISettings()
{
	// Open the config file.
	// No name == application name + "rc".
	// No type == QStandardPaths::GenericConfigLocation
	KConfigGroup config(KSharedConfig::openConfig(), "Settings");
	/// @todo Add any readEntry()'s here.
}

void MainWindow::readLibSettings(QSettings& settings)
{
	int num_libs = settings.beginReadArray("libraries");
	qInfo() << "Reading" << num_libs << "libraries...";
	for(int i = 0; i < num_libs; ++i)
	{
		settings.setArrayIndex(i);

		QByteArray jdoc_str = settings.value("asJson").toByteArray();

//		qDebug() << "jdoc_str=" << jdoc_str;
		QJsonDocument jsondoc = QJsonDocument::fromJson(jdoc_str);
//		qDebug() << "Jsondoc:" << jsondoc.toJson();
//		qDebug() << "Jsondoc is object?:" << jsondoc.isObject();

		QPointer<LibraryModel> libmodel = LibraryModel::constructFromJson(jsondoc.object(), this);

		if(!libmodel)
		{
			QMessageBox::critical(this, qApp->applicationDisplayName(), "Failed to open library",
								  QMessageBox::Ok);
		}
		else
		{
			MDIModelViewPair mvpair;
			mvpair.m_model = libmodel;
			mvpair.m_model_was_existing = false;

			addChildMDIModelViewPair_Library(mvpair);
		}
	}
	settings.endArray();
}

void MainWindow::writeSettings()
{
	qDebug() << "writeSettings() start";
	QSettings settings;
/// @todo REMOVE
//	settings.beginGroup("mainwindow");
//	settings.setValue("geometry", saveGeometry());
//	settings.setValue("window_state", saveState());
//	settings.endGroup();
	// Write the open library settings.
	writeLibSettings(settings);
	qDebug() << "writeSettings() end";
}


void MainWindow::writeLibSettings(QSettings& settings)
{
	qDebug() << "writeLibSettings() start";

	// First it seems we have to remove the array.
	settings.remove("libraries");

	settings.beginWriteArray("libraries");
	qDebug() << "Writing"  << m_libmodels.size() << "libraries";
	for(size_t i = 0; i < m_libmodels.size(); ++i)
	{
//		qDebug() << "Model #:" << i;
		settings.setArrayIndex(i);

		// Serialize the library to a QJsonObject.
		QJsonObject lib_object;
		m_libmodels[i]->writeToJson(lib_object);
		// Create a QJsonDocument from the QJsonObject.
		QJsonDocument jsondoc(lib_object);
//		qDebug() << "LIB AS JSON:" << jsondoc.toJson();
		// Convert the QJSonDocument to a QByteArray.
		QByteArray jdoc_bytes = jsondoc.toJson();

		// Finally write the QJsonDocument to the QSettings array.
		settings.setValue("asJson", jdoc_bytes);
	}
	settings.endArray();
	qDebug() << "writeLibSettings() end";
}

/**
 * Called by a "timer(0)" started in the constructor.
 * Does some final setup work which should be done once the constructor has finished and the event loop has started.
 */
void MainWindow::onStartup()
{
	// Set up the GUI from the ui.rc file embedded in the app's QResource system.
	setupGUI(KXmlGuiWindow::Default, ":/kxmlgui5/AwesomeMediaLibraryManagerui.rc");

	initRootModels();

    // Create the "Now Playing" playlist and view.
    newNowPlaying();

	// Load any files which were opened at the time the last session was closed.
	qInfo() << "Loading libraries open at end of last session...";
	QSettings settings;
	readLibSettings(settings);

	// Open the windows the user had open at the end of last session.
	openWindows();
}

/**
 * Open the windows the user had open at the end of last session.
 * @todo Actually now only opens a window for each libmodel.
 */
void MainWindow::openWindows()
{
	qInfo() << "Opening windows which were open at end of last session...";

	for(auto m : m_libmodels)
	{
		qDebug() << "Opening view on existing model:" << m->getLibraryName() << m->getLibRootDir();

		auto child = MDILibraryView::openModel(m, this);

		/// @todo Should be encapsulated such that what we get back from openModel() is correct.
		child.m_model_was_existing = true;

		if(child.m_view)
        {
			addChildMDIModelViewPair_Library(child);
        }
	}
}


//////
////// Action targets.
//////

/**
 * Top-level menu/toolbar action for creating a new Library view by picking a library root directory.
 * ~= "File->Open...".
 */
void MainWindow::importLib()
{
	auto check_for_existing_view = [this](QUrl url) -> MDIModelViewPair {
		auto mvpair = findSubWindowModelViewPair(url);
		return mvpair;
    };

    auto child = MDILibraryView::open(this, check_for_existing_view);
	if(child.m_view)
    {
		addChildMDIModelViewPair_Library(child);
    }
    else
    {
		qCritical() << "MDILibraryView::open() returned nullptr";
    }
}

void MainWindow::openFileLibrary(const QUrl& filename)
{
	auto check_for_existing_view = [this](QUrl url) -> MDIModelViewPair {
		auto mvpair = findSubWindowModelViewPair(url);
		return mvpair;
	};

	auto child = MDILibraryView::openFile(filename, this, check_for_existing_view);
	if(child.m_view)
	{
		addChildMDIModelViewPair_Library(child);
	}
	else
	{
		qCritical() << "MDILibraryView::open() returned nullptr";
	}
}

void MainWindow::onRescanLibrary()
{
	// Start a rescan on all models.
	for(auto l : m_libmodels)
	{
		l->startRescan();
	}
}

void MainWindow::onCancelRescan()
{
	for(auto l : m_libmodels)
	{
		l->cancelRescan();
	}
}

void MainWindow::onShowLibrary(QPointer<LibraryModel> libmodel)
{
	// We'll just try to open the same URL as the libmodel, and let the "opening an existing view/model"
	// logic do the rest.
	openFileLibrary(libmodel->getLibRootDir());
}


void MainWindow::onRemoveDirFromLibrary(QPointer<LibraryModel> libmodel)
{
	qDebug() << QString("Removing libmodel from library:") << libmodel;

	// Find any open MDI Windows viewing this library.
	auto window = findSubWindow(libmodel->getLibRootDir());
	if(window)
	{
		qDebug() << "Found window:" << window;
		window->close();
	}
	// Find and Delete the model.
	size_t index = 0;
	for(auto m : m_libmodels)
	{
		if(m == libmodel)
		{
			qDebug() << QString("Removing libmodel:") << m << ", have" << m_libmodels.size() << "model(s).";
			m->close(true);
			m_libmodels.erase(m_libmodels.begin() + index);
			// Delete the model.
			m.clear();
			break;
		}
		index++;
	}
	qDebug() << QString("Num models:") << m_libmodels.size();

	// Write the library settings out now.
	QSettings settings;
	writeLibSettings(settings);
	settings.sync();

	/// @todo ???
}

/**
 * Top-level menu/toolbar action for creating a new, empty playlist.
 * ~= "File->New".
 */
void MainWindow::newPlaylist()
{
    // Create the View object.
    auto child = new MDIPlaylistView(this);

    // Tell it to create a new, empty model.
    child->newFile();

	/// @todo Maybe refactor the "newFile()" setup to look more like the static openXxx() functions,
	/// so we always get an MDIModelViewPair here and don't need to hand-roll it.
	MDIModelViewPair mvpair;
	mvpair.m_view = child;
	mvpair.m_view_was_existing = false;
	mvpair.setModel(child->underlyingModel());
	mvpair.m_model_was_existing = false;

	addChildMDIModelViewPair_Playlist(mvpair);

	statusBar()->showMessage(tr("Opened new Playlist '%1'").arg(child->windowTitle()));
}

/**
 * Top-level menu/toolbar action for creating a new, empty "Now Playing" playlist.
 * ~= "File->New", except there is no user action for creating the "Now Playing" view/model.
 */
void MainWindow::newNowPlaying()
{
    auto child = new MDINowPlayingView(this);
    child->newFile();

	// Set this view's model to be the single "Now Playing" model.
	m_now_playing_playlist_model = child->underlyingModel();

    /// @todo Do we really need to keep this as a member pointer?
    m_now_playing_playlist_view = child;

	/// @todo Maybe refactor the "newFile()" setup to look more like the static openXxx() functions,
	/// so we always get an MDIModelViewPair here and don't need to hand-roll it.
	MDIModelViewPair mvpair;
	mvpair.m_view = child;
	mvpair.m_view_was_existing = false;
	mvpair.setModel(m_now_playing_playlist_model);
	mvpair.m_model_was_existing = false;

	addChildMDIModelViewPair_Playlist(mvpair);

	connectNowPlayingViewAndMainWindow(child);

    statusBar()->showMessage(tr("Opened 'Now Playing' Playlist '%1'").arg(child->windowTitle()));
}

/**
 * Top-level menu/toolbar action for opening an existing playlist.
 * ~= "File->Open...".
 */
void MainWindow::openPlaylist()
{
	qCritical() << "Not implemented";
}

void MainWindow::onSendEntryToPlaylist(std::shared_ptr<LibraryEntry> libentry, QPointer<PlaylistModel> playlist_model)
{
	qDebug() << QString("Sending entry to playlist:") << playlist_model;
	if(!playlist_model.isNull())
	{
		auto new_playlist_entry = PlaylistModelItem::createFromLibraryEntry(libentry);
		playlist_model->appendRow(new_playlist_entry);
	}
}

void MainWindow::onSendToNowPlaying(LibraryEntryMimeData *mime_data)
{
	// Resend the entry to the "Now Playing" playlist view.
	qDebug() << "Re-emitting sendToNowPlaying() with mime_data:" << mime_data;
	Q_EMIT sendToNowPlaying(mime_data);
}

/**
 * Add a child view to the MDIArea and hook it up to a few signals/slots.
 */
void MainWindow::addChildMDIView(MDITreeViewBase* child)
{
	// Connect Cut, Copy, Delete, and Select All actions to the availability signals emitted by the child.
	/// @note This works because only the active child will send these signals.
	/// Otherwise we'd need to swap which child was connected to the actions.
	connect(child, &MDITreeViewBase::cutAvailable, m_act_cut, &QAction::setEnabled);
	connect(child, &MDITreeViewBase::cutAvailable, m_act_delete, &QAction::setEnabled);
	connect(child, &MDITreeViewBase::copyAvailable, m_act_copy, &QAction::setEnabled);
	connect(child, &MDITreeViewBase::selectAllAvailable, m_act_select_all, &QAction::setEnabled);

	/// @todo Same thing with undo/redo.
	// child.undoAvailable.connect(editUndoAct.setEnabled)
	// child.redoAvailable.connect(redoAct.setEnabled)

	// Add the child subwindow to the MDI area.
	auto mdisubwindow = m_mdi_area->addSubWindow(child);

    // Add actions from the child to the Window menu and its action group.
	m_menu_window->addAction(child->windowMenuAction());
	m_act_group_window->addAction(child->windowMenuAction());
//M_WARNING("EXPERIMENTAL")
//	m_collection_dock_widget->addActionExperimental(child->windowMenuAction());

	// Show the child window we just added.
	mdisubwindow->show();
}

void MainWindow::addChildMDIModelViewPair_Library(const MDIModelViewPair& mvpair)
{
	if(mvpair.hasView())
	{
		auto libview = qobject_cast<MDILibraryView*>(mvpair.m_view);

		Q_CHECK_PTR(libview);

		// Did a view of the URL the user specified already exist?
		if(mvpair.m_view_was_existing)
		{
			// View already existed, just activate its parent subwindow and we're done.
			qDebug() << "View already existed";
			m_mdi_area->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(mvpair.m_view->parent()));
		}
		else
		{
			// View is new.
			connectActiveMDITreeViewBaseAndMetadataDock(libview, m_metadataDockWidget);
			connectLibraryViewAndMainWindow(libview);

			// Add the view as a new MDI child.
			addChildMDIView(mvpair.m_view);
		}
		statusBar()->showMessage(tr("Opened view on library '%1'").arg(libview->getDisplayName()));
	}

	if(mvpair.hasModel())
	{
		QPointer<LibraryModel> libmodel = qobject_cast<LibraryModel*>(mvpair.m_model);
		Q_CHECK_PTR(libmodel);

		bool model_really_already_existed = (std::find(m_libmodels.begin(), m_libmodels.end(), libmodel) != m_libmodels.end());

		// View is new, did the model already exist?
		if(mvpair.m_model_was_existing)
		{
			qDebug() << "Model existed:" << mvpair.m_model << libmodel->getLibRootDir() << libmodel->getLibraryName();
			Q_ASSERT(model_really_already_existed);
		}
		else
		{
			// Model is new.
			qDebug() << "Model is new:" << mvpair.m_model << libmodel->getLibRootDir() << libmodel->getLibraryName();
			Q_ASSERT(!model_really_already_existed);

			m_libmodels.push_back(libmodel);

			connectLibraryModelToActivityProgressWidget(libmodel.data(), m_activity_progress_widget);

			// Add the new library to the ModelViewPairs Model.
			// The Collection Doc Widget uses this among others.
			QStandardItem* new_lib_row_item = new QStandardItem(libmodel->getLibraryName());
			new_lib_row_item->setData(QVariant::fromValue(libmodel));
			new_lib_row_item->setData(QIcon::fromTheme("folder"), Qt::DecorationRole);
			QString tttext = tr("<b>%1</b><hr>%2").arg(libmodel->getLibraryName()).arg(libmodel->getLibRootDir().toDisplayString());
			new_lib_row_item->setData(QVariant(tttext), Qt::ToolTipRole);
			m_stditem_libraries->appendRow(new_lib_row_item);
			qDebug() << "LIBS ROWCOUNT:" << m_stditem_libraries->rowCount() << new_lib_row_item->parent();
		}
	}
}

void MainWindow::addChildMDIModelViewPair_Playlist(const MDIModelViewPair& mvpair)
{
	if(mvpair.hasView())
	{
		auto playlist_view = qobject_cast<MDIPlaylistView*>(mvpair.m_view);

		Q_CHECK_PTR(playlist_view);

		// Did a view of the URL the user specified already exist?
		if(mvpair.m_view_was_existing)
		{
			// View already existed, just activate its parent subwindow and we're done.
			qDebug() << "View already existed";
			m_mdi_area->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(mvpair.m_view->parent()));
		}
		else
		{
			// View is new.

			// Add the view as a new MDI child.
			addChildMDIView(playlist_view);

			// Add the new Playlist View to the ModelViewPairs Model.
			// The Collection Doc Widget uses this among others.
			QStandardItem* new_playlist_row_item = new QStandardItem(playlist_view->getDisplayName());
			new_playlist_row_item->setData(QVariant::fromValue(playlist_view));
			new_playlist_row_item->setData(QIcon::fromTheme("view-media-playlist"), Qt::DecorationRole);
			QString tttext = tr("<b>%1</b><hr>%2").arg(playlist_view->getDisplayName()).arg(playlist_view->windowFilePath());
			new_playlist_row_item->setData(QVariant(tttext), Qt::ToolTipRole);
			m_stditem_playlist_views->appendRow(new_playlist_row_item);
		}
		statusBar()->showMessage(tr("Opened view on playlist '%1'").arg(playlist_view->getDisplayName()));
	}

	if(mvpair.hasModel())
	{
		auto playlist_model = qobject_cast<PlaylistModel*>(mvpair.m_model);
		Q_CHECK_PTR(playlist_model);

		bool model_really_already_existed = (std::find(m_playlist_models.begin(), m_playlist_models.end(), playlist_model) != m_playlist_models.end());

		// View is new, did the model already exist?
		if(mvpair.m_model_was_existing)
		{
			qDebug() << "Model existed:" << mvpair.m_model << playlist_model->getLibRootDir() << playlist_model->getLibraryName();
			Q_ASSERT(model_really_already_existed);
		}
		else
		{
			// Model is new.
			qDebug() << "Model is new:" << mvpair.m_model << playlist_model->getLibRootDir() << playlist_model->getLibraryName();
			Q_ASSERT(!model_really_already_existed);

			// Add the underlying model to the PlaylistModel list.
			m_playlist_models.push_back(playlist_model);
		}
	}
}

// Top-level "saveAs" action handler for "Save playlist as..."
void MainWindow::savePlaylistAs()
{
	auto child = activeChildMDIView();
	if(child != nullptr)
	{
		MDIPlaylistView* playlist_ptr= qobject_cast<MDIPlaylistView*>(child);
		if(playlist_ptr != nullptr && playlist_ptr->saveAs())
		{
			statusBar()->showMessage("Playlist saved", 2000);
		}
	}
}

void MainWindow::onCloseSubwindow()
{
	auto active_subwin = this->m_mdi_area->activeSubWindow();
	qInfo() << "Closing MDI Subwindow:" << active_subwin;

	this->m_mdi_area->closeActiveSubWindow();
}

void MainWindow::onCut()
{
	auto active_child = qobject_cast<MDIPlaylistView*>(activeChildMDIView());
	if(active_child)
	{
		active_child->onCut();
	}
}

void MainWindow::onCopy()
{
	auto active_child = qobject_cast<MDITreeViewBase*>(activeChildMDIView());
	if(active_child)
	{
		active_child->onCopy();
	}
}

void MainWindow::onPaste()
{
	auto active_child = qobject_cast<MDIPlaylistView*>(activeChildMDIView());
	if(active_child)
	{
		active_child->onPaste();
	}
}

void MainWindow::onSelectAll()
{
	auto active_child = qobject_cast<MDITreeViewBase*>(activeChildMDIView());
	if(active_child)
	{
		active_child->onSelectAll();
	}
}

void MainWindow::onDelete()
{
	auto child_treeview = qobject_cast<MDIPlaylistView*>(activeChildMDIView());
	if(child_treeview)
	{
		child_treeview->onDelete();
	}
}

void MainWindow::startSettingsDialog()
{
	KConfigDialog *dialog = KConfigDialog::exists( "settings" );
	if( !dialog )
	{
		// KConfigDialog didn't find an instance of this dialog, so lets create it:
		dialog = new SettingsDialog(this, "settings", AMLMSettings::self());

		connect(dialog, &KConfigDialog::settingsChanged, this, &MainWindow::onSettingsChanged);
	}
	static_cast<SettingsDialog*>( dialog )->show( /*page*/);

#if 0
	if(!m_settings_dlg)
	{
		// This is the first time anyone has opened the settings dialog.
		m_settings_dlg = QSharedPointer<SettingsDialog>(new SettingsDialog(this, "App Settings", Settings::self()), &QObject::deleteLater);
	}

	// Open the settings dialog modeless.
	// Note this from the Qt5 docs:
	// http://doc.qt.io/qt-5/qdialog.html
	// "If you invoke the show() function after hiding a dialog, the dialog will be displayed in its original position. [...]
	// To preserve the position of a dialog that has been moved by the user, save its position in your closeEvent() handler and
	// then move the dialog to that position, before showing it again"
	m_settings_dlg->show();
	m_settings_dlg->raise();
	m_settings_dlg->activateWindow();
#endif
}

void MainWindow::onOpenShortcutDlg()
{
	// Start the Keyboard Shorcut editor dialog.
	KShortcutsDialog::configure(actionCollection(), KShortcutsEditor::LetterShortcutsDisallowed, this);

	AMLMSettings::self()->save();
}

void MainWindow::changeStyle(const QString& styleName)
{
	qDebug() << "signaled to set Style to" << styleName;
	qApp->setStyle(QStyleFactory::create(styleName));
	qApp->setPalette(qApp->style()->standardPalette());
}

void MainWindow::changeIconTheme(const QString& iconThemeName)
{
	qDebug() << "signaled to set Icon Theme to" << iconThemeName;

	Theme::setThemeName(iconThemeName);

	for(auto w : qApp->allWidgets())
	{
		QEvent style_changed_event(QEvent::StyleChange);
		QCoreApplication::sendEvent(w, &style_changed_event);
	}
}

void MainWindow::onChangeStyle(QAction *action)
{
	// Get the name of the style to change to.
	QString style = action->data().toString();

	// Update the settings first.
	/// @todo Not sure if this is really correct.  If the new style e.g. causes a crash, we
	///       will have maybe permanently hosed ourself by now always starting with that style.
	AMLMSettings::setWidgetStyle(style);

	// Do the actual style change work.
	doChangeStyle();
}

void MainWindow::doChangeStyle()
{
	QString newStyle = AMLMSettings::widgetStyle();
	if (newStyle.isEmpty() || newStyle == QStringLiteral("Default"))
	{
		newStyle = Theme::getUserDefaultStyle("Breeze");
	}
	QApplication::setStyle(QStyleFactory::create(newStyle));

//	// Changing widget style resets color theme, so update color theme again
//	ThemeManager::instance()->slotChangePalette();
}

void MainWindow::about()
{
    AboutBox about_box(this);

	about_box.exec();
}

void MainWindow::setActiveSubWindow(QMdiSubWindow* window)
{
	m_mdi_area->setActiveSubWindow(window);
}


void MainWindow::doExperiment()
{
	m_experimental->DoExperiment();
}

void MainWindow::onChangeWindowMode(QAction* action)
{
	if(action == m_tabs_act)
	{
		m_mdi_area->setViewMode(QMdiArea::TabbedView);
	}
	else
	{
		m_mdi_area->setViewMode(QMdiArea::SubWindowView);
	}
}

void MainWindow::onShowMenuBar(bool show)
{
	if (!show)
	{
		KMessageBox::information(this, tr("This will hide the menu bar completely. You can show it again by typing Ctrl+M."),
								 tr("Hide menu bar"), QStringLiteral("show-menubar-warning"));
	}
M_WARNING("TODO: CTRL+M doesn't get the bar back.");
//	menuBar()->setVisible(show);
}

void MainWindow::onConfigureToolbars()
{
	auto config_group = KSharedConfig::openConfig()->group("MainWindowToolbarSettings");

	saveMainWindowSettings(config_group);

	KEditToolBar dialog(factory(), this);

	connect(&dialog, &KEditToolBar::newToolbarConfig, this, &MainWindow::onApplyToolbarConfig);

	dialog.exec();
}

void MainWindow::onApplyToolbarConfig()
{
	auto config_group = KSharedConfig::openConfig()->group("MainWindowToolbarSettings");

	applyMainWindowSettings(config_group);
}

void MainWindow::onSettingsChanged()
{
	qDb() << "SLOT: Settings changed";
	Q_EMIT settingsChanged();
}


void MainWindow::onTextFilterChanged()
{
	auto filterWidget = m_filterToolbar->findChild<FilterWidget*>();
	Q_ASSERT(filterWidget != nullptr);
	QRegExp regExp(filterWidget->text(),
					   filterWidget->caseSensitivity(),
					   filterWidget->patternSyntax());

	auto mdisubwin = m_mdi_area->currentSubWindow();
	MDILibraryView* libtreeview = mdisubwin ? mdisubwin->findChild<MDILibraryView*>() : nullptr;
	if(libtreeview)
	{
		libtreeview->proxy_model()->setFilterRegExp(regExp);
	}
}


void MainWindow::stopAllBackgroundThreads()
{
	for(auto model : m_libmodels)
	{
		model->stopAllBackgroundThreads();
	}
}

void MainWindow::saveProperties(KConfigGroup& config_group)
{
	BASE_CLASS::saveProperties(config_group);
}

void MainWindow::readProperties(const KConfigGroup& config_group)
{
	BASE_CLASS::readProperties(config_group);
}

bool MainWindow::queryClose()
{
	return BASE_CLASS::queryClose();
}

//////
////// Slots
//////

void MainWindow::onSubWindowActivated(QMdiSubWindow *subwindow)
{
//	qDebug() << "Activated subwindow:" << subwindow;
	if(subwindow)
	{
		auto mdibase = qobject_cast<MDITreeViewBase*>(subwindow->widget());
		if(mdibase)
		{
			qDebug() << "Updating actions";
			updateActionEnableStates();
			updateConnections();
		}
	}
}



