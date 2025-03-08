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
	connect_or_die(this, &QMediaPlayer::errorOccurred, this, &MP2::onErrorOccurred);
	connect_or_die(this, &QMediaPlayer::positionChanged, this, &MP2::onPositionChanged);
	connect_or_die(this, &QMediaPlayer::durationChanged, this, &MP2::onDurationChanged);
	connect_or_die(this, &QMediaPlayer::mediaStatusChanged, this, &MP2::onMediaStatusChanged);

	// Reflect some signals from our QAudioOutput to signals coming from this object.
	// This avoids having to expose m_audio_output for a third party to make connections to.
	connect_or_die(m_audio_output.get(), &QAudioOutput::volumeChanged, this, &MP2::volumeChanged);
	connect_or_die(m_audio_output.get(), &QAudioOutput::mutedChanged, this, &MP2::mutedChanged);
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

float MP2::volume() const
{
	return m_audio_output->volume();
}

void MP2::createActions()
{

}

void MP2::getTrackInfoFromUrl(QUrl url)
{
	qDebug() << "URL: " << url.toString();
	if(url.hasFragment())
	{
		m_is_subtrack = true;
		ntp ntpfrag(url);
		if(!ntpfrag.empty())
		{
			m_track_startpos_ms = ntpfrag.start_secs()*1000.0;
			m_track_endpos_ms = ntpfrag.end_secs()*1000.0;
			qDb() << "SET START AND END";
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
	if (playback_state == QMediaPlayer::StoppedState)  //<= This is usually PlayingState / LoadedMedia.
	{
		if (m_is_subtrack)
		{
			QMediaPlayer::setPosition(m_track_startpos_ms);
		}
	}
	m_playing = true;
	QMediaPlayer::play();
}

void MP2::stop()
{
	m_playing = false;
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
	m_shuffle_setting = shuffle_on ? MP2::Shuffle : MP2::Sequential;
}

void MP2::repeat(bool loop)
{
	m_loop_setting = loop ? MP2::Loop : MP2::NoLoop;
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

	if (m_is_subtrack && (QMediaPlayer::position() >= m_track_endpos_ms))
	{
		// Subtrack, and the position is past its end.  Stop playback and tell the playlist we're connected to
		// to send us the next media URL to play.
		if (!m_EndOfMedia_sending_playlistToNext)
		{
			m_onPositionChanged_sending_playlistToNext = true;
			QMediaPlayer::stop();
			Q_EMIT playlistToNext();
		}
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
			// Player should be in StoppedState here.
			qDb() << QString("setPosition() to track start: %1 ms").arg(m_track_startpos_ms);
			QMediaPlayer::setPosition(m_track_startpos_ms);

			if (m_playing)
			{
				play();
			}
			break;
		}
		case QMediaPlayer::EndOfMedia:
		{
			// Player should be in StoppedState here.
			// Note: We aren't get these msgs from subtracks when we do a seek-to-end for some reason.
			/// We have two separate things which will emit playlistToNext() (see the other playlistToNext() emit in
			/// onPositionChanged()).  We could end up with a subtrack at the end of its file triggering
			/// both of these emits, and hence doing a double skip.  This logic here and in onPositionChanged()
			/// prevents that race and guarantees only one playlistToNext() is sent.
			/// These two flags allow only one to send the signal, which comes back to us via the
			/// onPlaylistPositionChanged() slot.  That slot clears the flags to reset this mechanism for the next track.
			if(!m_onPositionChanged_sending_playlistToNext)
			{
				m_EndOfMedia_sending_playlistToNext = true;
				Q_EMIT playlistToNext();
			}
			break;
		}
	}
}

void MP2::onSourceChanged(const QUrl& media_url)
{
	qDebug() << "onSourceChanged, URL:" << media_url;

	if(media_url.isValid() && !media_url.isEmpty())
	{
		// Get the info we need from the URL's Fragment.
		getTrackInfoFromUrl(media_url);
		// Remove the Fragment before we pass the URL to QMediaPlayer.
		QUrl url_minus_fragment = media_url.toString(QUrl::RemoveFragment);
		// Note that setSource():
		// - Discards all info it has for the current media source.
		// - Stops playback.
		// - returns immediately.
		// So we can't setPosition() to the beginning of the track here, we have to do that when we get a
		// mediaStatusChange of QMediaPlayer::LoadedMedia.
		QMediaPlayer::setSource(url_minus_fragment);
	}
}



void MP2::onPlaylistPositionChanged(const QModelIndex& current, const QModelIndex& previous)
{
	// We get in here when the Playlist view sends the QItemSelectionModel::currentChanged signal.
	// That signal can come from:
	// - User input on the playlist view.
	// - The MP2::playlistToNext signal emitted by us (in two different places).

	m_onPositionChanged_sending_playlistToNext = false;
	m_EndOfMedia_sending_playlistToNext = false;

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

void MP2::onErrorOccurred(QMediaPlayer::Error error, const QString& errorString)
{
	qCr() << "PLAYER ERROR:" << error << errorString;
}
