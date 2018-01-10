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

#include "FilterWidget.h"
#include "MainWindow.h"
#include "NetworkAwareFileDialog.h"

#include "MDITreeViewBase.h"
#include "MDILibraryView.h"
#include "MDIPlaylistView.h"
#include "gui/MDIArea.h"
#include "MetadataDockWidget.h"
#include "CollectionDockWidget.h"

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

#include <functional>
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

//
// Note: Very roughly based on Qt5 MDI example, http://www.informit.com/articles/article.aspx?p=1405543&seqNum=6, and counless
// other variations on the theme.
//

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags) : QMainWindow(parent, flags), m_player(parent)
{
    // Name our GUI thread.
    QThread::currentThread()->setObjectName("GUIThread");
    qDebug() << "Current thread:" << QThread::currentThread()->objectName();


    // Get some standard paths.
    // App-specific cache directory.
    m_cachedir = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    qInfo() << "App cache dir:" << m_cachedir;
    // App-specific directory where persistent application data can be stored.  On Windows, this is the roaming, not local, path.
    m_appdatadir = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    qInfo() << "App data dir:" << m_appdatadir;

    // Look for icons.
    Theme::initialize();

    /// @todo
    changeIconTheme(QIcon::themeName());

    // Follow the system style for the Icon&/|Text setting for toolbar buttons.
    setToolButtonStyle(Qt::ToolButtonFollowStyle);

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

    readSettings();

    setWindowTitle(qApp->applicationDisplayName());

    setUnifiedTitleAndToolBarOnMac(true);

    // Send ourself a message to re-load the files we had open last time we were closed.
    QTimer::singleShot(0, this, &MainWindow::onStartup);

}

MainWindow::~MainWindow()
{

}

