/*
 * Copyright 2017, 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
/// @file

#include <config.h>

#include "MainWindow.h"

// Std C++
#include <functional>
#include <algorithm>
#include <vector>

// Qt
#include <QObject>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QLayout>
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
#include <QMdiSubWindow>
#include <QTimer>
#include <QMessageBox>
#include <QComboBox>
#include <QStyleFactory>
#include <QDirIterator>
#include <QClipboard>
#include <QSharedPointer>
#include <QStandardItem>
#include <QThread>
#include <QWhatsThis>
#include <QMimeData>
#include <QTableView>
#include <QProgressDialog>

// KF
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
#include <KJob>
#include <KIO/Job>
#include <KIO/JobTracker>
#include <KJobWidgets>
#include <KActionCollection>
#include <KF6/KIconWidgets/KIconButton>

// Ours
#include "AMLMApp.h"
#include <src/gui/actions/StandardActions.h>
#include "Experimental.h"
#include "FilterWidget.h"

#include "MDITreeViewBase.h"
#include "MDILibraryView.h"
#include "MDIPlaylistView.h"
#include "MDINowPlayingView.h"

// For KF KConfig infrastructure.
#include <AMLMSettings.h>
#include <gui/actions/ActionHelpers.h>
#include <gui/settings/SettingsDialog.h>
#include <gui/widgets/CollectionStatsWidget.h>
#include <gui/widgets/CollectionView.h>

#include <logic/models/LibraryModel.h>
#include <logic/models/PlaylistModel.h>
#include <proxymodels/ShuffleProxyModel.h>

#include "gui/MDIArea.h"
#include "MetadataDockWidget.h"
#include "CollectionDockWidget.h"
#include "widgets/PlayerControls.h"

#include "logic/models/LibraryEntryMimeData.h"

#include "utils/ConnectHelpers.h"
#include "utils/DebugHelpers.h"

#include <logic/MP2.h>
#include "Theme.h"
#include "logic/models/LibraryEntryMimeData.h"

#include "AboutBox.h"
#include "logic/proxymodels/ModelChangeWatcher.h"

#include <gui/menus/ActionBundle.h>
#include <gui/menus/HelpMenu.h>

// Asynchronous activity progress monitoring.
#include <gui/activityprogressmanager/ActivityProgressStatusBarTracker.h>
#include <logic/proxymodels/LibrarySortFilterProxyModel.h>
#include <logic/serialization/XmlSerializer.h>

#include <utils/Stopwatch.h>

/// @note EXPERIMENTAL
#include <gui/widgets/ExperimentalKDEView1.h>
#include <Core.h>
#include <DebugSequence.h>


//
// Note: The MDI portions of this file are very roughly based on the Qt MDI example,
// the MDI editor example here: http://www.informit.com/articles/article.aspx?p=1405543&seqNum=6, and countless
// other variations on the theme, with my own adaptations liberally applied throughout.
//

/// Singleton pointer.
QPointer<MainWindow> MainWindow::m_instance { nullptr };


MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags) : BASE_CLASS(parent, flags)
{
	// Name our MainWindow.
	setObjectName("MainWindow");

    // Name our GUI thread.
    QThread::currentThread()->setObjectName("GUIThread");
    qDebug() << "Current thread:" << QThread::currentThread()->objectName();

    // Set the singleton pointer.
    m_instance = this;


    /// @note Per what Kdenlive is doing.
    /// Call tree there is ~:
    /// main()
    ///   Core::build()
    ///     Core::initialize()
    ///       m_main_win = new MainWindow();
    ///     m_main_win->init()
    ///       // Then a bit of a mix.
    ///       setCentralWidget()
    ///       setupActions()
    ///       create some docks
    ///       create more actions
    ///       create some menus
    ///       setupGUI(Toolbar|StatusBar|Save|Create)
    ///       create more menus
    ///     ^^->show();
    ///   app.exec()

    // Do further init() in a separate function, but which we can still do in the constructor call.
    init();
}

MainWindow::~MainWindow()
{
// M_WARNING("LOOKS LIKE WE'RE HANGING HERE");
    // KDev's MainWindow does only this here:
    /**
     * if (memberList().count() == 1) {
        // We're closing down...
        Core::self()->shutdown();
        }

        delete d;
     */

#if 1
    AMLMApp::instance()->SLOT_onAboutToQuit();
#else
    // Shouldn't have been destroyed until now.
    Q_CHECK_PTR(instance()->m_activity_progress_tracker);
    delete instance()->m_activity_progress_tracker;
    instance()->m_activity_progress_tracker = nullptr;
#endif

	Q_CHECK_PTR(m_player);
	delete m_player;

	delete m_actionCollection;

    m_instance = nullptr;
}

QPointer<MainWindow> MainWindow::instance()
{
    return m_instance;
}

KActionCollection* MainWindow::actionCollection()
{
    if(!m_actionCollection)
    {
        m_actionCollection = new KActionCollection(this);
    	m_actionCollection->setObjectName("MainWindow_KActionCollection");
    }
    return m_actionCollection;
}


void MainWindow::createStandardStatusBarAction()
{
	// This is per the same function in KXmlGuiWindow.
	if (!m_act_ktog_show_status_bar)
	{
		m_act_ktog_show_status_bar = KStandardAction::showStatusbar(this, &KMainWindow::setSettingsDirty,
																	actionCollection());
		QStatusBar* sb = statusBar();
		connect_or_die(m_act_ktog_show_status_bar, &QAction::toggled, sb, &QStatusBar::setVisible);
		m_act_ktog_show_status_bar->setChecked(sb->isHidden());
	}
	else
	{
		// Per KXmlGuiWindow, we could get here on a language change.
		auto tmpStatusbar = KStandardAction::showStatusbar(nullptr, nullptr, nullptr);
		m_act_ktog_show_status_bar->setText(tmpStatusbar->text());
		/// @todo Qt6 has obsoleted "What's This", prob don't need this.
		m_act_ktog_show_status_bar->setWhatsThis(tmpStatusbar->whatsThis());
		delete tmpStatusbar;
	}
}

void MainWindow::setStandardToolBarMenuEnabled(bool showToolBarMenu)
{
	/// @todo The KXmlGui* code for this looks substantial.
	/// @sa https://api.kde.org/legacy/3.5-api/kdelibs-apidocs/kdeui/html/ktoolbarhandler_8cpp_source.html#l00127
	// if (showToolBarMenu)
	// {
	// }
	qDb() << "Empty setStandardToolBarMenuEnabled() called";
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

    /// @note Don't know if we need this or not.  Krita does this in its main window constructor.
    actionCollection()->addAssociatedWidget(this);

	readPreGUISettings();

	// Follow the system style for the Icon&/|Text setting for toolbar buttons.
	setToolButtonStyle(Qt::ToolButtonFollowStyle);

	// Set up our Theme/Style management and actions.
	Theme::initialize();
    m_actgroup_styles = Theme::getWidgetStylesActionGroup(this);
	m_act_styles_kaction_menu = qobject_cast<KActionMenu*>(m_actgroup_styles->parent());
	Q_CHECK_PTR(m_act_styles_kaction_menu);




	/// Set "document mode" for the tab bar of tabbed dock widgets.
	setDocumentMode(true);
	setAnimated(true);
	setDockNestingEnabled(true);

    /// @todo ifdef this to development only.
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
	createToolBars();
	createStatusBar();
	createDockWidgets();
	/// @note Temporary move, should really be before createToolBars().
	createMenus();

	updateActionEnableStates();

	// Set up the style GUI elements if we need to.
	if (!AMLMSettings::widgetStyle().isEmpty()
		&& QString::compare(QApplication::style()->objectName(), AMLMSettings::widgetStyle(), Qt::CaseInsensitive) != 0)
	{
		// Initialize the different style.
		SLOT_setApplicationStyle(AMLMSettings::widgetStyle());
	}

	////// Connect up signals and slots.
	createConnections();

	setWindowTitle(amlmApp->applicationDisplayName());

	setUnifiedTitleAndToolBarOnMac(true);

	// Send ourself a message to re-load the files we had open last time we were closed.
	QTimer::singleShot(0, this, &MainWindow::onStartup);
}

