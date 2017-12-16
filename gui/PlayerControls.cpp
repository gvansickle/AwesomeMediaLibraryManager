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

#include <qxtglobalshortcut.h>
#include <QtCore/QPointer>

#include "utils/Theme.h"
#include "utils/ConnectHelpers.h"
#include "utils/ActionHelpers.h"

PlayerControls::PlayerControls(QWidget *parent) : QWidget(parent)
{
	m_playButton = new QToolButton(this);
	connect_clicked(m_playButton, this, &PlayerControls::playClicked);
    m_icon_play = Theme::iconFromTheme("media-playback-start");
	m_icon_pause = Theme::iconFromTheme("media-playback-pause");
	m_playButton->setIcon(m_icon_play);
	//m_playButton->defaultAction()->setShortcut(QKeySequence(Qt::Key_MediaPlay));
	///@todo m_play_act = make_action(m_icon_play, "Play", this, QKeySequence(Qt::Key_MediaPlay), "Start media playback");
	/// m_playButton->setDefaultAction(m_play_act);

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

    // Shuffle button will be connected to an action, no need to set an icon here.
	m_shuffleButton = new QToolButton(this);

	m_repeatButton = new QToolButton(this);
	m_repeat_icon = Theme::iconFromTheme("media-playlist-repeat");
	m_repeat_act = make_action(m_repeat_icon, "Repeat", this, QKeySequence(), "Repeat after last song in playlist is played");
	m_repeat_act->setCheckable(true);
	m_repeatButton->setDefaultAction(m_repeat_act);
	m_playerRepeat = false;
	// Signal-to-signal connection, emit a changeRepeat(bool) when the "Repeat" button changes checked state.
	connect(m_repeat_act, &QAction::toggled, this, &PlayerControls::changeRepeat);

	m_muteButton = new QToolButton(this);
	m_icon_mute = QIcon::fromTheme("audio-volume-muted");
	m_icon_unmute = QIcon::fromTheme("audio-volume-high");
	m_muteButton->setIcon(m_icon_unmute);
	connect_clicked(m_muteButton, this, &PlayerControls::muteClicked);

    // Position slider
	m_positionSlider = new QSlider(Qt::Horizontal);
    //slider->setRange(0, player.duration() / 1000)
    //slider->sliderMoved.connect(seek)

    // Time Remaining/Duration label.
	m_labelDuration = new QLabel(this);

    // Volume slider.
	m_volumeSlider = new QSlider(Qt::Horizontal);
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

void PlayerControls::registerMediaKeySequences()
{
	qDebug() << "Setting global shortcuts";

	m_media_key_stop_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaStop), m_stop_act, this);
	m_media_key_next_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaNext), m_skip_fwd_act, this);
	m_media_key_prev_gshortcut = make_QxtGlobalShortcut(QKeySequence(Qt::Key_MediaPrevious), m_skip_back_act, this);

	// Qt::Key_MediaPause
	// Qt::Key_MediaPlay
	// Qt::Key_MediaTogglePlayPause
	// Qt::Key_LaunchMedia
}

int PlayerControls::volume() const
{
	return m_volumeSlider->value();
}

void PlayerControls::setState(QMediaPlayer::State state)
{
	if(state != m_playerState)
    {
		m_playerState = state;

        if(state == QMediaPlayer::StoppedState)
        {
			m_stop_act->setEnabled(false);
			m_playButton->setIcon(m_icon_play);
        }
        else if( state == QMediaPlayer::PlayingState)
        {
			m_stop_act->setEnabled(true);
			m_playButton->setIcon(m_icon_pause);
        }
        else if( state == QMediaPlayer::PausedState)
        {
			m_stop_act->setEnabled(true);
			m_playButton->setIcon(m_icon_play);
        }
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
			m_muteButton->setIcon(m_icon_mute);
        }
        else
        {
			m_muteButton->setIcon(m_icon_unmute);
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
void PlayerControls::playClicked()
{
	if(m_playerState == QMediaPlayer::StoppedState || m_playerState == QMediaPlayer::PausedState)
    {
        emit play();
    }
	else if(m_playerState == QMediaPlayer::PlayingState)
    {
        emit pause();
    }
}

void PlayerControls::muteClicked()
{
	emit changeMuting(!m_playerMuted);
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

