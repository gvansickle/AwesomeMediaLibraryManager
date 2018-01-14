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

#ifndef MP2_H
#define MP2_H

#include <QAction>
#include <QMediaPlayer>



class MP2 : public QMediaPlayer
{
	Q_OBJECT

public:
	MP2(QObject *parent = Q_NULLPTR, Flags flags = Flags());

	/// Property overrides.
	qint64 position() const;
	qint64 duration() const;

signals:
	void positionChanged2(qint64);
	void durationChanged2(qint64);

private:
	Q_DISABLE_COPY(MP2)

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

public slots:
	void play();
	void stop();
	void setShuffleMode(bool shuffle_on);
	void repeat(bool checked);

	void setPosition(qint64 position);
	void onPositionChanged(qint64 pos);
	void onDurationChanged(qint64 duration);
	void onMediaChanged(const QMediaContent &media);
	void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
	void onCurrentMediaChanged(const QMediaContent &qmediacontent);
	void onStateChanged(QMediaPlayer::State state);
	void onPlayerError(QMediaPlayer::Error error);
};

#endif // MP2_H
