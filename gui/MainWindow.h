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

#define HAVE_KF5 1 // @todo

#ifdef HAVE_KF5

#include <KMainWindow>

class KToolBar;
using ToolBarClass = KToolBar;

#else // !HAVE_KF5

#include <QMainWindow>

class QToolBar;
using ToolBarClass = QToolBar;

#endif


#include <QUrl>

#include <vector>
#include <utility> // For std::pair<>
#include <memory>

#include <logic/MP2.h>
#include "mdi/MDIModelViewPair.h"

class QActionGroup;
class QWidget;
class QLabel;
class QMdiSubWindow;
class MDIArea;
class QSettings;

class QStandardItem;
class QStandardItemModel;

class MDITreeViewBase;
class MDILibraryView;
class MDIPlaylistView;
class MetadataDockWidget;
class CollectionDockWidget;
class ActivityProgressWidget;
class ActionBundle;
class PlayerControls;
class MDINowPlayingView;
class Experimental;
class SettingsDialog;
class LibraryModel;
class PlaylistModel;

class LibraryEntry;
class LibraryEntryMimeData;

class MainWindow: public KMainWindow
{
	Q_OBJECT

	using BASE_CLASS = KMainWindow;

Q_SIGNALS:
	/**
	 * Signal which serves essentially as a repeater for other views which want to
	 * send one or more tracks to the "Now Playing" view.
	 */
	void sendToNowPlaying(LibraryEntryMimeData* mime_data);

	/// User changed the settings in the Settings dialog.
	void settingsChanged();

public:
    MainWindow(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MainWindow() override;

	/**
	 * Get a pointer to the MainWindow singleton.
	 */
	static MainWindow* getInstance();

	/**
	 * Called from the closeEvent() of views just before they accept the event.
	 * This widget should delete any references it is keeping to @a viewptr.
	 *
	 * @note This arrangement is racey, need to find a better way to manage this.
	 */
	void view_is_closing(MDITreeViewBase* viewptr, QAbstractItemModel* modelptr);

public Q_SLOTS:

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
    void closeEvent(QCloseEvent* event) override;

protected Q_SLOTS:

    // Probably don't need anything here, since we probably won't be deriving from MainWindow.

private Q_SLOTS:
    void onSubWindowActivated(QMdiSubWindow* subwindow);
    void onFocusChanged(QWidget* old, QWidget* now);

    void changeStyle(const QString& styleName);
    void changeIconTheme(const QString& iconThemeName);

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

    void onSettingsChanged();

    /// Filter slots.
    void onTextFilterChanged();

private:
    Q_DISABLE_COPY(MainWindow)

	/// @name Startup Initialization
	/// @{
    void createActions();
    void createActionsEdit();
	void createActionsView();
	void createActionsTools();

    void createMenus();
    void createToolBars();
    void createStatusBar();
	void createDockWidgets();
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

	void connectLibraryModelToActivityProgressWidget(LibraryModel* lm, ActivityProgressWidget* apw);

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

	bool queryClose() override;

    /// @name Persistency
    ///@{

    /// Reads the primary settings.
	void readPreGUISettings();
    void onStartup();
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

	/// @name Tools actions
	/// @{

	QAction* m_rescanLibraryAct;
	QAction* m_cancelRescanAct;
	QAction* m_settingsAct;
	QAction* m_act_shortcuts_dialog;
	/// @}

	/// View actions.
	/// @{
	QAction *m_act_lock_layout;
	QAction *m_act_reset_layout;
	ActionBundle* m_ab_docks;
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

    /// Help actions.
    QAction* m_helpAct;
    QAction* m_whatsThisAct;
    QAction* m_aboutAct;
    QAction* m_aboutQtAct;

    /// Experimental actions.
    QAction* m_experimentalAct;

    /// Menus
    QMenu* m_fileMenu;
    QMenu *m_menu_edit;
    QMenu* m_viewMenu;
    QMenu* m_toolsMenu;
    QMenu* m_menu_window;
    QMenu* m_helpMenu;

    /// Toolbars
    ToolBarClass* m_fileToolBar;
	ToolBarClass* m_toolbar_edit;
	ToolBarClass* m_settingsToolBar;
	ToolBarClass* m_controlsToolbar;
	ToolBarClass* m_filterToolbar;

    /// Docks
	CollectionDockWidget* m_collection_dock_widget;
    MetadataDockWidget* m_metadataDockWidget;

    /// The Activity Progress Widget.
    ActivityProgressWidget* m_activity_progress_widget;

    /// The Settings (AKA Preferences, AKA Config) dialog.
    QSharedPointer<SettingsDialog> m_settings_dlg;
};

