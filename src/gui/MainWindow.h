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

#include <config.h>

/// Std C++

#include <vector>
#include <utility> // For std::pair<>
#include <memory>

/// Qt5

#include <QUrl>
class QActionGroup;
class QWidget;
class QLabel;
class QMdiSubWindow;
class QSettings;
class QStandardItem;
class QStandardItemModel;

/// KF5

#if HAVE_KF501
#include <KMainWindow>
#include <KXmlGuiWindow>

class KJob;
namespace KIO
{
    class Job;
}
class KStatusBarJobTracker;
class KToolBar;
using ToolBarClass = KToolBar;

class KToggleAction;
class KToggleToolBarAction;
class KActionMenu;

#else // !HAVE_KF501

#include <QMainWindow>

class QToolBar;
using ToolBarClass = QToolBar;

#endif

/// Ours

#include <logic/MP2.h>
#include "mdi/MDIModelViewPair.h"

class MDIArea;
class MDITreeViewBase;
class MDILibraryView;
class MDIPlaylistView;
class MetadataDockWidget;
class CollectionDockWidget;
class ActivityProgressStatusBarTracker;
class ActionBundle;
class PlayerControls;
class MDINowPlayingView;
class Experimental;
class SettingsDialog;
class LibraryModel;
class PlaylistModel;

class LibraryEntry;
class LibraryEntryMimeData;

/**
 * Awesome Media Library Manager's MainWindow class.
 *
 * @note I suspect deriving from KXmlGuiWindow instead of KMainWindow, when I'm not using XML for the GUI,
 *       is going to bite me at some point (EDIT: == continuously).  But this gives me an actionCollection().  So there's that.
 */
class MainWindow: public KXmlGuiWindow ///KMainWindow
{
	Q_OBJECT

	using BASE_CLASS = KXmlGuiWindow;

Q_SIGNALS:
	/**
	 * Signal which serves essentially as a repeater for other views which want to
	 * send one or more tracks to the "Now Playing" view.
	 */
	void sendToNowPlaying(LibraryEntryMimeData* mime_data);

