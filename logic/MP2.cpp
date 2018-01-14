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

#include "MP2.h"
#include "ntp.h"

#include "utils/ActionHelpers.h"
#include "utils/ConnectHelpers.h"

#include <QMediaPlaylist>
#include <QDebug>
#include <QUrlQuery>

#include <utils/Fraction.h>

MP2::MP2(QObject* parent, Flags flags) : QMediaPlayer(parent, flags)
{
	setAudioRole(QAudio::MusicRole);
	setNotifyInterval(10); // ms

	createActions();

	// Make initial connections from the underlying QMediaPlayer to slots in this.
	connect(this, &QMediaPlayer::positionChanged, this, &MP2::onPositionChanged);
	connect(this, &QMediaPlayer::durationChanged, this, &MP2::onDurationChanged);
	connect(this, &QMediaPlayer::mediaChanged, this, &MP2::onMediaChanged);
	connect(this, &QMediaPlayer::mediaStatusChanged, this, &MP2::onMediaStatusChanged);
	connect(this, &QMediaPlayer::currentMediaChanged, this, &MP2::onCurrentMediaChanged);
	connect(this, &QMediaPlayer::stateChanged, this, &MP2::onStateChanged);
	connect(qobject_cast<QMediaPlayer*>(this), static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error), this,
			static_cast<void(MP2::*)(QMediaPlayer::Error)>(&MP2::onPlayerError));
}

qint64 MP2::position() const
{
	if(m_is_subtrack)
	{
		return QMediaPlayer::position() - m_track_startpos_ms;
	}
	else
	{
		return QMediaPlayer::position();
	}
}

qint64 MP2::duration() const
{
	if(m_is_subtrack)
	{
		return m_track_endpos_ms - m_track_startpos_ms;
	}
	else
	{
		return QMediaPlayer::duration();
	}
}

void MP2::createActions()
{

}

void MP2::setTrackInfoFromUrl(QUrl url)
{
	qDebug() << QString("URL: '%1'").arg(url.toString());
	if(url.hasFragment())
	{
		m_is_subtrack = true;
		ntp ntpfrag(url);
		if(!ntpfrag.empty())
		{
			m_track_startpos_ms = ntpfrag.start_secs()*1000.0;
			m_track_endpos_ms = ntpfrag.end_secs()*1000.0;
		}
	}
	else
	{
		m_is_subtrack = false;
	}
	updateSeekToEndInfoOnMediaChange();
}

void MP2::updateSeekToEndInfoOnMediaChange()
{
	// We need to call this whenever the current media status changes.
	// If we were in seek-to-end mode, this sets things up to handle any possible pending seek msg, and cancels out of
	// seek-to-end mode.

	if(m_seek_to_end_mode)
	{
		// We were seeking to the end of the last track when this next track started.
		if(m_pending_seek_msg)
		{
			// There's an outstanding seek-to-end msg.  Ignore it when it comes in.
			qDebug() << "Seek msg pending, will ignore next setPosition(-1)";
			m_ignore_seek_msg = true;
		}
		// We're no longer in seek-to-end mode, this new track cancels it.
		m_seek_to_end_mode = false;
	}
}

void MP2::seekToEnd()
{
	// Start the seek-to-end process for a subtrack.
	qDebug() << "Queueing setPosition(-1) message.";
	m_seek_to_end_mode = true;
	m_pending_seek_msg = true;
	m_ignore_seek_msg = false;
	// Queue up a message to seek to the end of the current media.
	QMetaObject::invokeMethod(this, "setPosition", Qt::QueuedConnection, Q_ARG(qint64, -1));
}

void MP2::play()
{
	QMediaPlayer::play();
}

void MP2::stop()
{
	QMediaPlayer::stop();
	updateSeekToEndInfoOnMediaChange();
	if(m_is_subtrack)
	{
		QMediaPlayer::setPosition(m_track_startpos_ms);
	}
}

void MP2::setShuffleMode(bool shuffle_on)
{
	// QMediaPlaylist doesn't have orthogonal support for Sequential/Shuffle and Repeat/Non-repeat playback.
	// It's a single setting of QMediaPlaylist::PlaybackMode, which can be one of:
	// QMediaPlaylist::Random == Shuffle + Repeat
	// QMediaPlaylist::Sequential == Sequential + Non-Repeat
	// QMediaPlaylist::Loop == Sequential + Repeat.

	auto current_playlist = playlist();
	if(current_playlist)
	{
		if(shuffle_on)
		{
			// Shuffle plus Implicit repeat.
			current_playlist->setPlaybackMode(QMediaPlaylist::Random);
			qDebug() << "Shuffle+Repeat";
		}
		else
		{
			// Not shuffle.  Switch to Sequential + Repeat or + Non-repeat, depending on what
			// was last set.
			if(m_last_repeat_state)
			{
				current_playlist->setPlaybackMode(QMediaPlaylist::Loop);
				qDebug() << "Seq+Repeat";
			}
			else
			{
				current_playlist->setPlaybackMode(QMediaPlaylist::Sequential);
				qDebug() << "Seq+Stop";
			}
		}
	}
}

