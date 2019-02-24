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

#include "PlayerControls.h"

#include <QToolButton>
#include <QStyle>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QTime>
#include <QShortcut>
#include <QtCore/QPointer>

#if HAVE_QXTGLOBALSHORTCUT
#include <qxtglobalshortcut.h>
#endif

#include <gui/Theme.h>
#include "utils/ConnectHelpers.h"
#include <gui/actions/ActionHelpers.h>

PlayerControls::PlayerControls(QWidget *parent) : QWidget(parent)
{
	setObjectName("PlayerControlsWidget");

	// Play/pause button.
	m_icon_play = Theme::iconFromTheme("media-playback-start");
	m_icon_pause = Theme::iconFromTheme("media-playback-pause");
	m_play_act = new QAction(m_icon_play, tr("Play"), this);
	m_pause_act = new QAction(m_icon_pause, tr("Pause"), this);
	m_play_pause_toggle_act = new QAction(m_icon_pause, tr("Toggle Play/Pause"), this);
	m_playButton = new QToolButton(this);
	m_playButton->setDefaultAction(m_play_act);
	connect_trig(m_play_act, this, &PlayerControls::onPlayAction);
	connect_trig(m_pause_act, this, &PlayerControls::onPauseAction);
	connect_trig(m_play_pause_toggle_act, this, &PlayerControls::onTogglePlayPauseAction);

	// Stop button.
	m_stop_act = new QAction(Theme::iconFromTheme("media-playback-stop"), tr("Stop"), this);
	m_stopButton = new QToolButton(this);
	m_stopButton->setDefaultAction(m_stop_act);
	m_stop_act->setEnabled(false);
	connect_trig(m_stop_act, this, &PlayerControls::stop);

	// Next button.
	m_skip_fwd_act = new QAction(Theme::iconFromTheme("media-skip-forward"), tr("Next song"), this);
	m_nextButton = new QToolButton(this);
	m_nextButton->setDefaultAction(m_skip_fwd_act);
	connect_trig(m_skip_fwd_act, this, &PlayerControls::next);

	// Previous button.
	m_skip_back_act = new QAction(Theme::iconFromTheme("media-skip-backward"), tr("Previous song"), this);
	m_previousButton = new QToolButton(this);
	m_previousButton->setDefaultAction(m_skip_back_act);
	connect_trig(m_skip_back_act, this, &PlayerControls::previous);

    // Shuffle button.
	m_shuffleAct = new QAction(Theme::iconFromTheme("media-playlist-shuffle"), tr("Shuffle"), this);
	m_shuffleAct->setToolTip(tr("Shuffle mode"));
	m_shuffleAct->setStatusTip(tr("Toggle the shuffle mode of the currently playing playlist"));
	m_shuffleAct->setCheckable(true);
	m_shuffleButton = new QToolButton(this);
	m_shuffleButton->setDefaultAction(m_shuffleAct);
	connect(m_shuffleAct, &QAction::toggled, this, &PlayerControls::changeShuffle);

	m_repeatButton = new QToolButton(this);
	m_repeat_icon = Theme::iconFromTheme("media-playlist-repeat");
	m_repeat_act = make_action(m_repeat_icon, tr("Repeat"), this, QKeySequence(), "Repeat after last song in playlist is played");
	m_repeat_act->setCheckable(true);
	m_repeatButton->setDefaultAction(m_repeat_act);
	m_playerRepeat = false;
	// Signal-to-signal connection, emit a changeRepeat(bool) when the "Repeat" button changes checked state.
	connect(m_repeat_act, &QAction::toggled, this, &PlayerControls::changeRepeat);

	// Mute button.
	// Note a few things here:
	// - This is not a checkable action, just a normal stateless triggered()-sending button/action.
	// - The current mute state is sent back to us from the player.  We use this to switch between the
	//   muted and unmuted icons.
	m_icon_muted = QIcon::fromTheme("audio-volume-muted");
	m_icon_not_muted = QIcon::fromTheme("audio-volume-high");
	m_mute_act = new QAction(m_icon_not_muted, tr("Mute"), this);
	m_muteButton = new QToolButton(this);
	m_muteButton->setDefaultAction(m_mute_act);
	connect_trig(m_mute_act, this, &PlayerControls::muteClicked);

    // Position slider
	m_positionSlider = new QSlider(Qt::Horizontal);
    //slider->setRange(0, player.duration() / 1000)
    //slider->sliderMoved.connect(seek)

    // Time Remaining/Duration label.
	m_labelDuration = new QLabel(this);

    // Volume slider.
	m_volumeSlider = new QSlider(Qt::Horizontal);
	m_volumeSlider->setToolTip(tr("Volume"));
	m_volumeSlider->setRange(0, 100);
	// Signal-to-signal connection, emit a changeVolume(int) signal when the user moves the slider.
	connect(m_volumeSlider, &QSlider::sliderMoved, this, &PlayerControls::changeVolume);

    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_stopButton);
	layout->addWidget(m_previousButton);
	layout->addWidget(m_playButton);
	layout->addWidget(m_nextButton);
	layout->addWidget(m_shuffleButton);
	layout->addWidget(m_repeatButton);
	layout->addWidget(m_muteButton);
	layout->addWidget(m_positionSlider);
	layout->addWidget(m_labelDuration);
	layout->addWidget(m_volumeSlider);
    setLayout(layout);

    // State info.
	m_last_pos = 0;
	m_last_dur = 0;
    updateDurationInfo(0,0);

	// Set up support for media control keys.
	registerMediaKeySequences();
}