	/// User changed the settings in the Settings dialog.
	void settingsChanged();

public:
	/// Constructor
	explicit MainWindow(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
	/// Destructor
    ~MainWindow() override;

	/**
	 * Get a pointer to the MainWindow singleton.
	 */
	static QPointer<MainWindow> instance();

	/// Init function to offload all the init which used to be in the constructor.
	void init();

    /// Init function which is called after setupGUI() has been called.
    void post_setupGUI_init();

	/**
	 * Called from the closeEvent() of views just before they accept the event.
	 * This widget should delete any references it is keeping to @a viewptr.
	 *
	 * @note This arrangement is racey, need to find a better way to manage this.
	 */
	void view_is_closing(MDITreeViewBase* viewptr, QAbstractItemModel* modelptr);

	/**
	 * Helper function to add an action to the actionCollection() with a name.
	 * @note Inspired by similar functionality in Kdenlive's main window.
	 */
    void addAction(const QString& action_name, QAction* action);

    /**
     * Helper function for adding new dock widgets to the main window.
     * @note Inspired by similar functionality in Kdenlive's main window.
     */
    QDockWidget* addDock(const QString& title, const QString& object_name, QWidget* widget,
                         Qt::DockWidgetArea area = Qt::TopDockWidgetArea);

    ToolBarClass* addToolBar(const QString &win_title, const QString& object_name);

    /**
     * Get ptr to the ActivityProgressStatusBarTracker singleton.
     */
    static ActivityProgressStatusBarTracker* master_tracker_instance();

public Q_SLOTS:

    void onStartup();


    /// Slot corresponding to the "Open Directory as new Library" action.
    /// This is ~= a "File->Open" action.
    void importLib();

	void openFileLibrary(const QUrl& filename);

    /**
     * Open a new, empty playlist.
     * ~= "File->New".
     */
    void newPlaylist();

    /**
     * Open an existing playlist.
     * ~= "File->Open...".
     */
    void openPlaylist();

    void savePlaylistAs();

	void onCloseSubwindow();

    void onRescanLibrary();

	void onCancelRescan();

    void startSettingsDialog();

    void onOpenShortcutDlg();

    /// @name Edit action forwarders.
    /// @{
    void onCut();
    void onCopy();
    void onPaste();
    void onSelectAll();
    void onDelete();
    /// @}


    void about();

	/**
	 * Slot which forwards the param to the QMdiArea.
	 */
	void setActiveSubWindow(QMdiSubWindow* window);

protected:
	bool queryClose() override;
    void closeEvent(QCloseEvent* event) override;


protected Q_SLOTS:

    // Probably don't need anything here, since we probably won't be deriving from MainWindow.

private Q_SLOTS:
    void onSubWindowActivated(QMdiSubWindow* subwindow);
    void onFocusChanged(QWidget* old, QWidget* now);

    /// @name Theme/Style related slots.
    /// @{
    void changeStyle(const QString& styleName);
    void changeIconTheme(const QString& iconThemeName);
    void onChangeStyle(QAction* action);
	/// @}

    /**
	 * Slot FBO the Collection sidebar to bring the MDILibraryView associated with @a libmodel to the fore
     * and/or creat a new MDILibraryView for it if one doesn't exist.
     */
	void onShowLibrary(QPointer<LibraryModel> libmodel);

	/**
	 * Slot FBO the Collection sidebar to remove the specified model and any attached views.
	 */
	void onRemoveDirFromLibrary(QPointer<LibraryModel> libmodel);

	void onSendEntryToPlaylist(std::shared_ptr<LibraryEntry> libentry, QPointer<PlaylistModel> playlist_model);
	void onSendToNowPlaying(LibraryEntryMimeData* mime_data);

    void doExperiment();

	void updateActionEnableStates();
    void updateActionEnableStates_Edit();

    void onChangeWindowMode(QAction* action);

	/// @name Settings action related slots.
	/// @{

	/// Slot which shows/hides the menu bar.
	void onShowMenuBar(bool show);

	void onConfigureToolbars();

	void onApplyToolbarConfig();

	void onSettingsChanged();

	/// @}

    /// Filter slots.
    void onTextFilterChanged();

private:
    Q_DISABLE_COPY(MainWindow)

	/// @name Startup Initialization
	/// @{
    void createActions();
	void createActionsEdit(KActionCollection *ac);
	void createActionsView(KActionCollection *ac);
	void createActionsTools(KActionCollection *ac);
	void createActionsSettings(KActionCollection *ac);
	void createActionsHelp(KActionCollection *ac);

    void createMenus();
    void createToolBars();
    void createStatusBar();
	void createDockWidgets();

    /**
     * Must only be called after createActions(), createMenus()/toolbars()/statusBar()/DockWidgets().
     */
    void addViewMenuActions();

    void initRootModels();
	/// @}

    /// Equivalent of a "File->New" action for the Now Playing model/view.
    void newNowPlaying();

	void addChildMDIView(MDITreeViewBase* child);
	void addChildMDIModelViewPair_Library(const MDIModelViewPair& mvpair);
	void addChildMDIModelViewPair_Playlist(const MDIModelViewPair& mvpair);
	MDITreeViewBase* activeChildMDIView();

	/// @name Bulk Signal/Slot Connection management.
    ///@{
    void connectPlayerAndControls(MP2 *m_player, PlayerControls *m_controls);
    void connectPlayerAndPlaylistView(MP2 *m_player, MDIPlaylistView *playlist_view);
    void connectPlayerControlsAndPlaylistView(PlayerControls *m_controls, MDIPlaylistView *playlist_view);

    void connectLibraryViewAndMainWindow(MDILibraryView* lv);
	void connectPlaylistViewAndMainWindow(MDIPlaylistView* plv);
	void connectNowPlayingViewAndMainWindow(MDINowPlayingView* now_playing_view);
    void connectActiveMDITreeViewBaseAndMetadataDock(MDITreeViewBase* viewbase, MetadataDockWidget* metadata_dock_widget);
    ///@}

    void stopAllBackgroundThreads();

	/// @name Session management.
	/// @{

	/// Override it if you need to save other data about your documents on session end.
	/// sessionConfig is a config to which that data should be saved. Normally, you don't need this function.
	/// But if you want to save data about your documents that are not in opened windows you might need it.
	void saveGlobalProperties(KConfig *sessionConfig) override {}

	/// Save instance-specific properties.
	/// https://api.kde.org/frameworks/kxmlgui/html/classKMainWindow.html
	/// "Invoked when the session manager requests your application to save its state."
	/// - Not called when the user closes the app normally.
	/// - No user interactions (dialogs) allowed in here.
	void saveProperties(KConfigGroup& config_group) override;

	/// Read instance-specific properties.
	/// Called indirectly by restore().
	void readProperties(const KConfigGroup& config_group) override;
	/// @}


    /// @name Persistency
    ///@{

    /// Reads the primary settings.
	void readPreGUISettings();
    void openWindows();
    void writeSettings();
    void writeLibSettings(QSettings& settings);
    void readLibSettings(QSettings& settings);
    ///@}

    /// Signal-Slot-related functions.
    void createConnections();
    void updateConnections();

    /// MDI-related functions.
    /// @{
	MDIModelViewPair findSubWindowModelViewPair(QUrl url) const;
	QMdiSubWindow* findSubWindow(QUrl url) const;
	MDITreeViewBase* findSubWindowView(QUrl url) const;
    /// @}


    bool maybeSaveOnClose();

	void doChangeStyle();


	/// @name Private data members.
    /// @{

    /// App-specific cache directory.
    QUrl m_cachedir;

    /// App-specific directory where persistent application data can be stored.  On Windows, this is the roaming, not local, path.
    QUrl m_appdatadir;

    /// The media player instance.
	MP2* m_player;

    /// Experimental "scratch" widget for doing development experiments.
    Experimental* m_experimental;

	/// The "model of models", used for the collection dock widget.
	QPointer<QStandardItemModel> m_model_of_model_view_pairs;
	QStandardItem* m_stditem_libraries;
	QStandardItem* m_stditem_playlist_views;

    /// The library models.
	std::vector<QPointer<LibraryModel>> m_libmodels;

    /// The "Now Playing" playlist model and view.
	QPointer<PlaylistModel> m_now_playing_playlist_model;
    QPointer<MDIPlaylistView> m_now_playing_playlist_view;

    /// The list of PlaylistModels.
	std::vector<QPointer<PlaylistModel>> m_playlist_models;

    /// @}

    /// The player controls widget.
    PlayerControls* m_controls;
    QLabel* m_numSongsIndicator;

    /// The MDI area.
    MDIArea* m_mdi_area;

    /// Actions

    /// @name File actions
    /// @{
    QAction* m_importLibAct;
    QAction* m_saveLibraryAsAct;
    QAction* m_removeDirFromLibrary;
    QAction* m_newPlaylistAct;
    QAction* m_openPlaylistAct;
    QAction* m_savePlaylistAct;
    QAction* m_exitAction;
    QAction* m_scanLibraryAction;
    /// @}

    /// @name Edit actions.
    /// @{
public:
	ActionBundle* m_ab_cut_copy_paste_actions;
	ActionBundle* m_ab_extended_edit_actions;
private:
    QAction *m_act_cut;
    QAction *m_act_copy;
    QAction *m_act_paste;
    QAction *m_act_delete;
    QAction *m_act_select_all;
    /// @}

	/// View actions.
	/// @{
	QAction *m_act_lock_layout;
	QAction *m_act_reset_layout;
	KToggleAction* m_act_ktog_show_menu_bar;
	KToggleToolBarAction* m_act_ktog_show_tool_bar;
	KToggleAction* m_act_ktog_show_status_bar;
	ActionBundle* m_ab_docks;
	QActionGroup* m_actgroup_styles;
	/// @}

	/// @name Tools actions
	/// @{
	QAction* m_rescanLibraryAct;
	QAction* m_cancelRescanAct;
	/// @}

	/// @name Settings actions.
	/// @{
	/// Widget style action menu.
	KActionMenu* m_act_styles_kaction_menu;
	QAction* m_act_shortcuts_dialog;
	QAction* m_act_settings;
	QAction* m_act_config_toolbars;
	/// @}

    /// @name Window actions.
    /// @{
    QActionGroup *m_tabs_or_subwindows_group;
    QAction *m_tabs_act;
    QAction *m_subwins_act;
    QAction* m_windowNextAct;
    QAction* m_windowPrevAct;
    QAction* m_windowCascadeAct;
    QAction* m_windowTileAct;
    QAction* m_act_close_all;
    QAction* m_act_close;
    QActionGroup* m_act_group_window;
    QAction* m_act_window_list_separator;
    /// @}

	/// @name Help actions.
#if !HAVE_KF501
	QAction* m_helpAct;
    QAction* m_whatsThisAct;
    QAction* m_aboutAct;
    QAction* m_aboutQtAct;
#endif

    /// Experimental actions.
    QAction* m_experimentalAct;

    /// Menus
	QMenu* m_menu_file;
	QMenu* m_menu_edit;
	QMenu* m_menu_view;
	QMenu* m_menu_tools;
	QMenu* m_menu_settings;
    QMenu* m_menu_window;
	QMenu* m_menu_help;

    /// Toolbars
    ToolBarClass* m_fileToolBar;
	ToolBarClass* m_toolbar_edit;
	ToolBarClass* m_settingsToolBar;
	ToolBarClass* m_controlsToolbar;
	ToolBarClass* m_filterToolbar;

public:
    /// Docks
	CollectionDockWidget* m_collection_dock_widget;
    MetadataDockWidget* m_metadataDockWidget;

private:

    /// The MainWindow signleton.
    static QPointer<MainWindow> m_instance;

#if HAVE_KF501
    /**
     * Master Tracker for all asynchronous activites.
     * Its widget is the progress bar in the status bar.
     * Probably belongs in AMLMApp, but constructor needs a QWidget parent.
     */
    ActivityProgressStatusBarTracker* m_activity_progress_tracker { nullptr };
#endif

    /// The Settings (AKA Preferences, AKA Config) dialog.
    QSharedPointer<SettingsDialog> m_settings_dlg;
};

