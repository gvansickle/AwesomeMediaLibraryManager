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

#ifndef MP2_H
#define MP2_H

/// @file

#include <QAction>
#include <QMediaPlayer>
#include <QAudioOutput>


class MP2 : public QMediaPlayer
{
	Q_OBJECT

public:
	enum ShuffleSetting {Shuffle, Sequential};
	Q_ENUM(ShuffleSetting)

	enum LoopSetting {Loop, NoLoop};
	Q_ENUM(LoopSetting)

public:
	explicit MP2(QObject *parent = nullptr);

	/// Property overrides.
	qint64 position() const;
	qint64 duration() const;
	bool muted() const;
	float volume() const;

Q_SIGNALS:
	/// This signal is emitted from the QMediaPlayer::positionChanged()-absorbing slot onPositionChanged()
	/// with an argument that is normalized to 0, that is, relative to the start of the track.
	void positionChanged2(qint64);
	/// This signal is emitted from the QMediaPlayer::durationChanged()-absorbing slot onDurationChanged()
	/// with an argument that is normalized to 0, that is, relative to the start of the track.
	void durationChanged2(qint64);
	void mutedChanged(bool);
	void volumeChanged(float);
	/// Signal which tells the playlist to go to the next item
	/// because of a media status change of QMediaPlayer::EndOfMedia.
	void playlistToNext();

private:
	Q_DISABLE_COPY(MP2)

	void createActions();
	void getTrackInfoFromUrl(QUrl url);

	std::unique_ptr<QAudioOutput> m_audio_output;

	/// @name State for use when we're playing a section of a larger sound file.
	/// @{
	bool m_is_subtrack = false;
	qint64 m_track_startpos_ms = 0;
	qint64 m_track_endpos_ms = 0;
	/// @}

	ShuffleSetting m_shuffle_setting {Shuffle};
	LoopSetting m_loop_setting {Loop};
	bool m_playing { false };
	bool m_seeking { false };

	/// We need to keep track of who gets to the end-of-(sub)track first, so we can block the other one
	/// from emitting the same playlistToNext signal.
	bool m_EndOfMedia_sending_playlistToNext {false};
	bool m_onPositionChanged_sending_playlistToNext {false};



public Q_SLOTS:
	void play();
	void stop();
    void setMuted(bool muted);
    void setVolume(float volume);
	void setShuffleMode(bool shuffle_on);
	void repeat(bool loop);
	void seek(int msecs);
	void seekStart();
	void seekEnd();

	void onPlaylistPositionChanged(const QModelIndex& current, const QModelIndex& previous);

private Q_SLOTS:
	void onPositionChanged(qint64 pos);
	void onDurationChanged(qint64 duration);
	void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
	void onPlaybackStateChanged(QMediaPlayer::PlaybackState newState);
	void onSourceChanged(const QUrl& media_url);
	void onErrorOccurred(QMediaPlayer::Error error, const QString& errorString);
};



#endif // MP2_H