#if HAVE_QXTGLOBALSHORTCUT
static QPointer<QxtGlobalShortcut> make_QxtGlobalShortcut(const QKeySequence& key_seq, QAction* action_to_trigger, QObject *parent = nullptr)
{
	QPointer<QxtGlobalShortcut> retval;

	retval = new QxtGlobalShortcut(key_seq, parent);

	if(!retval || !retval->isValid())
	{
		qWarning() << "Failed to set global shortcut:" << key_seq;
	}

	QObject::connect(retval, &QxtGlobalShortcut::activated, [=](){ action_to_trigger->triggered(); });

	return retval;
}
#endif

void PlayerControls::registerMediaKeySequences()
{
#if 0 /// @todo Not sure what's happening here on Linux (Fedora 27), this takes over the whole keyboard.

	qDebug() << "Setting global shortcuts";

	/// @note It looks like QxtGlobalShortcut maps (on Windows) VK_MEDIA_PLAY_PAUSE to Qt::Key_MediaPlay, and nothing to
	/// Qt::Key_MediaTogglePlayPause.  I'm seeing what looks like the same thing on Fedora, though the creation of
	/// the Qt::Key_MediaTogglePlayPause shortcut below doesn't fail.
	/// For now we'll map Qt::Key_MediaPlay to the m_play_pause_toggle_act.
	m_media_key_play_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaPlay), /*m_play_act*/ m_play_pause_toggle_act, this);
	qDebug() << "Play:" << m_media_key_play_gshortcut;
	m_media_key_pause_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaPause), m_pause_act, this);
	qDebug() << "Pause:" << m_media_key_pause_gshortcut;
	m_media_key_toggle_play_pause_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaTogglePlayPause), m_play_pause_toggle_act, this);
	qDebug() << "PlayPauseToggle:" << m_media_key_toggle_play_pause_gshortcut;
	m_media_key_stop_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaStop), m_stop_act, this);
	m_media_key_next_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaNext), m_skip_fwd_act, this);
	m_media_key_prev_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaPrevious), m_skip_back_act, this);
	m_media_key_mute_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_VolumeMute), m_mute_act, this);

	/// @todo This doesn't appear to work.
	//m_media_key_toggle_shuffle = make_QxtGlobalShortcut(Theme::keySequenceFromTheme(Theme::Key_ToggleShuffle), m_shuffleAct, this);

	qDebug() << "Setting global shortcuts complete";

#endif
}

int PlayerControls::volume() const
{
	return m_volumeSlider->value();
}

/**
 * Slot connected to the player, which notifies us of its current state so we can set the control states appropriately.
 * @param state
 */
