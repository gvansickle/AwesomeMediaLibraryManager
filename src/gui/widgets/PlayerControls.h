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

#ifndef PLAYERCONTROLS_H
#define PLAYERCONTROLS_H

#include <QWidget>
#include <QMediaPlayer>
#include <QIcon>
#include <QtCore/QPointer>

#if HAVE_QXTGLOBALSHORTCUT
#include <qxtglobalshortcut.h>
#endif

class QToolButton;
class QSlider;
class QLabel;
class QShortcut;


class PlayerControls : public QWidget
{
    Q_OBJECT

public:
	explicit PlayerControls(QWidget *parent = nullptr);

    QMediaPlayer::PlaybackState state() const;
	bool isMuted() const { return m_playerMuted; }
    float volume() const;

Q_SIGNALS:
    void play();
    void pause();
    void stop();
    void next();
    void previous();
	void changeShuffle(bool);
	void changeRepeat(bool);
	void changeVolume(float);
    void changeMuting(bool);
	void positionSliderMoved(int);

public Q_SLOTS:
    void setPlaybackState(QMediaPlayer::PlaybackState state);
    void setVolume(float volume);
    void setMuted(bool muted);
	void onDurationChanged(qint64 duration);
	void onPositionChanged(qint64 pos);

private Q_SLOTS:
    void onPlayAction();
	void onPauseAction();
	void onTogglePlayPauseAction();
    void muteClicked();


private:
	Q_DISABLE_COPY(PlayerControls)

	QMediaPlayer::PlaybackState m_playerState = QMediaPlayer::StoppedState;
	bool m_playerMuted = false;
	bool m_playerRepeat = false;

	/// State info.
	qint64 m_last_pos = 0;
	qint64 m_last_dur = 0;

    /// ToolButtons
	QToolButton* m_playButton;
	QToolButton* m_stopButton;
	QToolButton* m_nextButton;
	QToolButton* m_previousButton;
	QToolButton* m_shuffleButton;
	QToolButton* m_repeatButton;
	QToolButton* m_muteButton;

    /// Icons.
    QIcon m_icon_play;
    QIcon m_icon_pause;
    QIcon m_icon_stop;
    QIcon m_icon_skip_fwd;
    QIcon m_icon_skip_back;
	QIcon m_repeat_icon;
    QIcon m_icon_muted;
	QIcon m_icon_not_muted;

	/// Actions.
	QAction* m_play_act;
	QAction* m_pause_act;
	QAction* m_play_pause_toggle_act;
	QAction* m_stop_act;
	QAction* m_skip_fwd_act;
	QAction* m_skip_back_act;
	QAction* m_shuffleAct;
	QAction* m_repeat_act;
	QAction* m_mute_act;

	QSlider* m_positionSlider;
	QSlider* m_volumeSlider;
	QLabel* m_labelDuration;

#if HAVE_QXTGLOBALSHORTCUT
	/// Global Media Key shortcuts.
	QPointer<QxtGlobalShortcut> m_media_key_play_gshortcut;
	QPointer<QxtGlobalShortcut> m_media_key_pause_gshortcut;
	QPointer<QxtGlobalShortcut> m_media_key_toggle_play_pause_gshortcut;
	QPointer<QxtGlobalShortcut> m_media_key_stop_gshortcut;
	QPointer<QxtGlobalShortcut> m_media_key_next_gshortcut;
	QPointer<QxtGlobalShortcut> m_media_key_prev_gshortcut;
	QPointer<QxtGlobalShortcut> m_media_key_mute_gshortcut;
	QPointer<QxtGlobalShortcut> m_media_key_toggle_shuffle;
#endif

	void registerMediaKeySequences();

    void updateDurationInfo(qint64 pos, qint64 duration);
};

#endif // PLAYERCONTROLS_H