void MainWindow::post_setupGUI_init()
{
    // Post setupGUI(), we can now add the status/tool/dock actions.
	addViewMenuActions();

	// KF: Activate Autosave of toolbar/menubar/statusbar/window layout settings.
	// "Make sure you call this after all your *bars have been created."
	// @note In KXmlGuiWindow this is done by setupGUI().
	setAutoSaveSettings();

	onSetLayoutLocked(AMLMSettings::layoutIsLocked());
}

void MainWindow::applyMainWindowSettings(const KConfigGroup& config)
{
	KMainWindow::applyMainWindowSettings(config);
	QStatusBar* sb = findChild<QStatusBar*>();
	if (sb && m_act_ktog_show_status_bar)
	{
		m_act_ktog_show_status_bar->setChecked(sb->isHidden());
	}
}

/**
 * Called by a "timer(0)" started in the constructor.
 * Does some final setup work which should be done once the constructor has finished and the event loop has started.
 */
void MainWindow::onStartup()
{
    initRootModels();

    // Create the "Now Playing" playlist model and view.
    newNowPlaying();

    /// @experimental
    // Create a new Collection view.
	/// @note This is atm the view onto the AMLMDatabase model.
    newCollectionView();

    // Load any files which were opened at the time the last session was closed.
    qInfo() << "Loading libraries open at end of last session...";
    readLibSettings();

    // Open the windows the user had open at the end of last session.
    openWindows();

    // KDE
    // Don't need to do this when using setupGUI(StatusBar).
//	createStandardStatusBarAction();
    // Don't need to do this when using setupGUI(ToolBar).
//	setStandardToolBarMenuEnabled(true);

    // Create the master job tracker singleton.
    // https://api.kde.org/frameworks/kjobwidgets/html/classKStatusBarJobTracker.html
    // parent: "the parent of this object and of the widget displaying the job progresses"
// M_WARNING("Q: Don't know if statusBar() is the correct parent here.  Need this before initRootModels() etc in onStartup?");
    auto sb = statusBar();
    Q_CHECK_PTR(sb);
    m_activity_progress_tracker = new ActivityProgressStatusBarTracker(sb);
    statusBar()->addPermanentWidget(m_activity_progress_tracker->get_status_bar_widget());

    // Set up the GUI from the ui.rc file embedded in the app's QResource system.
//	setupGUI(KXmlGuiWindow::Default, ":/kxmlgui5/AwesomeMediaLibraryManagerui.rc");
    // No Create, we're going to try not using the XML file above.
    // No ToolBar, because even though we have toolbars, adding that flag causes crashes somewhere
    //   in a context menu and when opening the KEditToolBar dialog.
    //   Without it, we seem to lose no functionality, but the crashes are gone.
// M_WARNING("Crashing here on Windows");
#if 0 // debug, no KXmlGui.
    setupGUI(KXmlGuiWindow:: Create | Keys | StatusBar | ToolBar | Save,
    	":/kxmlgui6/AwesomeMediaLibraryManager/AwesomeMediaLibraryManagerui.rc");
#elif 0
	setupGUI(KXmlGuiWindow::/*Create |*/ Keys | StatusBar | /*ToolBar |*/ Save,
		":/kxmlgui6/AwesomeMediaLibraryManager/AwesomeMediaLibraryManagerui.rc");
#else
	/// @sa KXmlGuiWindow::setupGui().
	createStandardStatusBarAction();
	setStandardToolBarMenuEnabled(true);
#endif
	// Debug check
	qDebug() << "Toolbar valid:" << toolBar();
	qDebug() << "Configure action exists:" << actionCollection()->action("options_configure_toolbars");
	// qDebug() << "All actions:" << actionCollection()->actions();

	post_setupGUI_init();
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
    // IMPORTANT SAFETY TIP: Per this: https://web.fe.up.pt/~jmcruz/etc/kde/kdeqt/kde3arch/xmlgui.html
    // "KDE's class for toplevel windows, KMainWindow, inherits KXMLGUIClient and therefore supports XMLGUI
    // out of the box. All actions created within it must have the client's actionCollection() as parent."

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
	addAction("import_library", m_importLibAct);

	m_saveLibraryAsAct = make_action(QIcon::fromTheme("folder-close"), "&Save library as...", this);
	addAction("save_library_as", m_saveLibraryAsAct);

	m_removeDirFromLibrary = make_action(QIcon::fromTheme("edit-delete"), "Remove &Dir from library...", this);
	addAction("remove_dir_from_library", m_removeDirFromLibrary);

    ////// Playlist actions.
	m_newPlaylistAct = make_action(QIcon::fromTheme("document-new"), "&New playlist...", this,
                                  QKeySequence::New,
                                  "Create a new playlist");
	connect_trig(m_newPlaylistAct, this, &MainWindow::newPlaylist);
	addAction("new_playlist", m_newPlaylistAct);

	m_openPlaylistAct = make_action(QIcon::fromTheme("document-open"), "&Open playlist...", this,
                                QKeySequence::Open,
                                "Open an existing playlist");
	addAction("open_playlist", m_openPlaylistAct);

	m_savePlaylistAct = make_action(QIcon::fromTheme("document-save"), "&Save playlist as...", this,
                                   QKeySequence::Save);

	connect_trig(m_savePlaylistAct, this, &MainWindow::savePlaylistAs);
	addAction("save_playlist_as", m_savePlaylistAct);

#if !(HAVE_KF501 || HAVE_KF6)
	m_exitAction = make_action(QIcon::fromTheme("application-exit"), "E&xit", this,
                              QKeySequence::Quit,
                              "Exit application");
	connect_trig(m_exitAction, this, &MainWindow::close);
#else
    m_exitAction = KStandardAction::quit(this, &MainWindow::close, ac);
#endif
    addAction("file_quit", m_exitAction);

	//
	// Edit actions.
	//
	createActionsEdit(ac);

	//
	// View actions.
	//
	createActionsView(ac);

	createActionsTools(ac);

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
	// setStandardToolBarMenuEnabled(true);

	// Provides a menu entry that allows showing/hiding the statusbar
	// createStandardStatusBarAction();

	// Standard 'Configure' menu actions
	// createSettingsActions();
/// @end
}

void MainWindow::createActionsEdit(KActionCollection *ac)
{
	// The cut/copy/paste action "sub-bundle".
    m_ab_cut_copy_paste_actions = new ActionBundle(ac);
#if !(HAVE_KF501 || HAVE_KF6)
	// Specifying the ActionBundle as each QAction's parent automatically adds it to the bundle.
	m_act_cut = make_action(Theme::iconFromTheme("edit-cut"), tr("Cu&t"), m_ab_cut_copy_paste_actions, QKeySequence::Cut,
                                                    tr("Cut the current selection to the clipboard"));
	m_act_copy = make_action(Theme::iconFromTheme("edit-copy"), tr("&Copy"), m_ab_cut_copy_paste_actions, QKeySequence::Copy,
													 tr("Copy the current selection to the clipboard"));
	m_act_paste = make_action(Theme::iconFromTheme("edit-paste"), tr("&Paste"), m_ab_cut_copy_paste_actions, QKeySequence::Paste,
													  tr("Paste the clipboard's contents into the current selection"));
#else

	m_act_cut = KStandardAction::cut(m_ab_cut_copy_paste_actions);
	m_act_copy = KStandardAction::copy(m_ab_cut_copy_paste_actions);
	m_act_paste = KStandardAction::paste(m_ab_cut_copy_paste_actions);

#endif

//    ac->addAction("cut_copy_paste", static_cast<QActionGroup*>(m_ab_cut_copy_paste_actions));

	connect_trig(m_act_cut, this, &MainWindow::onCut);
	addAction("edit_cut", m_act_cut);

    connect_trig(m_act_copy, this, &MainWindow::onCopy);
	addAction("edit_copy", m_act_copy);

	connect_trig(m_act_paste, this, &MainWindow::onPaste);
	addAction("edit_paste", m_act_paste);

	// The action bundle containing the other edit actions.
    m_ab_extended_edit_actions = new ActionBundle(ac);

	m_ab_extended_edit_actions->addSection(tr("Delete"));

	m_act_delete = make_action(Theme::iconFromTheme("edit-delete"), tr("&Delete"), m_ab_extended_edit_actions, QKeySequence::Delete,
                                                       tr("Delete this entry"));
    connect_trig(m_act_delete, this, &MainWindow::onDelete);
	addAction("edit_delete", m_act_delete);

	m_ab_extended_edit_actions->addSection(tr("Selections"));

	m_act_select_all = make_action(Theme::iconFromTheme("edit-select-all"), tr("Select &All"), m_ab_extended_edit_actions,
                                                               QKeySequence::SelectAll, tr("Select all items in the current list"));
	connect_trig(m_act_select_all, this, &MainWindow::onSelectAll);
	addAction("edit_select_all", m_act_select_all);

    // Find
//    m_ab_find_actions = new ActionBundle(ac);
    m_act_find = StandardActions::find(this, &MainWindow::SLOT_find, ac);
    m_act_find_next = StandardActions::findNext(this, &MainWindow::SLOT_find_next, ac);
    m_act_find_prev = StandardActions::findPrev(this, &MainWindow::SLOT_find_prev, ac);
}