MainWindow* MainWindow::getInstance()
{
	// Search the qApp for the main window.
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
	auto childIsBaseClass = qobject_cast<MDITreeViewBase*>(activeChildMDIView());
	auto childIsPlaylist = qobject_cast<MDIPlaylistView*>(activeChildMDIView());
	auto childIsLibrary = qobject_cast<MDILibraryView*>(activeChildMDIView());

	/// Update file actions.
	m_saveLibraryAsAct->setEnabled(childIsLibrary);
	m_savePlaylistAct->setEnabled(childIsPlaylist);

	// Update the Window menu actions.
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

    //
	// Tools actions.
    //

	m_rescanLibraryAct = make_action(QIcon::fromTheme("view-refresh"), tr("&Rescan libray..."), this,
									QKeySequence::Refresh);
	connect_trig(m_rescanLibraryAct, this, &MainWindow::onRescanLibrary);

	m_settingsAct = make_action(QIcon::fromTheme("configure"), tr("Settings..."), this,
							   QKeySequence::Preferences, "Open the Settings dialog.");
	connect_trig(m_settingsAct, this, &MainWindow::startSettingsDialog);

	m_scanLibraryAction = make_action(QIcon::fromTheme("tools-check-spelling"), "Scan library", this,
							   QKeySequence(), "Scan library for problems");

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

	m_closeAct = make_action(QIcon::fromTheme("window-close"), "Cl&ose", this,
                            QKeySequence::Close,
                            "Close the active window");
	connect_trig(m_closeAct, this->m_mdi_area, &QMdiArea::closeActiveSubWindow);

	m_closeAllAct = make_action(QIcon::fromTheme("window-close-all"), "Close &All", this,
                              QKeySequence(),
                               "Close all the windows");
	connect_trig(m_closeAllAct, this->m_mdi_area, &QMdiArea::closeAllSubWindows);

	m_act_window_list_separator = new QAction(this);
	m_act_window_list_separator->setText(tr("Window List"));
	m_act_window_list_separator->setSeparator(true);

	m_act_group_window = new QActionGroup(this);

	//
    // Help actions.
	//
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

	/// Experimental actions
	m_experimentalAct = make_action(QIcon::fromTheme("edit-bomb"), "Experimental", this,
								   QKeySequence(), "Invoke experimental code - USE AT YOUR OWN RISK");
	connect_trig(m_experimentalAct, this, &MainWindow::doExperiment);
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

	m_act_lock_layout = make_action(Theme::iconFromTheme(""), tr("Lock layout"), this);
	m_act_reset_layout = make_action(Theme::iconFromTheme(""), tr("Reset layout"), this);
M_WARNING("TODO: These appear to be unreparentable, so we can't give them to an ActionBundle.");
//	m_ab_docks->addAction(m_libraryDockWidget->toggleViewAction());
//	m_ab_docks->addAction(m_metadataDockWidget->toggleViewAction());
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
	m_menu_edit->setTearOffEnabled(true);

    // Create the View menu.
	m_viewMenu = menuBar()->addMenu(tr("&View"));
	m_ab_docks->appendToMenu(m_viewMenu);
	m_viewMenu->addActions({
							   m_act_lock_layout,
							   m_act_reset_layout
						   });

    // Tools menu.
	m_toolsMenu = menuBar()->addMenu("&Tools");
	m_toolsMenu->addActions(
        {//scanLibraryAction,
		 m_toolsMenu->addSection("Rescans"),
		 m_rescanLibraryAct,
		 m_toolsMenu->addSection("Settings"),
		 m_settingsAct,
                });

    // Create the Window menu.
	m_menu_window = menuBar()->addMenu(tr("&Window"));
	m_menu_window->addActions({
		m_menu_window->addSection(tr("Close")),
		m_closeAct,
		m_closeAllAct,
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

	// Create the help menu.
	m_helpMenu = menuBar()->addMenu("&Help");
	m_helpMenu->addSection("Help");
	m_helpMenu->addAction(m_helpAct);
	m_helpMenu->addAction(m_whatsThisAct);
	m_helpMenu->addSection("About");
	m_helpMenu->addAction(m_aboutAct);
	m_helpMenu->addAction(m_aboutQtAct);
}


void MainWindow::createToolBars()
{
	//
	// File
	//
	m_fileToolBar = addToolBar(tr("File"));
	m_fileToolBar->setObjectName("FileToolbar");
	m_fileToolBar->addActions({m_importLibAct,
	                           m_rescanLibraryAct,
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
	m_libraryDockWidget = new CollectionDockWidget("Media Sources", this);
	addDockWidget(Qt::LeftDockWidgetArea, m_libraryDockWidget);

    // Create the metadata dock widget.
	m_metadataDockWidget = new MetadataDockWidget(tr("Metadata Explorer"), this);
	m_metadataDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_metadataDockWidget);
}

void MainWindow::newNowPlaying()
{
    auto child = new MDINowPlayingView(this);
    child->newFile();

    // Add the new child's underlying model to the list of playlist models.
    /// @todo REMOVE, only one Now Playing.
    m_playlist_models.push_back(child->underlyingModel());
    // Set this view's model as the single "Now Playing" model.
    m_now_playing_playlist_model = child->underlyingModel();

    /// @todo Do we really need to keep this as a member pointer?
    m_now_playing_playlist_view = child;

    connectNowPlayingViewAndMainWindow(child);

    addChildMDIView(child);

    statusBar()->showMessage(tr("Opened 'Now Playing' Playlist '%1'").arg(child->windowTitle()));

	// Add the new playlist to the collection doc widget.
    m_libraryDockWidget->addPlaylist(new PlaylistItem(child));
}

void MainWindow::createConnections()
{
	/// @todo
	connect(qApp, &QApplication::focusChanged, this, &MainWindow::onFocusChanged);

    // Connect player controls up to player.
	connectPlayerAndControls(&m_player, m_controls);

    // Connect with the CollectionDockWidget.
	connect(m_libraryDockWidget, &CollectionDockWidget::showLibViewSignal, this, &MainWindow::onShowLibrary);
	connect(m_libraryDockWidget, &CollectionDockWidget::removeLibModelFromLibSignal, this, &MainWindow::onRemoveDirFromLibrary);

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
M_WARNING("TODO: Hide qMediaPlaylist behind playlist_view?");
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

void MainWindow::connectLibraryToActivityProgressWidget(LibraryModel* lm, ActivityProgressWidget* apw)
{
	lm->connectProgressToActivityProgressWidget(apw);
}

void MainWindow::connectLibraryViewAndMainWindow(MDILibraryView *lv)
{
    qDebug() << "Connecting" << lv << "and" << this;
	connect(lv, &MDILibraryView::sendEntryToPlaylist, this, &MainWindow::onSendEntryToPlaylist);
	connect(lv, &MDILibraryView::sendToNowPlaying, this, &MainWindow::onSendToNowPlaying);
}

void MainWindow::connectNowPlayingViewAndMainWindow(MDIPlaylistView* plv)
{
    qDebug() << "Connecting";
	connect(this, &MainWindow::sendToNowPlaying, plv, &MDIPlaylistView::onSendToNowPlaying);

	connectPlayerAndPlaylistView(&m_player, plv);
	connectPlayerControlsAndPlaylistView(m_controls, plv);
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


        if(childIsLibrary)
        {
			auto connection_handle = connect(childIsLibrary,
										&MDILibraryView::playTrackNowSignal,
										this,
										&MainWindow::onPlayTrackNowSignal,
										Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
			if (!connection_handle)
			{
//				qDebug() << "Connection failed: already connected?";
			}
        }
		if(childIsPlaylist)
		{
//			connect_trig(m_act_paste, childIsPlaylist, &MDIPlaylistView::onPaste);
		}
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
		if(QMessageBox::warning(this, "Save Error",
								QString("Failed to save %1\nQuit anyway?").arg(failures.join(',')), /// @todo % "\n\t".join(failures),
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
	qDebug() << QString("Main Window received closeEvent.");
	stopAllBackgroundThreads();
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
 * Find any existing child MDI window associated with the give url.
 */
QMdiSubWindow* MainWindow::findSubWindow(QUrl url)
{
	for(auto window : m_mdi_area->subWindowList())
	{
		auto widget = window->widget();
		if(widget == nullptr)
		{
			qCritical() << "Mdi child has no widget.";
		}
		else if(widget->windowFilePath() == url.toString())
		{
			return window;
		}
	}
	return nullptr;
}

void MainWindow::onFocusChanged(QWidget* old, QWidget* now)
{
    qDebug() << "Keyboard focus has changed from" << old << "to" << now;
}

//////
////// Top-level QSettings save/restore.
//////

void MainWindow::readSettings()
{
        QSettings settings;
        settings.beginGroup("mainwindow");
        auto geometry = settings.value("geometry", QByteArray());
        if(geometry.isNull() || !geometry.isValid())
        {
            qDebug() << "No saved window geometry, using defaults.";
        }
        else
        {
            restoreGeometry(geometry.toByteArray());
        }
        auto window_state = settings.value("window_state", QByteArray());
        if(!window_state.isNull() && window_state.isValid())
        {
            restoreState(window_state.toByteArray());
        }
		settings.endGroup();
}

void MainWindow::readLibSettings(QSettings& settings)
{
	int num_libs = settings.beginReadArray("libraries");
	qDebug() << "Reading" << num_libs << "libraries...";
	for(int i = 0; i < num_libs; ++i)
	{
		settings.setArrayIndex(i);

		QByteArray jdoc_str = settings.value("asJson").toByteArray();

//		qDebug() << "jdoc_str=" << jdoc_str;
		QJsonDocument jsondoc = QJsonDocument::fromJson(jdoc_str);
//		qDebug() << "Jsondoc:" << jsondoc.toJson();
//		qDebug() << "Jsondoc is object?:" << jsondoc.isObject();

		QSharedPointer<LibraryModel> libmodel = LibraryModel::constructFromJson(jsondoc.object(), this);

		if(libmodel == nullptr)
		{
			QMessageBox::critical(this, qApp->applicationDisplayName(), "Failed to open library",
								  QMessageBox::Ok);
		}
		else
		{
			m_libmodels.push_back(libmodel);
			// Add the new library to the Collection Doc Widget.
			m_libraryDockWidget->addLibrary(new LocalLibraryItem(libmodel.data()));
		}
	}
	settings.endArray();
}

void MainWindow::writeSettings()
{
	qDebug() << "writeSettings() start";
	QSettings settings;
	settings.beginGroup("mainwindow");
	settings.setValue("geometry", saveGeometry());
	settings.setValue("window_state", saveState());
	settings.endGroup();
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
    // Set the Icon Theme.
    changeIconTheme(QIcon::themeName());

    // Create the "Now Playing" playlist and view.
    newNowPlaying();

	// Load any files which were opened at the time the last session was closed.
	qDebug() << QString("Loading files from last session...");
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
	qDebug() << "Opening windows which were opened from last session...";

	for(auto m : m_libmodels)
	{
		qDebug() << "Opening view on model:" << m->getLibraryName() << m->getLibRootDir();
		openMDILibraryViewOnModel(m.data());
	}
}


//////
////// Action targets.
//////

QSharedPointer<LibraryModel> MainWindow::openLibraryModelOnUrl(QUrl url)
{
	// Create the new LibraryModel.
	auto lib = QSharedPointer<LibraryModel>(new LibraryModel(this));

	// Connect it to the ActivityProgressWidget, since as soon as we set the URL, async activity will start.
	connectLibraryToActivityProgressWidget(lib.data(), m_activity_progress_widget);

	m_libmodels.push_back(lib);
	lib->setLibraryRootUrl(url);

	// Add the new library to the Collection Doc Widget.
	m_libraryDockWidget->addLibrary(new LocalLibraryItem(lib.data()));

	return lib;
}

void MainWindow::openMDILibraryViewOnModel(LibraryModel* libmodel)
{
	if(libmodel != nullptr)
	{
		// First check if we already have a view open.
		auto existing = findSubWindow(libmodel->getLibRootDir());
		if(existing != nullptr)
		{
			// Already have a view open, switch to it.
			m_mdi_area->setActiveSubWindow(existing);
			return;
		}

		// No view open, create a new one.
//		auto child = MDILibraryView::openModel(libmodel);
        auto child = createMdiChildLibraryView();
        child->setModel(libmodel);
//		if(child)
//		{
//			addChildMDIView(child);
//		}

M_WARNING("TODO: These seem out of place.");
        connectLibraryToActivityProgressWidget(libmodel, m_activity_progress_widget);
        connectActiveMDITreeViewBaseAndMetadataDock(child, m_metadataDockWidget);

		statusBar()->showMessage(QString("Opened view on library '%1'").arg(libmodel->getLibraryName()));
	}
}

void MainWindow::importLib()
{
	// Add a directory as the root of a new library.

	auto liburl = NetworkAwareFileDialog::getExistingDirectoryUrl(this, "Select a directory to import", QUrl(), "import_dir");
	QUrl lib_url = liburl.first;

	if(lib_url.isEmpty())
	{
		qDebug() << "User cancelled.";
		return;
	}

	LibraryModel* lib = nullptr;
	for(auto l : m_libmodels)
	{
		if(l->getLibRootDir() == lib_url)
		{
			qDebug() << "LibraryModel URL '" << lib_url << "' is already open.";
			lib = l.data();
		}
	}
	if(lib == nullptr)
	{
        // Create the new LibraryModel.
		lib = openLibraryModelOnUrl(lib_url).data();
	}

    // Open a view on it.
	openMDILibraryViewOnModel(lib);
    return;
}

void MainWindow::onRescanLibrary()
{
	// Start a rescan on all models.
	for(auto l : m_libmodels)
	{
		l->startRescan();
	}
}

void MainWindow::onShowLibrary(LibraryModel* libmodel)
{
	qDebug() << QString("onShowLibrary");
	openMDILibraryViewOnModel(libmodel);
	return;
}


void MainWindow::onRemoveDirFromLibrary(LibraryModel* libmodel)
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
    auto child = new MDIPlaylistView(this); ///@todo createMdiChildPlaylistView();

    // Tell it to create a new, empty model.
    child->newFile();

    // Add the underlying model to the list.
    m_playlist_models.push_back(child->underlyingModel());

    // Add it to the child views.
    addChildMDIView(child);
    statusBar()->showMessage(tr("Opened new Playlist '%1'").arg(child->windowTitle()));
}

/**
 * Top-level menu/toolbar action for opening an existing playlist.
 * ~= "File->Open...".
 */
void MainWindow::openPlaylist()
{
	qCritical() << "Not implemented";
}

void MainWindow::onPlayTrackNowSignal(QUrl url)
{
	qWarning() << QString("PlayTrackNow not implemented: '%1'").arg(url.toString());
}

void MainWindow::onSendEntryToPlaylist(std::shared_ptr<LibraryEntry> libentry, std::shared_ptr<PlaylistModel> playlist_model)
{
	qDebug() << QString("Sending entry to playlist:") << playlist_model.get();
	if(playlist_model != nullptr)
	{
		auto new_playlist_entry = PlaylistModelItem::createFromLibraryEntry(libentry);
		playlist_model->appendRow(new_playlist_entry);
	}
}

void MainWindow::onSendToNowPlaying(std::shared_ptr<LibraryEntry> libentry)
{
	// Resend the entry to the "Now Playing" playlist view.
    qDebug() << "Re-emitting sendToNowPlaying";
	emit sendToNowPlaying(libentry);
}

/**
 * Add a child view to the MDIArea and hook it up to a few signals/slots.
 */
void MainWindow::addChildMDIView(MDITreeViewBase* child)
{
	// Connect Cut, Copy, Delete, and Select All actions to the availability signals emitted by the child.
    /// @note This works because only the active child will send this signal.
    /// Otherwise we'd need to swap which child was connected to the menu.
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

	// Show the child window we just added.
	mdisubwindow->show();
}

MDILibraryView* MainWindow::createMdiChildLibraryView()
{
	// Create a new, empty LibraryView.

	// New Lib MDI View.
	auto child = new MDILibraryView(this);

	connectLibraryViewAndMainWindow(child);

	addChildMDIView(child);

	return child;
}

/**
 * Creates a new, empty "Now Playing" playlist and view, then adds it to the MDIArea.
 * @return
 */
MDIPlaylistView* MainWindow::createMdiChildPlaylistView()
{
	// Create a new playlist model.
//	auto new_playlist_model = new PlaylistModel(this);
//	m_playlist_models.push_back(new_playlist_model);

//	MDIPlaylistView* child = new MDIPlaylistView(this);
//	child->setModel(new_playlist_model);
//	auto child = MDIPlaylistView::openModel(new_playlist_model, this);
    auto child = new MDIPlaylistView(this);
    child->newFile();
    m_playlist_models.push_back(child->underlyingModel());

	addChildMDIView(child);

	// Add the new playlist to the collection doc widget.
	m_libraryDockWidget->addPlaylist(new PlaylistItem(child));
	return child;
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
	if(!m_settings_dlg)
	{
		// This is the first time anyone has opened the settings dialog.
		m_settings_dlg = QSharedPointer<SettingsDialog>(new SettingsDialog(this, this->windowFlags()), &QObject::deleteLater);
	}

	m_settings_dlg->exec();
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

void MainWindow::about()
{
    AboutBox about_box(this);

	about_box.exec();
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

//////
////// Slots
//////

void MainWindow::onSubWindowActivated(QMdiSubWindow *subwindow)
{
	qDebug() << "Activated subwindow:" << subwindow;
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

