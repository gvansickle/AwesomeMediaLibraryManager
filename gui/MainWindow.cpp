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
#include "utils/DebugHelpers.h"

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
#include <QMdiSubWindow>
#include <QTimer>
#include <QMessageBox>
#include <QSettings>
#include <QJsonObject>
#include <QJsonDocument>
#include <QComboBox>
#include <QStyleFactory>
#include <QDirIterator>

#include <functional>
#include <type_traits>

#include <logic/MP2.h>
#include <utils/Theme.h>
#include <QtCore/QThread>
#include <QtWidgets/QWhatsThis>

#include "gui/ActivityProgressWidget.h"
#include "AboutBox.h"

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

	connect(m_mdi_area, &QMdiArea::subWindowActivated, this, &MainWindow::subWindowActivated);

    // Mapper for the Window menu.
	m_windowMapper = new QSignalMapper(this);
	connect(m_windowMapper, SIGNAL(mapped(QWidget*)),
            this, SLOT(setActiveSubWindow(QWidget*)));

    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    createDockWindows();
    updateMenus();

	updateActions();

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

void MainWindow::updateActions()
{
	qDebug() << "ENTER";
	if(activeMdiChild() != nullptr)
	{
		qDebug() << "Active child:" << activeMdiChild();

		// We have an active MDI child.  What is it?
		auto childIsPlaylist = dynamic_cast<MDIPlaylistView*>(activeMdiChild());
		auto childIsLibrary = dynamic_cast<MDILibraryView*>(activeMdiChild());
		auto childBaseClass = dynamic_cast<MDITreeViewBase*>(activeMdiChild());

		if(childBaseClass)
		{
			// Update edit actions.

			// It's something that might have a selection.
			bool has_selection = childBaseClass->selectionModel()->hasSelection();

			// Can copy from any derived class if it has a selection.
			m_act_copy->setEnabled(has_selection);

			// A playlist can also cut and delete.
			auto mutating_actions = {m_act_cut, m_act_delete};
			for(auto act : mutating_actions)
			{
				act->setEnabled(childIsPlaylist && has_selection);
			}

			// We can paste into a Playlist regardless of selection.
			m_act_paste->setEnabled(childIsPlaylist);

			return;
		}
	}

	// No active MDI child, or not one that could have a selection or be pasted into.
	for(auto i : {m_act_copy, m_act_cut, m_act_paste, m_act_delete})
	{
		i->setDisabled(true);
	}

}