void MainWindow::createActionsView(KActionCollection *ac)
{
	// View actions.

    m_ab_docks = new ActionBundle(ac);

#if !(HAVE_KF501 || HAVE_KF6)
	m_act_lock_layout = make_action(Theme::iconFromTheme("emblem-locked"), tr("Lock layout"), this); // There's also an "emblem-unlocked"
	m_act_reset_layout = make_action(Theme::iconFromTheme("view-multiple-objects"), tr("Reset layout"), this);
#else
	// There's also "emblem-locked", "emblem-unlocked", and "system-lock-screen".
	// KF uses the latter in its KToolBar context menu.
	m_act_lock_layout = make_action(Theme::iconFromTheme("system-lock-screen"), tr("Lock toolbar positions"), ac);
	m_act_lock_layout->setCheckable(true);
	m_act_lock_layout->setChecked(KToolBar::toolBarsLocked());
	m_act_reset_layout = make_action(Theme::iconFromTheme("view-multiple-objects"), tr("Reset layout"), ac);
#endif
	/// @todo These appear to be unreparentable, so we can't give them to an ActionBundle.
//	m_ab_docks->addAction(m_libraryDockWidget->toggleViewAction());
//	m_ab_docks->addAction(m_metadataDockWidget->toggleViewAction());
// KF5 WAS THIS: m_act_ktog_show_tool_bar = new KToggleToolBarAction("FileToolbar", tr("Show File Toolbar"), ac);
// KF6 This is broken:	m_act_ktog_show_tool_bar = new KToggleToolBarAction(m_fileToolBar, /*"FileToolbar",*/ tr("Show File Toolbar"), this);
}

void MainWindow::createActionsTools(KActionCollection *ac)
{
	//
	// Tools actions.
	//

	m_rescanLibraryAct = make_action(QIcon::fromTheme("view-refresh"), tr("&Rescan libray..."), this,
									QKeySequence::Refresh);
	connect_trig(m_rescanLibraryAct, this, &MainWindow::onRescanLibrary);
	addAction("rescan_library", m_rescanLibraryAct);

	m_cancelRescanAct = make_action(Theme::iconFromTheme("process-stop"), tr("Cancel Rescan"), this,
									QKeySequence::Cancel);
	connect_trig(m_cancelRescanAct, this, &MainWindow::onCancelRescan);
    addAction("cancel_rescan", m_cancelRescanAct);

	m_scanLibraryAction = make_action(QIcon::fromTheme("tools-check-spelling"), "Scan library", this,
							   QKeySequence(), "Scan library for problems");
    addAction("scan_lib", m_scanLibraryAction);
}

