/*
 * Copyright 2017, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Qt
//#include <QMediaPlaylist>
#include <QDebug>
#include <QUrlQuery>
#include <QAudioDevice>
#include <QMediaDevices>

// Ours
#include <ModelUserRoles.h>
#include <PlaylistModelItem.h>
#include <utils/Fraction.h>
#include "utils/ConnectHelpers.h"
#include "ntp.h"
#include <gui/actions/ActionHelpers.h>


MP2::MP2(QObject* parent) : QMediaPlayer(parent)
{
//	setAudioRole(QAudio::MusicRole);
//	setNotifyInterval(10); // ms

	auto dev_list = QMediaDevices::audioOutputs();
	for (auto& dev : dev_list)
	{
		qDb() << "AUDIODEV:" << dev.description() << "ID:" << dev.id();
	}
	qDb() << M_ID_VAL(QMediaDevices::defaultAudioOutput().description());

	m_audio_output = std::make_unique<QAudioOutput>(this);
	m_audio_output->setDevice(QAudioDevice());
	setAudioOutput(m_audio_output.get());

	createActions();

	// Make initial connections from the underlying QMediaPlayer to slots in this.
	connect_or_die(this, &QMediaPlayer::positionChanged, this, &MP2::onPositionChanged);
	connect_or_die(this, &QMediaPlayer::durationChanged, this, &MP2::onDurationChanged);
//	connect_or_die(this, &QMediaPlayer::mediaChanged, this, &MP2::onMediaChanged);
	connect_or_die(this, &QMediaPlayer::mediaStatusChanged, this, &MP2::onMediaStatusChanged);

	// Reflect some signals from our QAudioOutput to signals coming from this object.
	// This avoids having to expose m_audio_output for a third party to make connections to.
	connect_or_die(m_audio_output.get(), &QAudioOutput::volumeChanged, this, &MP2::volumeChanged);
	connect_or_die(m_audio_output.get(), &QAudioOutput::mutedChanged, this, &MP2::mutedChanged);

	// connect_or_die(this, &QMediaPlayer::sourceChanged, this, &MP2::onSourceChanged);

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

bool MP2::muted() const
{
	return m_audio_output->isMuted();
}

int MP2::volume() const
{
	return m_audio_output->volume()*100;
}

void MP2::createActions()
{

}

void MP2::getTrackInfoFromUrl(QUrl url)
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
}

void MP2::play()
{
	qDb() << "play(), current media URL:" << source() << M_ID_VAL(m_track_startpos_ms) << M_ID_VAL(m_track_endpos_ms);
	auto playback_state = QMediaPlayer::playbackState();
	if (playback_state == QMediaPlayer::StoppedState)
	{
		if (m_is_subtrack)
		{
			QMediaPlayer::setPosition(m_track_startpos_ms);
		}
	}
	QMediaPlayer::play();
}

void MP2::stop()
{
	QMediaPlayer::stop();
	if(m_is_subtrack)
	{
		QMediaPlayer::setPosition(m_track_startpos_ms);
	}
}

void MP2::setMuted(bool muted)
{
	m_audio_output->setMuted(muted);
}

void MP2::setVolume(float volume)
{
	m_audio_output->setVolume(volume);
}

void MP2::setShuffleMode(bool shuffle_on)
{
#if 0 // QT5
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
#elif 1 // QT6
	// QT6's MediaPlayer doesn't have a QMediaPlaylist, so we can't do much here directly.
	M_WARNING("TODO: Add back playback mode support")
#endif
}

void MP2::repeat(bool checked)
{
#if 0 // QT5
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
#elif 1 // QT6
	if(checked)
	{
		setLoops(QMediaPlayer::Infinite);
	}
	else
	{
		setLoops(1);
	}
#endif
	// Regardless of whether we ignored it above or not, record the new state for setShuffleMode().
	m_last_repeat_state = checked;
}

void MP2::setPosition(qint64 position)
{
	qDebug() << "setPosition:" << position;
#if 0
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
#endif
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
		Q_EMIT positionChanged2(subtrack_pos);
	}
	else
	{
		Q_EMIT positionChanged2(pos);
	}

	if (m_is_subtrack && QMediaPlayer::position() >= m_track_endpos_ms)
	{
		qDb() << "Subtrack position past its end, seeking to the end of the file.";
		QMediaPlayer::setPosition(QMediaPlayer::duration());
	}
}

void MP2::onDurationChanged(qint64 duration)
{
	qDebug() << QString("onDurationChanged: %1").arg(duration);
	if(m_is_subtrack)
	{
		Q_EMIT durationChanged2(m_track_endpos_ms - m_track_startpos_ms);
	}
	else
	{
		Q_EMIT durationChanged2(duration);
	}
}

void MP2::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
	qDebug() << QString("onMediaStatusChanged:") << status;
	qDb() << M_ID_VAL(playbackState());
	switch(status)
	{
		case QMediaPlayer::NoMedia:
		{
			break;
		}
		case QMediaPlayer::LoadedMedia:
		{
			// if (/*were already playing*/)
			{

			}
			break;
		}
		case QMediaPlayer::EndOfMedia:
		{
			Q_EMIT playlistToNext();
			break;
		}
	}
}

#if 0 // !QT6
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
#elif 1 // QT6
void MP2::onSourceChanged(const QUrl& media_url)
{
	qDebug() << QString("onSourceChanged:") << media_url;

	if(media_url.isValid() && !media_url.isEmpty())
	{
		// Get the info we need from the URL's Fragment.
		getTrackInfoFromUrl(media_url);
		// Remove the Fragment before we pass the URL to QMediaPlayer.
		QUrl url_minus_fragment = media_url.toString(QUrl::RemoveFragment);
		QMediaPlayer::setSource(url_minus_fragment);
		qDebug() << QString("track start: %1 ms").arg(m_track_startpos_ms);
		QMediaPlayer::setPosition(m_track_startpos_ms);
	}
}
#endif

void MP2::onPlaylistPositionChanged(const QModelIndex& current, const QModelIndex& previous)
{
	// We get in here when the Playlist view sends the QItemSelectionModel::currentChanged signal.
	// That signal can come from:
	// - User input on the playlist view.
	// - The MP2::playlistToNext signal emitted by us.

	qDb() << "playlistPosChanged:" << current;

	if (!current.isValid())
	{
		qWr() << "Invalid QModelIndex: " << current;
	}
	else
	{
		// Set up to play the newly-selected track.
		QVariant libentry = current.siblingAtColumn(0).data(ModelUserRoles::PointerToItemRole);
		Q_ASSERT(libentry.isValid());
		std::shared_ptr<PlaylistModelItem> item = libentry.value<std::shared_ptr<PlaylistModelItem>>();
		auto new_url = item->getM2Url();
		qDb() << M_ID_VAL(new_url);
		onSourceChanged(new_url);
	}
}

#if 0 // QT6
void MP2::onStateChanged(QMediaPlayer::State state)
{
	qDebug() << QString("onStateChanged (Player): %1").arg(/*PlayerStateMap[*/state/*]*/);
}

void MP2::onPlayerError(QMediaPlayer::Error error)
{
	qWarning() << "Player error" << error << ":" << this->errorString();
}
#endif