void MP2::repeat(bool checked)
{
	// QMediaPlaylist doesn't have orthogonal support for Sequential/Shuffle and Loop/Non-loop playback.
	// It's a single setting of QMediaPlaylist::PlaybackMode, which can be one of:
	// QMediaPlaylist::Random == Shuffle + Loop
	// QMediaPlaylist::Sequential == Sequential + Non-loop
	// QMediaPlaylist::Loop == Sequential + Loop.
	auto current_playlist = playlist();
	if(current_playlist)
	{
		if(current_playlist->playbackMode() == QMediaPlaylist::Random)
		{
			// Ignore this signal while we're in shuffle playback mode.
			qDebug() << "Currently in Shuffle mode, ignoring";
		}
		else
		{
			if(checked)
			{
				current_playlist->setPlaybackMode(QMediaPlaylist::Loop);
				qDebug() << "Seq+Repeat";
			}
			else
			{
				current_playlist->setPlaybackMode(QMediaPlaylist::Sequential);
				qDebug() << "Seq+Stop";
			}
		}
	}
	// Regardless of whether we ignored it above or not, record the new state for setShuffleMode().
	m_last_repeat_state = checked;
}

void MP2::setPosition(qint64 position)
{
	qDebug() << "setPosition:" << position;
	if(position == -1)
	{
		// This is an incoming "seek_to_end" msg.
		if(!m_ignore_seek_msg)
		{
			if(m_pending_seek_msg)
			{
				// Set position to the last millisecond.  This is for ending subtracks.
				qDebug() << "setPosition: seeking to end";
				QMediaPlayer::setPosition(QMediaPlayer::duration());
			}
		}
		else
		{
			m_ignore_seek_msg = false;
			qDebug() << "setPosition: ignoring.";
		}
		// Regardless of whether we ignored it or not, we've absorbed the seek message.
		m_pending_seek_msg = false;
	}
	else
	{
		// It's a normal setPosition() message.  Just forward to the superclass.
		QMediaPlayer::setPosition(position);
	}
}

void MP2::onPositionChanged(qint64 pos)
{
	if(m_is_subtrack)
	{
		auto subtrack_pos = (pos-m_track_startpos_ms);
		emit positionChanged2(subtrack_pos);
	}
	else
	{
		emit positionChanged2(pos);
	}
	if(!m_seek_to_end_mode)
	{
		// We're not already trying to seek to the end of the track.
		if(m_is_subtrack)
		{
			if(QMediaPlayer::position() >= m_track_endpos_ms)
			{
				qDebug() << QString("onPositionChanged: Seeking to subtrack end: %1").arg(QMediaPlayer::duration());
				seekToEnd();
			}
		}
	}
}

void MP2::onDurationChanged(qint64 duration)
{
	qDebug() << QString("onDurationChanged: %1").arg(duration);
	if(m_is_subtrack)
	{
		emit durationChanged2(m_track_endpos_ms - m_track_startpos_ms);
	}
	else
	{
		emit durationChanged2(duration);
	}
}

void MP2::onMediaChanged(const QMediaContent& media)
{
	// This could be the playlist.
	qDebug() << QString("onMediaChanged: %1").arg(media.canonicalUrl().toString());
}

void MP2::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
	qDebug() << QString("onMediaStatusChanged:") << status;
	if(status == QMediaPlayer::NoMedia)
	{
		updateSeekToEndInfoOnMediaChange();
	}
	else if(status == QMediaPlayer::EndOfMedia)
	{
		updateSeekToEndInfoOnMediaChange();
	}
}

void MP2::onCurrentMediaChanged(const QMediaContent& qmediacontent)
{
	// This is the active media content being played.  Could be Null.
	qDebug() << QString("onCurrentMediaChanged:") << qmediacontent.canonicalUrl();
	updateSeekToEndInfoOnMediaChange();
	if(!qmediacontent.isNull())
	{
		if(qmediacontent.playlist() == nullptr)
		{
			setTrackInfoFromUrl(qmediacontent.canonicalUrl());
			qDebug() << QString("track start: %1").arg(m_track_startpos_ms);
			QMediaPlayer::setPosition(m_track_startpos_ms);
		}
	}
}

void MP2::onStateChanged(QMediaPlayer::State state)
{
	qDebug() << QString("onStateChanged (Player): %1").arg(/*PlayerStateMap[*/state/*]*/);
}

void MP2::onPlayerError(QMediaPlayer::Error error)
{
	qWarning() << "Player error" << error << ":" << this->errorString();
}