void MainWindow::createActionsSettings(KActionCollection *ac)
{
#if HAVE_KF501 || HAVE_KF6

	// QStyles KActionMenu menu.
	addAction(QStringLiteral("styles_menu"), m_act_styles_kaction_menu);
    connect_or_die(m_actgroup_styles, &QActionGroup::triggered,
    	this, [this](QAction* action) {
    		const QString styleName = action->data().toString();
			SLOT_setApplicationStyle(styleName);
    });

	// Show/hide menu bar.
	m_act_ktog_show_menu_bar = KStandardAction::showMenubar(this, &MainWindow::onShowMenuBar, ac);

	// Open the shortcut configuration dialog.
	m_act_shortcuts_dialog = KStandardAction::keyBindings(this, &MainWindow::onOpenShortcutDlg, ac);

	// Open the application preferences dialog.
	m_act_settings = KStandardAction::preferences(this, &MainWindow::startSettingsDialog, ac);

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
#if HAVE_KF501 || HAVE_KF6
	// For KDE we use a derivation of KHelpMenu.
    Q_UNUSED(ac)
#else
	m_helpAct = make_action(Theme::iconFromTheme("help-contents"), tr("&Help"), this,
							QKeySequence::HelpContents,
							"Show help contents");
	m_helpAct->setDisabled(true); /// @todo No help yet.
	// Qt5 has a pre-fabbed "What's This" action which handles everything, we don't need to even add a handler or an icon.
	// Qt6 is deprecating "What's this".
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

/**
 * @note This will only work properly if called after all Toolbars and Docks have been added
 * and the equivalent of setupGUI() has been called.
 */
void MainWindow::addViewMenuActions()
{
	// Layout lock.
	m_act_lock_layout->setChecked(AMLMSettings::layoutIsLocked());
	connect_or_die(m_act_lock_layout, &QAction::toggled, this, &MainWindow::onSetLayoutLocked);
	m_menu_view->addActions({
    	m_act_lock_layout,
		m_act_reset_layout,
		m_act_ktog_show_tool_bar});

    // List of dock widgets.
    m_menu_view->addSection(tr("Docks"));
    QList<QDockWidget*> dockwidgets = findChildren<QDockWidget*>();
    // qDb() << "Docks:" << dockwidgets;
    for(auto dock : std::as_const(dockwidgets))
    {
        // qDb() << "Dock:" << dock;
        if(dock->parentWidget() == this)
        {
            if(dock->toggleViewAction() == nullptr)
            {
                qWr() << "NULL TOGGLEVIEWACTION";
            }
            else
            {
                m_menu_view->addAction(dock->toggleViewAction());
            }
        }
    }

	// List of toolbars.
    m_menu_view->addSection(tr("Toolbars"));
    auto tbs = toolBars();
    for(auto tb : std::as_const(tbs))
    {
        auto action = tb->toggleViewAction();
        m_menu_view->addAction(action);
    }

	// Reset layout.
	/// @todo ???

}

void MainWindow::createMenus()
{
	m_menu_file = menuBar()->addMenu(tr("&File"));

	m_menu_file->addActions({//newFileAct,
						  m_menu_file->addSection("Libraries"),
						  m_importLibAct,
						  m_saveLibraryAsAct,
						  m_menu_file->addSection("Playlists"),
						  m_newPlaylistAct,
						  m_openPlaylistAct,
						  m_savePlaylistAct,
                          //saveAsAct,
						  m_menu_file->addSeparator(),
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
	m_menu_edit->addAction(m_act_find);
	m_menu_edit->addAction(m_act_find_next);
	m_menu_edit->addAction(m_act_find_prev);

    // Create the View menu.
    m_menu_view = menuBar()->addMenu(tr("&View"));
	// menu populated by addViewMenuActions().
//	menuBar()->addMenu(m_menu_view);
//	m_ab_docks->appendToMenu(m_menu_view);
//	m_menu_view->addActions({
//							   m_act_lock_layout,
//							   m_act_reset_layout,
//							   m_act_ktog_show_tool_bar,
//						   });

    // Tools menu.
	m_menu_tools = menuBar()->addMenu(tr("&Tools"));
	m_menu_tools->addActions(
        {//scanLibraryAction,
		 m_menu_tools->addSection("Rescans"),
		 m_rescanLibraryAct,
		 m_cancelRescanAct,
                });

	// Settings menu.
	m_menu_settings = menuBar()->addMenu(tr("&Settings"));
	m_menu_settings->addActions({
		m_act_ktog_show_menu_bar,
		m_menu_tools->addSeparator(),
		m_act_styles_kaction_menu,
		m_menu_tools->addSeparator(),
		m_act_shortcuts_dialog,
		m_act_config_toolbars,
		m_act_settings
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
#if !(HAVE_KF501 || HAVE_KF6)
	m_helpMenu = menuBar()->addMenu("&Help");
	m_helpMenu->addSection("Help");
	m_helpMenu->addAction(m_helpAct);
	m_helpMenu->addAction(m_whatsThisAct);
	m_helpMenu->addSection("About");
	m_helpMenu->addAction(m_aboutAct);
	m_helpMenu->addAction(m_aboutQtAct);
#else
    // Create a help menu based on KF KHelpMenu.
	auto help_menu = new HelpMenu(this, KAboutData::applicationData());
	menuBar()->addMenu(help_menu->menu());
#endif // !HAVE_KF5
}


void MainWindow::createToolBars()
{
	//
	// File
	//
    m_fileToolBar = addToolBar(tr("File"), "mainToolBar");

    Q_ASSERT(m_fileToolBar->parent() == this);

	m_fileToolBar->addActions({m_importLibAct,
	                           m_rescanLibraryAct,
							   m_cancelRescanAct,
							 m_fileToolBar->addSeparator(),
							 m_newPlaylistAct,
							 m_openPlaylistAct,
							 m_savePlaylistAct});

    const auto& actionlist = m_fileToolBar->actions();
    for(const auto& a : actionlist)
    {
        auto cold = a->associatedObjects().count();
        auto cnew = a->associatedWidgets().count();
        Q_ASSERT(cold == cnew);
        if(a->associatedWidgets().empty())
        {
            qWr() << "File toolbar action" << a << "has no associatedWidgets()";
        }
    }

	//
	// Edit
	//
    m_toolbar_edit = addToolBar(tr("Edit"), "EditToolbar");

    // Only add the cut/copy/paste subset of actions to the toolbar.
	m_ab_cut_copy_paste_actions->appendToToolBar(m_toolbar_edit);

	//
	// Settings
	//
    m_settingsToolBar = addToolBar(tr("Settings"), "SettingsToolbar");

	m_settingsToolBar->addAction(m_act_settings);
    m_settingsToolBar->addSeparator();
	m_settingsToolBar->addAction(m_experimentalAct);
    /// KF5 button that opens an Icon select dialog.
// @todo This doesn't link now for some reason:	 m_settingsToolBar->addWidget(new KIconButton(m_settingsToolBar));

#if HAVE_KF501 || HAVE_KF6
    // Create a combo box where the user can change the application's style.
	m_combobox_style = new QComboBox;
	m_combobox_style->setObjectName("kcfg_widget_style");
	m_combobox_style->addItem("Default");
	m_combobox_style->addItems(QStyleFactory::keys());
    // Set it to the current style.
	QString cur_style = amlmApp->style()->objectName();
	m_combobox_style->setCurrentIndex(m_combobox_style->findText(cur_style, Qt::MatchFixedString));
	m_settingsToolBar->addWidget(m_combobox_style);

	connect_or_die(m_combobox_style, &QComboBox::currentTextChanged, this, &MainWindow::SLOT_setApplicationStyle);

    // Create a combo box with icon themes.
    QComboBox* iconThemeComboBox = new QComboBox;
    iconThemeComboBox->addItems(Theme::GetIconThemeNames());
    m_settingsToolBar->addWidget(iconThemeComboBox);
	connect_or_die(iconThemeComboBox, &QComboBox::currentTextChanged, this, &MainWindow::changeIconTheme);
#endif
    // Create a toolbar for the player controls.
    m_controlsToolbar = addToolBar(tr("Player Controls"), "PlayerControlsToolbar");

    m_controlsToolbar->addWidget(m_controls);

	// Create a toolbar for filtering
    m_filterToolbar = addToolBar(tr("Filter bar"), "FilterToolbar");

	FilterWidget* fw = new FilterWidget;
	auto filterPatternLabel = new QLabel(tr("&Filter pattern:"));
	m_filterToolbar->addWidget(filterPatternLabel);
	m_filterToolbar->addWidget(fw);
	filterPatternLabel->setBuddy(fw);
}

void MainWindow::createStatusBar()
{
#ifdef HAVE_KF5
    // https://api.kde.org/frameworks/kjobwidgets/html/classKStatusBarJobTracker.html
    // parent: "the parent of this object and of the widget displaying the job progresses"
//    m_kf5_activity_progress_widget = new KStatusBarJobTracker(this, /*display cancel button*/ true);
//    KIO::setJobTracker(m_kf5_activity_progress_widget);
//    m_kf5_activity_progress_widget->setStatusBarMode(KStatusBarJobTracker::LabelOnly);
//    statusBar()->addPermanentWidget(m_kf5_activity_progress_widget);
#endif

//	m_activity_progress_widget = new ActivityProgressWidget(this);

//    statusBar()->addPermanentWidget(m_activity_progress_widget);

    statusBar()->showMessage("Ready");
}

void MainWindow::createDockWidgets()
{
#if HAVE_KF501 || HAVE_KF6
    auto dock_parent = actionCollection();
#else
    auto dock_parent = this;
#endif
    // Create the Library/Playlist dock widget.
    m_collection_dock_widget = new CollectionDockWidget(tr("Media Sources"), this);
	addDockWidget(Qt::LeftDockWidgetArea, m_collection_dock_widget);

    // Create the Collection Stats dock widget.
//    m_collection_stats_widget = new CollectionStatsWidget();
//    m_collection_stats_dock_widget = m_collection_stats_widget->make_dockwidget(tr("Collection Stats"), this);
	 m_collection_stats_dock_widget = CollectionStatsWidget::make_dockwidget(tr("Collection Stats"), this);
    m_collection_stats_dock_widget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//    m_collection_stats_dock_widget->setWidget(m_collection_stats_widget);
    addDockWidget(Qt::LeftDockWidgetArea, m_collection_stats_dock_widget);

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
	connect_or_die(amlmApp, &QApplication::focusChanged, this, &MainWindow::onFocusChanged);

    // Connect player controls up to player.
	connectPlayerAndControls(m_player, m_controls);

    // Connect with the CollectionDockWidget.
	connect_or_die(m_collection_dock_widget, &CollectionDockWidget::showLibraryModelSignal, this, &MainWindow::onShowLibrary);
	connect_or_die(m_collection_dock_widget, &CollectionDockWidget::removeLibModelFromLibSignal, this, &MainWindow::onRemoveDirFromLibrary);
	connect_or_die(m_collection_dock_widget, &CollectionDockWidget::activateSubwindow, this, &MainWindow::setActiveSubWindow);

	// Connect FilterWidget signals
	connect_or_die(m_filterToolbar->findChild<FilterWidget*>(), &FilterWidget::filterChanged, this, &MainWindow::onTextFilterChanged);

	updateConnections();
}

void MainWindow::connectPlayerAndControls(MP2 *player, PlayerControls *controls)
{
	// PlayerControls -> MP2 signals.
	connect_or_die(controls, &PlayerControls::play, player, &MP2::play);
	connect_or_die(controls, &PlayerControls::pause, player, &MP2::pause);
	connect_or_die(controls, &PlayerControls::stop, player, &MP2::stop);
	/// See connectPlayerControlsAndNowPlayingView(), this might not actually do anything.
	connect_or_die(controls, &PlayerControls::changeRepeat, player, &MP2::repeat);
	connect_or_die(controls, &PlayerControls::changeMuting, player, &MP2::setMuted);
	connect_or_die(controls, &PlayerControls::changeVolume, player, &MP2::setVolume);
	connect_or_die(controls, &PlayerControls::changeShuffle, player, &MP2::setShuffleMode);
    connect_or_die(controls, &PlayerControls::positionSliderMoved, player, &MP2::seek);
	connect_or_die(controls, &PlayerControls::posSliderPressed, player, &MP2::seekStart);
	connect_or_die(controls, &PlayerControls::posSliderReleased, player, &MP2::seekEnd);

	// MP2 -> PlayerControls signals.
    connect_or_die(player, &MP2::playbackStateChanged, controls, &PlayerControls::setPlaybackState);
	connect_or_die(player, &MP2::mutedChanged, controls, &PlayerControls::setMuted);
	connect_or_die(player, &MP2::volumeChanged, controls, &PlayerControls::setVolume);
	connect_or_die(player, &MP2::durationChanged2, controls, &PlayerControls::onDurationChanged);
	connect_or_die(player, &MP2::positionChanged2, controls, &PlayerControls::onPositionChanged);

	// Final setup.
	// Set control states to their initial states.
	controls->setVolume(player->volume());
	controls->setMuted(player->muted());
}

/// @todo RENAME
void MainWindow::connectPlayerAndNowPlayingView(MP2 *player, MDINowPlayingView *now_playing_view)
{
	// Connection for the player to tell the playlist to go to the next item when the current one is over.
	// This in turn will cause a MDINowPlayingView::nowPlayingIndexChanged() to be emitted, which the player
	// receives and sets up the now-current track.
	connect_or_die(player, &MP2::playlistToNext, m_now_playing_shuffle_proxy_model, &ShuffleProxyModel::next);
	connect_or_die(m_now_playing_shuffle_proxy_model, &ShuffleProxyModel::nowPlayingIndexChanged, player, &MP2::onPlaylistPositionChanged);
}

/// @todo RENAME
void MainWindow::connectPlayerControlsAndNowPlayingView(PlayerControls* controls, MDINowPlayingView* now_playing_view)
{
	/// @note Qt::ConnectionType() cast here is due to the mixed flag/enum nature of the type.  Qt::UniqueConnection (0x80) can be bitwise-
	/// OR-ed in with any other connection type, which are 0,1,2,3.
    connect_or_die(controls, &PlayerControls::next, m_now_playing_shuffle_proxy_model, &ShuffleProxyModel::next, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
    connect_or_die(controls, &PlayerControls::previous, m_now_playing_shuffle_proxy_model, &ShuffleProxyModel::previous, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
	connect_or_die(controls, &PlayerControls::changeShuffle, m_now_playing_shuffle_proxy_model, &ShuffleProxyModel::setShuffle);
	connect_or_die(controls, &PlayerControls::changeRepeat, m_now_playing_shuffle_proxy_model, &ShuffleProxyModel::setLoopAtEnd);

	// Set control states to their initial states.
	controls->setShuffleMode(m_now_playing_shuffle_proxy_model->shuffle());
	controls->setRepeatMode(m_now_playing_shuffle_proxy_model->loopAtEnd());

	// Connect play() signal-to-signal.
	connect_or_die(now_playing_view, &MDINowPlayingView::play, controls, &PlayerControls::play, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
}

void MainWindow::connectLibraryViewAndMainWindow(MDILibraryView *lv)
{
    qDebug() << "Connecting" << lv << "and" << this;
	connect_or_die(lv, &MDILibraryView::sendEntryToPlaylist, this, &MainWindow::onSendEntryToPlaylist);
	connect_or_die(lv, &MDILibraryView::sendToNowPlaying, this, &MainWindow::onSendToNowPlaying);

	connect_or_die(this, &MainWindow::settingsChanged, lv, &MDILibraryView::onSettingsChanged);
}

void MainWindow::connectPlaylistViewAndMainWindow(MDIPlaylistView* plv)
{
	qDebug() << "TODO Connecting";
// M_WARNING("TODO: connect");
	qDebug() << "TODO Connected";
}

void MainWindow::connectNowPlayingViewAndMainWindow(MDINowPlayingView* now_playing_view)
{
    qDebug() << "Connecting";
	connect_or_die(this, &MainWindow::sendToNowPlaying, now_playing_view, &MDINowPlayingView::onSendToNowPlaying);
	connect_or_die(this, &MainWindow::settingsChanged, now_playing_view, &MDILibraryView::onSettingsChanged);

    /// @todo Break this off, it's not NowPlaying<->MainWindow.
	connect_or_die(m_now_playing_playlist_model, &QAbstractItemModel::modelReset, m_now_playing_shuffle_proxy_model,
					&ShuffleProxyModel::onNumRowsChanged);
	connect_or_die(m_now_playing_playlist_model, &QAbstractItemModel::rowsInserted, m_now_playing_shuffle_proxy_model,
					&ShuffleProxyModel::onNumRowsChanged);
	connect_or_die(m_now_playing_playlist_model, &QAbstractItemModel::rowsRemoved, m_now_playing_shuffle_proxy_model,
					&ShuffleProxyModel::onNumRowsChanged);

    // ShuffleProxyModel -> Now Playing View
    connect_or_die(m_now_playing_shuffle_proxy_model, &ShuffleProxyModel::nowPlayingIndexChanged, now_playing_view, &MDINowPlayingView::setCurrentIndexAndRow);

    // Now Playing View -> ShuffleProxyModel
    connect_or_die(m_now_playing_playlist_view, &MDINowPlayingView::activated, m_now_playing_shuffle_proxy_model, &ShuffleProxyModel::jump);


	connectPlayerAndNowPlayingView(m_player, now_playing_view);
	connectPlayerControlsAndNowPlayingView(m_controls, now_playing_view);
	qDebug() << "Connected";
}

void MainWindow::connectActiveMDITreeViewBaseAndMetadataDock(MDITreeViewBase* viewbase,
															MetadataDockWidget* metadata_dock_widget)
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

    auto swl = m_mdi_area->subWindowList();

	for(const auto& child : std::as_const(swl))
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
	if(!failures.empty())
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
	/// @note Per KF docs:
	/// https://api.kde.org/frameworks/kxmlgui/html/classKMainWindow.html#a433d7bee79191266ca4ae335464922d5
	/// "We recommend that you reimplement queryClose() rather than closeEvent(). If you do it anyway,
	/// ensure to call the base implementation to keep the feature of auto-saving window settings working."
	qDebug() << QString("Main Window received closeEvent.");

    KMainWindow::closeEvent(event);

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
    const auto &subwinlist = m_mdi_area->subWindowList();
    for(const auto& window : subwinlist)
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
        // Get the QMdiSubWindow's widget.
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
		for (const auto& lm : m_libmodels)
		{
			if (lm->getView() == view)
			{
				// Found the pair.
				return *lm;
			}
		}

		for (const auto& pm : m_playlist_models)
		{
			if (pm->getView() == view)
			{
				// Found the pair.
				return *pm;
			}
		}

		// If we get here, we didn't find the view in the MDIModelViewPair stores.
		// Something is broken.
		Q_ASSERT(false);
    }
	else
	{
		// No existing View, is there an existing Model open?
		/// @todo: Seems like there should be a cleaner way to handle this.
		for(const auto& lm : m_libmodels)
		{
            if(qobject_cast<LibraryModel*>(lm->getRootModel())->getLibRootDir() == url)
			{
                qDebug() << "Found existing LibraryModel:" << lm->getRootModel().data();
                retval = *lm;
                retval.m_view_was_existing = false;
				retval.m_model_was_existing = true;
			}
		}
		for(const auto& pm : m_playlist_models)
		{
			if(qobject_cast<PlaylistModel*>(pm->getRootModel())->getLibRootDir() == url)
			{
                qDebug() << "Found existing PlaylistModel:" << pm;
                retval = *pm;
				retval.m_view_was_existing = false;
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

		for(auto i : std::as_const(indexes_to_delete))
		{
			m_model_of_model_view_pairs->removeRow(i.row(), parentindex);
		}

		//m_model_of_model_view_pairs->indexFromItem()

        std::erase_if(m_playlist_models, [modelptr](auto& mp){ return modelptr == mp->getRootModel(); });
	}
}

void MainWindow::addAction(const QString& action_name, QAction* action)
{
	auto ac = actionCollection();

	ac->addAction(action_name, action);
    ac->setDefaultShortcut(action, action->shortcut());
}

ToolBarClass* MainWindow::addToolBar(const QString &win_title, const QString &object_name)
{
    // KMainWindow has a toolBar() factory function.  It takes a "name" string, however that is used as
    // the toolbar's objectName().  Need a windowTitle as well.
    auto retval = toolBar(object_name);
    retval->setWindowTitle(win_title);

    return retval;
}

ActivityProgressStatusBarTracker *MainWindow::master_tracker_instance()
{
    // Make sure it's been constructed.
    Q_ASSERT(instance()->m_activity_progress_tracker != nullptr);

    return instance()->m_activity_progress_tracker;
}

QDockWidget *MainWindow::addDock(const QString &title, const QString &object_name, QWidget *widget, Qt::DockWidgetArea area)
{
    auto retval = new QDockWidget(title, this);
    retval->setObjectName(object_name);
    retval->setWidget(widget);
    addDockWidget(area, retval);
    return retval;
}


//////
////// Top-level settings save/restore.
//////

void MainWindow::readPreGUISettings()
{
	// Open the config file.
	// No name == application name + "rc".
	// No type == QStandardPaths::GenericConfigLocation
	KConfigGroup config(KSharedConfig::openConfig(), "Settings");
	/// @todo Add any readEntry()'s here.
}

void MainWindow::readLibSettings()
{
	int num_libs;
	static DebugSequence dseq;

	qIn() << "START readLibSettings";

	// Throw up a progress dialog indicating that we're loading the database.
	auto* prog = new QProgressDialog(tr("Opening database..."), tr("Abort open"), 0, 0, this);

//	prog.setWindowModality(Qt::WindowModal);
	prog->setValue(1);
	prog->setValue(2);
	prog->show();

	// The primary database file.
	/// @todo Get this from settings.
	QString database_filename = QDir::homePath() + "/AMLMDatabase.xml";

	qIn() << "Loading" << database_filename;

	// Try to Load it asynchronously into a new model.
	/// AMLM::Core::self()->getDefaultColumnSpecs()
	auto exp_db_model = AMLM::Core::self()->getScanResultsTreeModel();
    bool success = exp_db_model->LoadDatabase(database_filename);
    if(success)
    {
		// qDb() << "Load succeeded";
	}
	else
	{
		qWr() << "Database load failed:" << database_filename;
//				auto default_columnspecs = AMLM::Core::self()->getDefaultColumnSpecs();
//				AMLM::Core::self()->getScanResultsTreeModel()->setColumnSpecs(default_columnspecs);
	}

	/// @todo The playlist
	/// @todo Get this path from settings.
	QString overlay_filename = QDir::homePath() + "/AMLMDatabaseSerDes.xml";

	dseq.expect_and_set(0,1);

    auto extfuture_initial_lib_load = QtConcurrent::run([=](QPromise<SerializableQVariantList>& ef) {

		qIn() << "READING XML DB FROM FILE:" << overlay_filename;
		dseq.expect_and_set(1,2);
		SerializableQVariantList list("library_list", "library_list_item");
		Stopwatch library_list_read(tostdstr(QString("Loading: ") + overlay_filename));
		XmlSerializer xmlser;
		xmlser.set_default_namespace("http://xspf.org/ns/0/", "1");
		/// @todo This takes ~10 secs at the moment with a 300MB XML file.
		bool success = xmlser.load(list, QUrl::fromLocalFile(overlay_filename));
    	qIn() << "Load of" << overlay_filename << "success: " << success;
        ef.addResult(list);
	})
    .then(this, [this, overlay_filename, prog](QFuture<SerializableQVariantList> ef){
    	dseq.expect_and_set(2, 3);
		if(!ef.isValid())
		{
			qWr() << "XML Deserialization failed:" << overlay_filename;
			return;
		}

		SerializableQVariantList list = ef.result();

        qIn() << "###### READ" << list.size() << "libraries from XML DB:" << overlay_filename;

		for(const auto& list_entry : list)
		{
			QVariant qv = list_entry;
			Q_ASSERT(qv.isValid());
			Q_ASSERT(!qv.isNull());

			// Create a new LibraryModel and read contents into it.
			QPointer<LibraryModel> library_model = new LibraryModel(this);
			{
				Stopwatch sw("library_model-from-variant");
				library_model->fromVariant(qv);
			}

			Q_ASSERT(library_model->getLibRootDir().isValid());

			if(!library_model)
			{
				QMessageBox::critical(this, qApp->applicationDisplayName(), tr("Failed to open library"),
									  QMessageBox::Ok);
			}
			else
			{
				MDIModelViewPair mvpair;
				mvpair.appendModel(library_model);
				mvpair.m_model_was_existing = false;

				addChildMDIModelViewPair_Library(mvpair);
                onShowLibrary(library_model);
			}
		}
		qIn() << "###### READ AND CONVERTED XML DB:" << overlay_filename;

		prog->hide();
		prog->deleteLater();
	});

	// Set extfuture_initial_lib_load to the PerfectDeleter.
	PerfectDeleter::instance().addExtFuture(extfuture_initial_lib_load);

	qIn() << "END readLibSettings";

}

void MainWindow::writeSettings()
{
	qDebug() << "writeSettings() start";
/// @todo REMOVE
//	settings.beginGroup("mainwindow");
//	settings.setValue("geometry", saveGeometry());
//	settings.setValue("window_state", saveState());
//	settings.endGroup();
	// Write the open library settings.
    writeLibSettings();
	qDebug() << "writeSettings() end";
}


void MainWindow::writeLibSettings()
{
	qDebug() << "writeLibSettings() start";

	Stopwatch libsave_sw("writeLibSettings()");

	QString database_filename = QDir::homePath() + "/AMLMDatabaseSerDes.xml";

	qIn() << "WRITING" << m_libmodels.size() << "libmodels to XML file:" << database_filename;

	XmlSerializer xmlser;
	xmlser.set_default_namespace("http://xspf.org/ns/0/", "1");

	SerializableQVariantList list("library_list", "library_list_item");
	for(size_t i = 0; i < m_libmodels.size(); ++i)
	{
		// m_libmodels are pointers to QObject-derived, we need to push into the list manually.
        LibraryModel* lmp = qobject_cast<LibraryModel*>(m_libmodels[i]->getRootModel());
		QVariant qv = lmp->toVariant();
		list.push_back(qv);
	}

	xmlser.save(list, QUrl::fromLocalFile(database_filename), "the_library_model_list");

	qIn() << "###### WROTE XML DB:" << database_filename;

	qDebug() << "writeLibSettings() end";
}




void MainWindow::openWindows()
{
	qInfo() << "Opening" << m_libmodels.size() << "windows which were open at end of last session...";

	for(const auto& m : m_libmodels)
	{
        auto mp = qobject_cast<LibraryModel*>(m->getRootModel());
        qDebug() << "Opening view on existing model:" << mp->getLibraryName() << mp->getLibRootDir();

        auto child = MDILibraryView::openModel(mp, this);

		/// @todo Should be encapsulated such that what we get back from openModel() is correct.
		child.m_model_was_existing = true;

        if(child.getView())
        {
			addChildMDIModelViewPair_Library(child);
        }
	}
}


//////
////// Action targets.
//////

/**
 * Top-level menu/toolbar action for creating a new Library model+view by picking a library root directory.
 * ~= "File->Open...".
 */
void MainWindow::importLib()
{
	auto check_for_existing_view = [this](QUrl url) -> MDIModelViewPair {
		auto mvpair = findSubWindowModelViewPair(url);
		return mvpair;
    };

    auto child = MDILibraryView::open(this, check_for_existing_view);
    if(child.getView())
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
    if(child.getView())
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
// M_WARNING("HACKISH, MAKE THIS BETTER");
/// @todo So really what we're doing is removing any libraries and re-opening them.

	QVector<QUrl> lib_root_urls;

	for(auto& l : m_libmodels)
	{
        auto lp = qobject_cast<LibraryModel*>(l->getRootModel());
//		lp->startRescan();
        lib_root_urls << lp->getLibRootDir();
        onRemoveDirFromLibrary(lp);
	}

	for(const auto& url : std::as_const(lib_root_urls))
	{
		openFileLibrary(url);
	}
}

void MainWindow::onCancelRescan()
{
	for(const auto& l : m_libmodels)
	{
        auto lp = qobject_cast<LibraryModel*>(l->getRootModel());
        lp->cancelRescan();
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
    qDebug() << QString("Removing libmodel from library:") << libmodel.data();

	// Find any open MDI Windows viewing this library.
	auto window = findSubWindow(libmodel->getLibRootDir());
	if(window)
	{
		qDebug() << "Found window:" << window;
		window->close();
	}
	// Find and Delete the model.
	size_t index = 0;
    for(auto& m : m_libmodels)
	{
        auto mp = qobject_cast<LibraryModel*>(m->getRootModel());
        if(mp == libmodel)
		{
            qDebug() << QString("Removing libmodel:") << m << ", have" << m_libmodels.size() << "model(s).";
            mp->close(true);
			m_libmodels.erase(m_libmodels.begin() + index);
			// Delete the model.
			// @todo I think the erase above will delete the model.
            // delete m;
			break;
		}
		index++;
	}
	qDebug() << QString("Num models:") << m_libmodels.size();

	// Write the library settings out now.
    // note that this writes the Library settings to ${HOME}/AMLMDatabaseSerDes.xml (not a QSettings or KConfig settings file).
	writeLibSettings();
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
    mvpair.appendModel(QPointer<PlaylistModel>(child->underlyingModel()));
    mvpair.m_model_was_existing = false;
    mvpair.appendView(child);
	mvpair.m_view_was_existing = false;

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

	m_now_playing_shuffle_proxy_model = new ShuffleProxyModel(this);
	m_now_playing_shuffle_proxy_model->setSourceModel(m_now_playing_playlist_model);
	m_now_playing_library_sortfilter_model = new LibrarySortFilterProxyModel(this);
	m_now_playing_library_sortfilter_model->setDynamicSortFilter(false);
	m_now_playing_library_sortfilter_model->setSortCaseSensitivity(Qt::CaseInsensitive);
	m_now_playing_library_sortfilter_model->setSourceModel(m_now_playing_shuffle_proxy_model);


	/// @todo Do we really need to keep this as a member pointer?
	m_now_playing_playlist_view = child;

	/// @todo Maybe refactor the "newFile()" setup to look more like the static openXxx() functions,
	/// so we always get an MDIModelViewPair here and don't need to hand-roll it.
	MDIModelViewPair mvpair;
	mvpair.appendModel(m_now_playing_playlist_model);
	mvpair.appendProxyModel(m_now_playing_shuffle_proxy_model);
    mvpair.appendProxyModel(m_now_playing_library_sortfilter_model);
    mvpair.appendView(m_now_playing_playlist_view);
	mvpair.m_view_was_existing = false;
	mvpair.m_model_was_existing = false;

	addChildMDIModelViewPair_Playlist(mvpair);

	connectNowPlayingViewAndMainWindow(child);

    statusBar()->showMessage(tr("Opened 'Now Playing' Playlist '%1'").arg(child->windowTitle()));
}

void MainWindow::newCollectionView()
{
    qDbo() << "Creating new CollectionView";
    auto child = new CollectionView(this);
    qDbo() << "Created new CollectionView:" << child;
    qDbo() << "Adding to mdi area";
    auto mdi_child = m_mdi_area->addSubWindow(child);
    Q_CHECK_PTR(mdi_child);

//    child->getTableView()->setModel(model);
//    child->setPane2Model(AMLMApp::instance()->cdb2_model_instance());
// M_WARNING("SHARED PTR")
//	child->setPane2Model(AMLMApp::instance()->IScanResultsTreeModel().get());
//	child->setPane2Model(AMLM::Core::self()->getScanResultsTreeModel().get());
	child->setPane2Model(AMLM::Core::self()->getEditableTreeModel().get());

	m_exp_second_child_view = new ExperimentalKDEView1(this);
	auto second_mdi_child = m_mdi_area->addSubWindow(m_exp_second_child_view);
// M_WARNING("SHARED PTR")
    auto srtmodel = AMLM::Core::self()->getScanResultsTreeModel().get();
	m_exp_second_child_view->setModel(srtmodel);

    mdi_child->show();
	second_mdi_child->show();
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
    qDebug() << "Sending entry to playlist:" << playlist_model.data();
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
	connect_or_die(child, &MDITreeViewBase::cutAvailable, m_act_cut, &QAction::setEnabled);
	connect_or_die(child, &MDITreeViewBase::cutAvailable, m_act_delete, &QAction::setEnabled);
	connect_or_die(child, &MDITreeViewBase::copyAvailable, m_act_copy, &QAction::setEnabled);
	connect_or_die(child, &MDITreeViewBase::selectAllAvailable, m_act_select_all, &QAction::setEnabled);

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
		auto libview = qobject_cast<MDILibraryView*>(mvpair.getView());

		Q_CHECK_PTR(libview);

		// Did a view of the URL the user specified already exist?
		if(mvpair.m_view_was_existing)
		{
			// View already existed, just activate its parent subwindow and we're done.
			qDebug() << "View already existed";
			m_mdi_area->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(mvpair.getView()->parent()));
		}
		else
		{
			// View is new.
			connectActiveMDITreeViewBaseAndMetadataDock(libview, m_metadataDockWidget);
			connectLibraryViewAndMainWindow(libview);

			// Add the view as a new MDI child.
			addChildMDIView(mvpair.getView());
		}
		statusBar()->showMessage(tr("Opened view on library '%1'").arg(libview->getDisplayName()));
	}

	if(mvpair.hasModel())
	{
        QPointer<LibraryModel> libmodel = qobject_cast<LibraryModel*>(mvpair.getRootModel());
		Q_CHECK_PTR(libmodel);

        bool model_really_already_existed = (std::find_if(m_libmodels.begin(), m_libmodels.end(), [libmodel](auto& lmvpair){
                                                 return lmvpair->getRootModel() == libmodel;
                                             }) != m_libmodels.end());

		// View is new, did the model already exist?
		if(mvpair.m_model_was_existing)
		{
            qDebug() << "Model existed:" << mvpair.getRootModel().data() << libmodel->getLibRootDir() << libmodel->getLibraryName();
			Q_ASSERT(model_really_already_existed);
		}
		else
		{
			// Model is new.
            qDebug() << "Model is new:" << mvpair.getRootModel().data() << libmodel->getLibRootDir() << libmodel->getLibraryName();
			Q_ASSERT(!model_really_already_existed);

            MDIModelViewPair* lmvpair(new MDIModelViewPair());
			*lmvpair = mvpair;
            m_libmodels.push_back(lmvpair);

            /// @todo This needs cleanup.
			dynamic_cast<CollectionStatsWidget*>(m_collection_stats_dock_widget->widget())->setModel(libmodel);

			// Add the new library to the ModelViewPairs Model.
			// The Collection Dock Widget uses this among others.
			QStandardItem* new_lib_row_item = new QStandardItem(libmodel->getLibraryName());
			new_lib_row_item->setData(QVariant::fromValue(libmodel));
			new_lib_row_item->setData(QIcon::fromTheme("folder"), Qt::DecorationRole);
            QString tttext = tr("<b>%1</b><hr>%2")
                                 .arg(libmodel->getLibraryName(),
                                      libmodel->getLibRootDir().toDisplayString());
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
        auto playlist_view = qobject_cast<MDIPlaylistView*>(mvpair.getView());

		Q_CHECK_PTR(playlist_view);

		// Did a view of the URL the user specified already exist?
		if(mvpair.m_view_was_existing)
		{
			// View already existed, just activate its parent subwindow and we're done.
			qDebug() << "View already existed";
            m_mdi_area->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(mvpair.getView()->parent()));
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
            QString tttext = tr("<b>%1</b><hr>%2").arg(playlist_view->getDisplayName(), playlist_view->windowFilePath());
			new_playlist_row_item->setData(QVariant(tttext), Qt::ToolTipRole);
			m_stditem_playlist_views->appendRow(new_playlist_row_item);
		}
		statusBar()->showMessage(tr("Opened view on playlist '%1'").arg(playlist_view->getDisplayName()));
	}

	if(mvpair.hasModel())
	{
        qDb() << "ROOT MODEL TYPE IS:" << mvpair.getRootModel()->metaObject()->className();

        auto proxy_model = qobject_cast<LibrarySortFilterProxyModel*>(mvpair.getTopModel());
        auto playlist_model = qobject_cast<PlaylistModel*>(proxy_model->sourceModel());
        Q_CHECK_PTR(playlist_model);

        bool model_really_already_existed = (std::find_if(m_playlist_models.begin(), m_playlist_models.end(), [playlist_model](auto& x){
                                                 return x->getRootModel() == playlist_model; }) != m_playlist_models.end());

		// View is new, did the model already exist?
		if(mvpair.m_model_was_existing)
		{
            qDebug() << "Model existed:" << mvpair.getRootModel()->objectName() << playlist_model->getLibRootDir() << playlist_model->getLibraryName();
			Q_ASSERT(model_really_already_existed);
		}
		else
		{
			// Model is new.
            qDebug() << "Model is new:" << mvpair.getRootModel()->objectName() << playlist_model->getLibRootDir() << playlist_model->getLibraryName();
			Q_ASSERT(!model_really_already_existed);

            auto lmvpair = new MDIModelViewPair();
            *lmvpair = mvpair;

			// Add the underlying model to the PlaylistModel list.
            m_playlist_models.push_back(lmvpair);
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

void MainWindow::SLOT_find()
{

}

void MainWindow::SLOT_find_next()
{

}

void MainWindow::SLOT_find_prev()
{

}

void MainWindow::startSettingsDialog()
{
//	KConfigDialog *dialog = KConfigDialog::exists( "settings" );
    auto dialog = SettingsDialog::exists("settings");
	if( !dialog )
	{
		// KConfigDialog didn't find an instance of this dialog, so lets create it:
		dialog = new SettingsDialog(this, "settings", AMLMSettings::self());

		connect_or_die(dialog, &KConfigDialog::settingsChanged, this, &MainWindow::onSettingsChanged);
	}
    dialog->show( /*page*/);
//    dialog->exec();

#if 0
	if(!m_settings_dlg)
	{
		// This is the first time anyone has opened the settings dialog.
		m_settings_dlg = QSharedPointer<SettingsDialog>(new SettingsDialog(this, "App Settings", Settings::self()), &QObject::deleteLater);
	}

	// Open the settings dialog modeless.
	// Note this from the Qt docs:
	// http://doc.qt.io/qt-6/qdialog.html
	// "If you invoke the show() function after hiding a dialog, the dialog will be displayed in its original position.
	// This is because the window manager decides the position for windows that have not been explicitly placed by the
	// programmer. To preserve the position of a dialog that has been moved by the user, save its position in your
	// closeEvent() handler and then move the dialog to that position, before showing it again."
	m_settings_dlg->show();
	m_settings_dlg->raise();
	m_settings_dlg->activateWindow();
#endif
}

void MainWindow::onOpenShortcutDlg()
{
	// Start the Keyboard Shorcut editor dialog.
//	KShortcutsDialog::configure(actionCollection(), KShortcutsEditor::LetterShortcutsDisallowed, this);
	KShortcutsDialog::showDialog(actionCollection(), KShortcutsEditor::LetterShortcutsDisallowed, this);

	AMLMSettings::self()->save();
}

void MainWindow::applyStyle(const QString& styleName)
{
	if(auto style = QStyleFactory::create(styleName))
	{
		QApplication::setStyle(style);
		m_currentStyle = styleName;
	}
	else
	{
		// styleName not available, fallback to user's KDE-global style (in a "kdeglobal" file).
		// If that style is somehow not available, fallback to Breeze.
		auto fallback_name = Theme::getUserDefaultQStyle("Breeze");
		qWarning() << "Style" << styleName << "not available, falling back to" << fallback_name;
		if (auto style = QStyleFactory::create(fallback_name))
		{
			QApplication::setStyle(style);
		}
	}
}

void MainWindow::updateStyleSelectionUi(const QString& style_name)
{
	// Set combo box selection.
	if (m_combobox_style)
	{
		QSignalBlocker blocker(m_combobox_style);
		m_combobox_style->setCurrentText(style_name);
	}
	// Update menu actions checked states.
	if (m_actgroup_styles)
	{
        for (QAction* action : m_actgroup_styles->actions())
		{
			qDb() << M_ID_VAL(action->data()) << M_ID_VAL(style_name);
			action->setChecked(action->data() == style_name);
		}
	}
}

void MainWindow::SLOT_setApplicationStyle(const QString& styleName)
{
	qDebug() << "Signaled to set Style to" << styleName;

	// Apply the style.
	applyStyle(styleName);
	// Set the new config setting.
	AMLMSettings::setWidgetStyle(styleName);
	// Update UI.
	updateStyleSelectionUi(styleName);
	// Write out the new setting.
	AMLMSettings::self()->save();
}

void MainWindow::changeIconTheme(const QString& iconThemeName)
{
	qDebug() << "signaled to set Icon Theme to" << iconThemeName;

    Theme::setIconThemeName(iconThemeName);

    for(auto& w : amlmApp->allWidgets())
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
// M_WARNING("TODO: CTRL+M doesn't get the bar back.");
//	menuBar()->setVisible(show);
}

void MainWindow::onSetLayoutLocked(bool checked)
{
	// Lock all toolbars.
	KToolBar::setToolBarsLocked(checked);

	// Lock DockWidgets.
	auto dockwidgetlist = this->findChildren<QDockWidget*>();
	for (auto* dockwidget : std::as_const(dockwidgetlist))
	{
		if (checked)
		{
			dockwidget->setFeatures(dockwidget->features().setFlag(QDockWidget::DockWidgetMovable, false));
		}
		else
		{
			dockwidget->setFeatures(dockwidget->features().setFlag(QDockWidget::DockWidgetMovable, true));
		}
	}
	// MainWindow::onSettingsChanged();
	// KToolBar::emitToolbarStyleChanged();
	onApplyToolbarConfig();

	AMLMSettings::setLayoutIsLocked(checked);
	AMLMSettings::self()->save();
}

// Slot
void MainWindow::onConfigureToolbars()
{
	// saveMainWindowSettings(config_group);

    /// @todo This probably need to go away, it's dependant on us being derived from KXmlGui.
    // KEditToolBar dialog(actionCollection(), this);

    // connect_or_die(&dialog, &KEditToolBar::newToolBarConfig, this, &MainWindow::onApplyToolbarConfig);

    // dialog.exec();
}

void MainWindow::onApplyToolbarConfig()
{
	auto config_group = KSharedConfig::openConfig()->group("MainWindow");

	applyMainWindowSettings(config_group);
}

void MainWindow::onSettingsChanged()
{
	qDb() << "SLOT: Settings changed";
    qDb() << "Toolbar Text/Icon:" << AMLMSettings::toolbarTextIconModeCombo();

    // Open up the config sections we need.
//    KSharedConfigPtr top_level_config = KSharedConfig::openConfig();

//    KConfigGroup config_grp_mainwindow(top_level_config, "MainWindow");
//    KConfigGroup config_grp_toolbars(&config_grp_mainwindow, "Toolbars");

    // Update all the toolbars.
    auto text_icon_mode = AMLMSettings::toolbarTextIconModeCombo();
    setToolButtonStyle(text_icon_mode);
    auto toolbars = toolBars();
    for(auto tb : std::as_const(toolbars))
    {
        qDb() << "Toolbar:" << tb->objectName();
        tb->setToolButtonStyle(text_icon_mode);
    }

	Q_EMIT settingsChanged();
}


void MainWindow::onTextFilterChanged()
{
	auto filterWidget = m_filterToolbar->findChild<FilterWidget*>();
	Q_ASSERT(filterWidget != nullptr);
    QRegularExpression regExp(filterWidget->text(), filterWidget->caseSensitive());

	auto mdisubwin = m_mdi_area->currentSubWindow();
	MDILibraryView* libtreeview = mdisubwin ? mdisubwin->findChild<MDILibraryView*>() : nullptr;
	if(libtreeview)
	{
        libtreeview->proxy_model()->setFilterRegularExpression(regExp);
	}
}


void MainWindow::stopAllBackgroundThreads()
{
	for(auto& model : m_libmodels)
	{
        qobject_cast<LibraryModel*>(model->getRootModel())->stopAllBackgroundThreads();
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



