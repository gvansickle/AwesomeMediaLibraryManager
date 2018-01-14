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

#include "Experimental.h"
#include "MDILibraryView.h"
#include "MDIPlaylistView.h"
#include "MDITreeViewBase.h"
#include "PlayerControls.h"
#include "MDINowPlayingView.h"

#include <QMainWindow>
#include <QUrl>

#include <vector>
#include <utility> // For std::pair<>

#include <logic/LibraryModel.h>
#include <logic/PlaylistModel.h>
#include <logic/MP2.h>
#include <gui/settings/SettingsDialog.h>
#include "mdi/MDIModelViewPair.h"

class QActionGroup;
class QWidget;
class QLabel;
class QMdiSubWindow;
class MDIArea;
class QSettings;

class MDITreeViewBase;
class MDILibraryView;
class MDIPlaylistView;
class MetadataDockWidget;
class CollectionDockWidget;
class ActivityProgressWidget;
class ActionBundle;

class MainWindow: public QMainWindow
{
    Q_OBJECT

signals:
    void sendToNowPlaying(std::shared_ptr<LibraryEntry>);


public:
    MainWindow(QWidget *parent = Q_NULLPTR, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MainWindow() override;

	/**
	 * Get a pointer to the MainWindow singleton.
	 */
	static MainWindow* getInstance();

public slots:

    /// Slot corresponding to the "Open Directory as new Library" action.
    /// This is ~= a "File->Open" action.
    void importLib();

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

    void onRescanLibrary();
    void startSettingsDialog();

    /// @name Edit action forwarders.
    /// @{
    void onCut();
    void onCopy();
    void onPaste();
    void onSelectAll();
    void onDelete();
    /// @}


    void about();


protected:
    void closeEvent(QCloseEvent* event) override;

protected slots:

    // Probably don't need anything here, since we probably won't be deriving from MainWindow.

private slots:
    void onSubWindowActivated(QMdiSubWindow* subwindow);
    void onFocusChanged(QWidget* old, QWidget* now);

    void changeStyle(const QString& styleName);
    void changeIconTheme(const QString& iconThemeName);

    /**
     * Slot FBO the Collection sidebar to bring the Library @a libmodel to the fore
     * and/or creat a new MDILibraryView for it if one doesn't exist.
     */
    void onShowLibrary(LibraryModel* libmodel);

    void onRemoveDirFromLibrary(LibraryModel* libmodel);

    void onPlayTrackNowSignal(QUrl url);
    void onSendEntryToPlaylist(std::shared_ptr<LibraryEntry> libentry, std::shared_ptr<PlaylistModel> playlist_model);
    void onSendToNowPlaying(std::shared_ptr<LibraryEntry> libentry);

    void doExperiment();

	void updateActionEnableStates();
    void updateActionEnableStates_Edit();
	
    void onChangeWindowMode(QAction* action);

    /// Filter slots.
    void onTextFilterChanged();

private:
    Q_DISABLE_COPY(MainWindow)

    void createActions();
    void createActionsEdit();
	void createActionsView();
    
    void createMenus();
    void createToolBars();
    void createStatusBar();
	void createDockWidgets();

    /// Equivalent of a "File->New" action for the Now Playing model/view.
    void newNowPlaying();

	void addChildMDIView(MDITreeViewBase* child);
	MDITreeViewBase* activeChildMDIView();
    
	/// @name Bulk Signal/Slot Connection management.
    ///@{
    void connectPlayerAndControls(MP2 *m_player, PlayerControls *m_controls);
    void connectPlayerAndPlaylistView(MP2 *m_player, MDIPlaylistView *playlist_view);
    void connectPlayerControlsAndPlaylistView(PlayerControls *m_controls, MDIPlaylistView *playlist_view);

    void connectLibraryToActivityProgressWidget(LibraryModel* lm, ActivityProgressWidget* apw);

    void connectLibraryViewAndMainWindow(MDILibraryView* lv);
    void connectNowPlayingViewAndMainWindow(MDIPlaylistView* plv);
    void connectActiveMDITreeViewBaseAndMetadataDock(MDITreeViewBase* viewbase, MetadataDockWidget* metadata_dock_widget);
    ///@}

    void stopAllBackgroundThreads();

    /// @name Persistency
    ///@{

    /// Reads the primary settings.
    void readSettings();
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
	MDIModelViewPair findSubWindowModelViewPair(QUrl url);
    QMdiSubWindow* findSubWindow(QUrl url);
    MDITreeViewBase* findSubWindowView(QUrl url);
    QWidget* findSubWindowWithWidget(QWidget* widget) const;
    
    MDILibraryView* createMdiChildLibraryView();

    /**
     * Called by importModel().
     * @param url
     * @return
     */
    QSharedPointer<LibraryModel> openLibraryModelOnUrl(QUrl url);
    void openMDILibraryViewOnModel(QSharedPointer<LibraryModel> libmodel);

    /// @}
    
    
    bool maybeSaveOnClose();


    /// @name Private data members.
    /// @{

    /// App-specific cache directory.
    QUrl m_cachedir;

    /// App-specific directory where persistent application data can be stored.  On Windows, this is the roaming, not local, path.
    QUrl m_appdatadir;

    /// The media player instance.
    MP2 m_player;

    /// Experimental "scratch" widget for doing development experiments.
    Experimental* m_experimental;

    /// The library models.
    std::vector<QSharedPointer<LibraryModel>> m_libmodels;

    /// The "Now Playing" playlist model and view.
	QSharedPointer<PlaylistModel> m_now_playing_playlist_model;
    QPointer<MDIPlaylistView> m_now_playing_playlist_view;

    /// The list of PlaylistModels.
    std::vector<PlaylistModel*> m_playlist_models;

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
    QAction* m_rescanLibraryAct;
    QAction* m_saveLibraryAsAct;
    QAction* m_removeDirFromLibrary;
    QAction* m_newPlaylistAct;
    QAction* m_openPlaylistAct;
    QAction* m_savePlaylistAct;
    QAction* m_exitAction;
    QAction* m_scanLibraryAction;
    QAction* m_settingsAct;
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
    QAction* m_closeAllAct;
    QAction* m_closeAct;
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
    QToolBar* m_fileToolBar;
    QToolBar* m_toolbar_edit;
    QToolBar* m_settingsToolBar;
    QToolBar* m_controlsToolbar;
    QToolBar* m_filterToolbar;

    /// Docks
    CollectionDockWidget* m_libraryDockWidget;
    MetadataDockWidget* m_metadataDockWidget;

    /// The Activity Progress Widget.
    ActivityProgressWidget* m_activity_progress_widget;

    /// The Settings (AKA Preferences, AKA Config) dialog.
    QSharedPointer<SettingsDialog> m_settings_dlg;
};

