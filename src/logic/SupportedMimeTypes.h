/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef SRC_LOGIC_SUPPORTEDMIMETYPES_H_
#define SRC_LOGIC_SUPPORTEDMIMETYPES_H_

/// @file

#include <QObject>
#include <QVector>
#include <QMimeType>

/**
 *
 */
class SupportedMimeTypes: public QObject
{
    Q_OBJECT

public:
	explicit SupportedMimeTypes(QObject* parent = nullptr);
    ~SupportedMimeTypes() override;

	static SupportedMimeTypes& instance(QObject* parent = nullptr);

    QVector<QMimeType> supportedAudioMimeTypes() const;
    QStringList supportedAudioMimeTypesAsFilterStringList() const;
    QStringList supportedAudioMimeTypesAsSuffixStringList() const;

protected:
    QStringList m_mime_audio_types;
    QStringList m_mime_audio_associated_types;
    QStringList m_mime_playlist_types;

private:

};


#endif /* SRC_LOGIC_SUPPORTEDMIMETYPES_H_ */
