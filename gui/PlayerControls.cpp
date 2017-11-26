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

	m_stopButton = new QToolButton(this);
	m_icon_stop = QIcon::fromTheme("media-playback-stop");
	m_stopButton->setIcon(m_icon_stop);
	m_stopButton->setEnabled(false);
	connect_clicked(m_stopButton, this, &PlayerControls::stop);

	m_nextButton = new QToolButton(this);
	m_nextButton->setIcon(QIcon::fromTheme("media-skip-forward"));
	connect_clicked(m_nextButton, this, &PlayerControls::next);

	m_previousButton = new QToolButton(this);
	m_previousButton->setIcon(QIcon::fromTheme("media-skip-backward"));
	connect_clicked(m_previousButton, this, &PlayerControls::previous);

    // Shuffle button will be connected to an action, no need to set an icon here.
	m_shuffleButton = new QToolButton(this);

	m_repeatButton = new QToolButton(this);
	m_repeat_icon = QIcon::fromTheme("media-playlist-repeat");
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
			m_stopButton->setEnabled(false);
			m_playButton->setIcon(m_icon_play);
        }
        else if( state == QMediaPlayer::PlayingState)
        {
			m_stopButton->setEnabled(true);
			m_playButton->setIcon(m_icon_pause);
        }
        else if( state == QMediaPlayer::PausedState)
        {
			m_stopButton->setEnabled(true);
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
