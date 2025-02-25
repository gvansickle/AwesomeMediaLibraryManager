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

#include <QAction>
#include <QMediaPlayer>
#include <QAudioOutput>


class MP2 : public QMediaPlayer
{
	Q_OBJECT

	Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)

public:
	MP2(QObject *parent = Q_NULLPTR);

	/// Property overrides.
	qint64 position() const;
	qint64 duration() const;
    int muted() const;
    int volume() const;

Q_SIGNALS:
	void positionChanged2(qint64);
	void durationChanged2(qint64);
    void mutedChanged(bool);
    void volumeChanged(int);

private:
	Q_DISABLE_COPY(MP2)

	std::unique_ptr<QAudioOutput> m_audio_output;

	/// @name State for use when we're playing a section of a larger sound file.
	/// @{
	bool m_is_subtrack = false;
	qint64 m_track_startpos_ms = 0;
	qint64 m_track_endpos_ms = 0;
	/// @}

	/// @name Seek-to-end control variables for managing subtrack playback.
	/// @{
	bool m_seek_to_end_mode = false;
	bool m_pending_seek_msg = false;
	bool m_ignore_seek_msg = false;
	/// @}

	/// For managing transitions between Shuffle/Sequential+Stop/Sequential+Repeat.
	bool m_last_repeat_state = false;

	void createActions();
	void setTrackInfoFromUrl(QUrl url);
	void updateSeekToEndInfoOnMediaChange();
	void seekToEnd();

public Q_SLOTS:
	void play();
	void stop();
    void setMuted(bool muted);
    void setVolume(int volume);
	void setShuffleMode(bool shuffle_on);
	void repeat(bool checked);

	void setPosition(qint64 position);
	void onPositionChanged(qint64 pos);
	void onDurationChanged(qint64 duration);
//	void onMediaChanged(const QMediaContent &media);
	void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

#if 0 // QT5
	void onCurrentMediaChanged(const QMediaContent &qmediacontent);
#elif 1 // QT6
	void onSourceChanged(const QUrl& url);
#endif
#if 0 // @todo QT6
	void onStateChanged(QMediaPlayer::State state);
	void onPlayerError(QMediaPlayer::Error error);
#endif
};

#endif // MP2_H