void MainWindow::createActions()
{
    // File actions.
    ////// Library actions.
    m_importLibAct = make_action(QIcon::fromTheme("folder-open"), "&Import library...", this,
                                QKeySequence("CTRL+SHIFT+O"),
                                "Add a library location");
	connect_trig(m_importLibAct, this, &MainWindow::importLib);


	m_rescanLibraryAct = make_action(QIcon::fromTheme("view-refresh"), "&Rescan libray...", this,
                                    QKeySequence::Refresh);
	connect_trig(m_rescanLibraryAct, this, &MainWindow::onRescanLibrary);

	m_saveLibraryAsAct = make_action(QIcon::fromTheme("folder-close"), "&Save library as...", this);
                                ///triggered=saveLibraryAs);
	m_removeDirFromLibrary = make_action(QIcon::fromTheme("edit-delete"), "Remove &Dir from library...", this);

    ////// Playlist actions.
	m_newPlaylistAct = make_action(QIcon::fromTheme("document-new"), "&New playlist...", this,
                                  QKeySequence::New,
                                  "Create a new playlist");
	connect_trig(m_newPlaylistAct, this, &MainWindow::newPlaylist);

	m_openPlaylistAct = make_action(QIcon::fromTheme("document-open"), "&Open playlist...", this,
                                QKeySequence::Open,
                                "Open an existing playlist");
                                ///triggered=openPlaylist);
	m_savePlaylistAct = make_action(QIcon::fromTheme("document-save"), "&Save playlist as...", this,
                                   QKeySequence::Save);
                                   ///triggered=savePlaylistAs);
	connect_trig(m_savePlaylistAct, this, &MainWindow::savePlaylistAs);

	m_settingsAct = make_action(QIcon::fromTheme("configure"), "Settings...", this,
							   QKeySequence::Preferences, "Open the Settings dialog.");
	connect_trig(m_settingsAct, this, &MainWindow::startSettingsDialog);

	m_exitAction = make_action(QIcon::fromTheme("application-exit"), "E&xit", this,
                              QKeySequence::Quit,
                              "Exit application");
	connect_trig(m_exitAction, this, &MainWindow::close);

	//
	// Edit actions.
	//
	createEditActions();

	//////// Tools actions.

	m_scanLibraryAction = make_action(QIcon::fromTheme("tools-check-spelling"), "Scan library", this,
							   QKeySequence(), "Scan library for problems");
							   ///triggered=scanLibrary)

    // Window actions.
	m_tabs_or_subwindows_group = new QActionGroup(this);
	m_tabs_act = make_action(QIcon::fromTheme(""), "Tabs", m_tabs_or_subwindows_group,
							 QKeySequence(), "Display as tabs");
	m_tabs_act->setCheckable(true);

	m_subwins_act = make_action(QIcon::fromTheme(""), "Subwindows", m_tabs_or_subwindows_group,
								QKeySequence(), "Display as subwindows");
	m_subwins_act->setCheckable(true);
	m_tabs_act->setChecked(true);
	connect(m_tabs_or_subwindows_group, &QActionGroup::triggered, this, &MainWindow::onChangeWindowMode);

	m_windowNextAct = make_action(QIcon::fromTheme("go-next"), "&Next", this,
                                 QKeySequence::NextChild);
	connect_trig(m_windowNextAct, this->m_mdi_area, &QMdiArea::activateNextSubWindow);
                                 ///triggered=mdi_area.activateNextSubWindow)
	m_windowPrevAct = make_action(QIcon::fromTheme("go-previous"), "&Previous", this,
                            QKeySequence::PreviousChild);
	connect_trig(m_windowPrevAct, this->m_mdi_area, &QMdiArea::activatePreviousSubWindow);
                                ///triggered=mdi_area.activatePreviousSubWindow,

	m_windowCascadeAct = make_action(QIcon::fromTheme("window-cascade"), "Cascade", this);
	connect_trig(m_windowCascadeAct, this->m_mdi_area, &QMdiArea::cascadeSubWindows);
                                    ///triggered=mdi_area.cascadeSubWindows)
	m_windowTileAct = make_action(QIcon::fromTheme("window-tile"), "Tile", this);
	connect_trig(m_windowTileAct, this->m_mdi_area, &QMdiArea::tileSubWindows);
                                 ///triggered=mdi_area.tileSubWindows)
	m_closeAct = make_action(QIcon::fromTheme("window-close"), "Cl&ose", this,
                            QKeySequence::Close,
                            "Close the active window");
	connect_trig(m_closeAct, this->m_mdi_area, &QMdiArea::closeActiveSubWindow);
                            ///triggered=mdi_area.closeActiveSubWindow);
	m_closeAllAct = make_action(QIcon::fromTheme("window-close-all"), "Close &All", this,
                              QKeySequence(),
                               "Close all the windows");
	connect_trig(m_closeAllAct, this->m_mdi_area, &QMdiArea::closeAllSubWindows);

    // Help actions.
	m_helpAct = make_action(Theme::iconFromTheme("help-contents"), "&Help", this,
	                        QKeySequence::HelpContents,
							"Show help contents");
	m_helpAct->setDisabled(true); /// @todo No help yet.
	// Qt5 has a pre-fabbed "What's This" action which handles everything, we don't need to even add a handler or an icon.
	m_whatsThisAct = QWhatsThis::createAction(this);
	m_whatsThisAct->setStatusTip("Show more than a tooltip, less than full help on a GUI item");

	m_aboutAct = make_action(QIcon::fromTheme("help-about"), "&About", this,
                           QKeySequence(),
                            "Show the About box");
	connect_trig(m_aboutAct, this, &MainWindow::about);

	m_aboutQtAct = make_action(QIcon::fromTheme("help-about-qt"), "About &Qt", this,
                             QKeySequence(),
                              "Show the Qt library's About box");
	connect(m_aboutQtAct, &QAction::triggered, this, &QApplication::aboutQt);

	/// Experimental actions
	m_experimentalAct = make_action(QIcon::fromTheme("edit-bomb"), "Experimental", this,
								   QKeySequence(), "Invoke experimental code - USE AT YOUR OWN RISK");
								   //triggered=doExperiment);
	connect_trig(m_experimentalAct, this, &MainWindow::doExperiment);
}