void PlayerControls::setState(QMediaPlayer::State state)
{
	qDebug() << "new state:" << state;

	m_playerState = state;

	// For removing the existing default action of the Play/Pause button.  See below.
	auto delete_old_action_lambda = [&]() {
		QAction* old_action = m_playButton->defaultAction();
		if(old_action != nullptr)
		{
			m_playButton->removeAction(old_action);
		};
	};

    if(state == QMediaPlayer::StoppedState)
    {
	    qDebug() << "Stopped, setting play act";
		m_stop_act->setEnabled(false);
	    // Note: We actually need to remove the existing default action here, or the button grows a drop-down menu with
	    // all the actions.
		delete_old_action_lambda();
		m_playButton->setDefaultAction(m_play_act);
    }
    else if( state == QMediaPlayer::PlayingState)
    {
	    qDebug() << "Playing, setting pause act";
		m_stop_act->setEnabled(true);
		delete_old_action_lambda();
		m_playButton->setDefaultAction(m_pause_act);
    }
    else if( state == QMediaPlayer::PausedState)
    {
	    qDebug() << "Paused, setting play act";
		m_stop_act->setEnabled(true);
		delete_old_action_lambda();
		m_playButton->setDefaultAction(m_play_act);
    }
}

void PlayerControls::setVolume(int volume)
{
	m_volumeSlider->setValue(volume);
}

void PlayerControls::setMuted(bool muted)
{
	if(muted != m_playerMuted)
    {
		m_playerMuted = muted;
        if(muted)
        {
			m_mute_act->setIcon(m_icon_muted);
        }
        else
        {
			m_mute_act->setIcon(m_icon_not_muted);
        }
    }
}

QMediaPlayer::State PlayerControls::state() const
{
	return m_playerState;
}

/**
 * Cycle through the Playing/Paused states.
 */
void PlayerControls::onPlayAction()
{
	qDebug() << "play";
	Q_EMIT play();
}

void PlayerControls::onPauseAction()
{
	qDebug() << "pause";
	Q_EMIT pause();
}

void PlayerControls::onTogglePlayPauseAction()
{
	qDebug() << "toggle";
	if(m_playerState == QMediaPlayer::StoppedState || m_playerState == QMediaPlayer::PausedState)
	{
		Q_EMIT play();
	}
	else if(m_playerState == QMediaPlayer::PlayingState)
	{
		Q_EMIT pause();
	}
}


void PlayerControls::muteClicked()
{
	Q_EMIT changeMuting(!m_playerMuted);
}

void PlayerControls::onDurationChanged(qint64 duration)
{
    // Duration is in ms.
	m_last_dur = duration;
	m_positionSlider->setRange(0, duration);
    updateDurationInfo(-1, duration);
}

void PlayerControls::onPositionChanged(qint64 pos)
{
    // Position is in ms.
	m_last_pos = pos;
	m_positionSlider->setValue(pos);
    updateDurationInfo(pos, -1);
}

void PlayerControls::updateDurationInfo(qint64 pos, qint64 duration)
{
    // Update the Time Remaining/Duration QLabel.
    if(pos < 0)
    {
		pos = m_last_pos;
    }
    if(duration < 0)
    {
		duration = m_last_dur;
    }

    pos /= 1000;
    duration /= 1000;

    QString format;
    if(duration > 3600)
    {
        format = "hh:mm:ss";
    }
    else
    {
        format = "mm:ss";
    }

    QString pos_str = "--:--";
    QString dur_str = "--:--";

    if(pos >= 0)
    {
        auto currentTime = QTime((pos / 3600) % 60, (pos / 60) % 60,
                            pos % 60, (pos * 1000) % 1000);
        pos_str = currentTime.toString(format);
    }
    if( duration >= 0)
    {
        auto totalTime = QTime((duration / 3600) % 60, (duration / 60) % 60,
                          duration % 60, (duration * 1000) % 1000);
        dur_str = totalTime.toString(format);
    }

    QString tStr = pos_str + " / " + dur_str;

	m_labelDuration->setText(tStr);
}