void MainWindow::createEditActions()
{
	m_act_cut = make_action(Theme::iconFromTheme("edit-cut"), tr("Cu&t"), this, QKeySequence::Cut,
							tr("Cut the current selection to the clipboard"));
	m_act_copy = make_action(Theme::iconFromTheme("edit-copy"), tr("&Copy"), this, QKeySequence::Copy,
							 tr("Copy the current selection to the clipboard"));
	//connect_trig(m_act_copy, this, &MainWindow::copy);
	m_act_paste = make_action(Theme::iconFromTheme("edit-paste"), tr("&Paste"), this, QKeySequence::Paste,
							  tr("Paste the clipboard's contents into the current selection"));

	m_act_delete = make_action(Theme::iconFromTheme("edit-delete"), tr("Delete"), this, QKeySequence::Delete,
							   tr("Delete this entry"));

}


void MainWindow::createMenus()
{
	m_fileMenu = menuBar()->addMenu("&File");

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
	m_menu_edit = menuBar()->addMenu("&Edit");
	m_menu_edit->addActions({
								m_act_cut,
								m_act_copy,
								m_act_paste
							});

    // Create the View menu.
	m_viewMenu = menuBar()->addMenu("&View");

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
	m_windowMenu = menuBar()->addMenu("&Window");
    updateWindowMenu();
	connect(m_windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

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
	m_fileToolBar = addToolBar("File");
	m_fileToolBar->setObjectName("FileToolbar");
	m_fileToolBar->addActions({m_importLibAct,
	                           m_rescanLibraryAct,
							 m_fileToolBar->addSeparator(),
							 m_newPlaylistAct,
							 m_openPlaylistAct,
							 m_savePlaylistAct});

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

void MainWindow::createDockWindows()
{

    // Create the Library/Playlist dock widget.
	m_libraryDockWidget = new CollectionDockWidget("Media Sources", this);
	addDockWidget(Qt::LeftDockWidgetArea, m_libraryDockWidget);

    // Create the metadata dock widget.
	m_metadataDockWidget = new MetadataDockWidget("Metadata", this);
	m_metadataDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, m_metadataDockWidget);
    //player.playlistSelectionChanged.connect(metadataDockWidget.playlistSelectionChanged)
}

void MainWindow::createConnections()
{
	/// @todo
	connect(qApp, &QApplication::focusChanged, this, &MainWindow::onFocusChanged);

	// Connect menu "about to shows" to the action updater.
	connect(m_menu_edit, &QMenu::aboutToShow, this, &MainWindow::updateActions);

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
	connect(lv, &MDILibraryView::sendEntryToPlaylist, this, &MainWindow::onSendEntryToPlaylist);
	connect(lv, &MDILibraryView::sendToNowPlaying, this, &MainWindow::onSendToNowPlaying);
}

void MainWindow::connectNowPlayingViewAndMainWindow(MDIPlaylistView* plv)
{
	connect(this, &MainWindow::sendToNowPlaying, plv, &MDIPlaylistView::onSendToNowPlaying);

	connectPlayerAndPlaylistView(&m_player, plv);
	connectPlayerControlsAndPlaylistView(m_controls, plv);
}

void MainWindow::updateConnections()
{
	if(activeMdiChild() != nullptr)
	{
//		qDebug() << "Updating connectons for activated window" << activeMdiChild()->windowTitle();

		auto childIsPlaylist = dynamic_cast<MDIPlaylistView*>(activeMdiChild());
		auto childIsLibrary = dynamic_cast<MDILibraryView*>(activeMdiChild());

		if(childIsLibrary != nullptr)
		{
			auto connection_handle = connect(activeMdiChild()->selectionModel(), &QItemSelectionModel::selectionChanged,
			                                 m_metadataDockWidget, &MetadataDockWidget::playlistSelectionChanged,
			                                 Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
			if (!connection_handle)
			{
//				qDebug() << "Connection failed: already connected?";
			}

			connection_handle = connect(childIsLibrary,
			                            &MDILibraryView::playTrackNowSignal,
			                            this,
			                            &MainWindow::onPlayTrackNowSignal,
			                            Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
			if (!connection_handle)
			{
//				qDebug() << "Connection failed: already connected?";
			}
		}
	}
}

void MainWindow::updateMenus()
{
		// Update action enable states.  Mainly depends on if we have an MDI child window open.
//        qDebug() << "Updating menu status";
        bool hasMdiChild = activeMdiChild() != nullptr;

        bool childIsPlaylist = (dynamic_cast<MDIPlaylistView*>(activeMdiChild()) != nullptr);
        bool childIsLibrary = (dynamic_cast<MDILibraryView*>(activeMdiChild()) != nullptr);

        // File actions.
        for(auto act : {
			m_saveLibraryAsAct,
            })
        {
            act->setEnabled(hasMdiChild && childIsLibrary);
        }
        for(auto act : {
			m_savePlaylistAct
            })
        {
            act->setEnabled(hasMdiChild && childIsPlaylist);
        }
}

void MainWindow::updateWindowMenu()
{
	m_windowMenu->clear();
	m_windowMenu->addActions({
		m_windowMenu->addSection(tr("Subwindow Mode")),
		m_tabs_act,
		m_subwins_act,
		m_windowMenu->addSection(tr("Window Navigation")),
		m_windowNextAct,
		m_windowPrevAct,
		m_windowCascadeAct,
		m_windowTileAct,
		m_windowMenu->addSection(tr("Close")),
		m_closeAct,
		m_closeAllAct
    });

	auto windows = m_mdi_area->subWindowList();
    if(windows.length() > 0)
    {
		m_windowMenu->addSection("Windows");
    }

    for(int i=0; i<windows.length(); ++i)
    {
        auto win = windows[i];
        auto child = win->widget();
        auto title = child->windowTitle();
		QString text = QString("%1 %2").arg(i + 1).arg(title);

        // Make the window number an accelerator key for the first 9 windows.
        if(i < 9)
        {
            text = "&" + text;
        }

		auto action = m_windowMenu->addAction(text);
        action->setCheckable(true);
        action->setChecked(child == activeMdiChild());
		connect_trig(action, m_windowMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
		m_windowMapper->setMapping(action, win);
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

/*Note: Returns the QMdiSubWindow widget(), not the subwindow itself. */
MDITreeViewBase* MainWindow::activeMdiChild()
{
	auto activeSubWindow = m_mdi_area->activeSubWindow();

    if(activeSubWindow)
    {
        return dynamic_cast<MDITreeViewBase*>(activeSubWindow->widget());
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

void MainWindow::setActiveSubWindow(QWidget *window)
{
//	qDebug() << "setActiveSubWindow: '" << window << "', '" << window->windowTitle() << "'";
	if(window != nullptr)
	{
		m_mdi_area->setActiveSubWindow(dynamic_cast<QMdiSubWindow*>(window));
	}
}

void MainWindow::onFocusChanged(QWidget* old, QWidget* now)
{
//	qDebug() << "Keyboard focus has changed from" << old << "to" << now;
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
			// Hook up the status signal from the library model to this class's onStatusSignal handler.
			connect(libmodel.data(), &LibraryModel::statusSignal, this, &MainWindow::onStatusSignal);
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
 */
void MainWindow::onStartup()
{
    // Set the Icon Theme.
    changeIconTheme(QIcon::themeName());

    // Create the "Now Playing" playlist.
    auto wins = createMdiNowPlayingView();
    m_now_playing_playlist_view = wins.first;
    QMdiSubWindow* mdisubwindow = wins.second;

M_WARNING("TODO: Specify a temp/cache file?")
    m_now_playing_playlist_view->newFile();

	connectNowPlayingViewAndMainWindow(m_now_playing_playlist_view);

    setActiveSubWindow(mdisubwindow);
    statusBar()->showMessage(QString("Opened 'Now Playing' Playlist '%1'").arg(m_now_playing_playlist_view->windowTitle()));

    m_now_playing_playlist_view->show();

	// Load any files which were opened at the time the last session was closed.
	qDebug() << QString("Loading files from last session...");
	QSettings settings;
	readLibSettings(settings);
	////// @todo
	openWindows();
}


void MainWindow::openWindows()
{
	qDebug() << QString("Opening windows which were opened from last session...");
	////// @todo Actually now always opens a window for each libmodel.
	for(auto m : m_libmodels)
	{
		qDebug() << QString("Opening view on model:") << m->getLibraryName() << m->getLibRootDir();
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
	m_libmodels.push_back(lib);
	lib->setLibraryRootUrl(url);

	// Add the new library to the Collection Doc Widget.
	m_libraryDockWidget->addLibrary(new LocalLibraryItem(lib.data()));

	// Hook up the status signal from the library model to this class's onStatusSignal handler.
	connect(lib.data(), &LibraryModel::statusSignal, this, &MainWindow::onStatusSignal);

	return lib;
}

void MainWindow::openMDILibraryViewOnModel(LibraryModel* libmodel)
{
	if(libmodel != nullptr)
	{
		auto existing = findSubWindow(libmodel->getLibRootDir());
		if(existing != nullptr)
		{
			// Already have a view open, switch to it.
			m_mdi_area->setActiveSubWindow(existing);
			return;
		}

		MDILibraryView* child;
		QMdiSubWindow* mdisubWindow;
		std::tie(child, mdisubWindow) = createMdiChildLibraryView();

		child->setModel(libmodel);
		setActiveSubWindow(mdisubWindow);
		connectLibraryToActivityProgressWidget(libmodel, m_activity_progress_widget);
		statusBar()->showMessage(QString("Opened view on library '%1'").arg(libmodel->getLibraryName()));
		child->show();
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


std::tuple<MDILibraryView*, QMdiSubWindow*> MainWindow::createMdiChildLibraryView()
{
	// Create a new, empty LibraryView.

	// New Lib MDI View.
	auto child = new MDILibraryView(this);
	auto mdisubwindow = m_mdi_area->addSubWindow(child);

	connectLibraryViewAndMainWindow(child);

	return std::make_tuple(child, mdisubwindow);
}

#if 0
    def saveLibraryAs(self):
        if activeMdiChild() and activeMdiChild().saveAs():
            activeMdiChild().saveAs()
#endif


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
 */
void MainWindow::newPlaylist()
{
    auto wins = createMdiChildPlaylist();
	MDIPlaylistView* child = wins.first;
	QMdiSubWindow* mdisubwindow = wins.second;

    child->newFile();

    setActiveSubWindow(mdisubwindow);
    statusBar()->showMessage(QString("Opened new Playlist '%1'").arg(child->windowTitle()));

    child->show();
}

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
	emit sendToNowPlaying(libentry);
}

std::pair<MDIPlaylistView*, QMdiSubWindow*> MainWindow::createMdiChildPlaylist()
{
	// Create a new playlist model.
	auto new_playlist_model = new PlaylistModel(this);
	m_playlist_models.push_back(new_playlist_model);

	MDIPlaylistView* child = new MDIPlaylistView(this);
	child->setModel(new_playlist_model);
	auto mdisubwindow = m_mdi_area->addSubWindow(child);

	// child.undoAvailable.connect(editUndoAct.setEnabled)
	// child.redoAvailable.connect(redoAct.setEnabled)
	// child.copyAvailable.connect(cutAct.setEnabled)
	// child.copyAvailable.connect(copyAct.setEnabled)

	// Connect signals.
	//child.cursorPositionChanged.connect(cursorPosChanged)

	// Add the new playlist to the collection doc widget.
	m_libraryDockWidget->addPlaylist(new PlaylistItem(child));
	return std::make_pair(child, mdisubwindow);
}

std::pair<MDINowPlayingView*, QMdiSubWindow*> MainWindow::createMdiNowPlayingView()
{
	// Create a new "Now Playing" playlist model.
	auto new_playlist_model = new PlaylistModel(this);

	// TODO REMOVE
	m_playlist_models.push_back(new_playlist_model);
	m_now_playing_playlist_model = new_playlist_model;

	MDINowPlayingView* child = new MDINowPlayingView(this);
	child->setModel(new_playlist_model);
	auto mdisubwindow = m_mdi_area->addSubWindow(child);

	// child.undoAvailable.connect(editUndoAct.setEnabled)
	// child.redoAvailable.connect(redoAct.setEnabled)
	// child.copyAvailable.connect(cutAct.setEnabled)
	// child.copyAvailable.connect(copyAct.setEnabled)

	// Connect signals.
	//child.cursorPositionChanged.connect(cursorPosChanged)

	// Add the new playlist to the collection doc widget.
	m_libraryDockWidget->addPlaylist(new PlaylistItem(child));
	return std::make_pair(child, mdisubwindow);
}

// Top-level "saveAs" action handler for "Save playlist as..."
void MainWindow::savePlaylistAs()
{
	auto child = activeMdiChild();
	if(child != nullptr)
	{
		MDIPlaylistView* playlist_ptr= qobject_cast<MDIPlaylistView*>(child);
		if(playlist_ptr != nullptr && playlist_ptr->saveAs())
		{
			statusBar()->showMessage("Playlist saved", 2000);
		}
	}
}

void MainWindow::startSettingsDialog()
{
	m_settings_dlg = QSharedPointer<SettingsDialog>(new SettingsDialog(this, this->windowFlags()), &QObject::deleteLater);

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

void MainWindow::subWindowActivated(QMdiSubWindow *subwindow)
{
    if(subwindow != nullptr)
    {
        //libmodel.rowsInserted.connect(onRowsInserted)
		updateConnections();
		updateMenus();
    }
}

void MainWindow::onStatusSignal(LibState state,  qint64 current, qint64 max)
{
	M_WARNING("TODO: Fix this")
#if 0
	if(state == LibState::ScanningForFiles)
	{
		m_actProgIndicator->setRange(0, max);
	}
	else if(state == LibState::PopulatingMetadata)
	{
		m_actProgIndicator->setRange(0, max);
		m_actProgIndicator->setValue(current);
	}

	// get summary stats over all libraries.
	qint64 num_songs = 0;
	for(auto libmodel : m_libmodels)
	{
		num_songs += libmodel->rowCount();
	}
	m_numSongsIndicator->setText(QString("Number of Songs: %1").arg(num_songs));
#endif
}

#if 0

    @pyqtSlot()
    def metaDataChanged(self):
		qDebug() << QString("MAIN GOT MESSAGE")
        //all_metadata = player.player.availableMetaData()
        current_window_title = "{} - {}".format(player.player.metaData(QMediaMetaData.AlbumArtist),
                               player.player.metaData(QMediaMetaData.Title))
        setWindowTitle(current_window_title)

#endif
