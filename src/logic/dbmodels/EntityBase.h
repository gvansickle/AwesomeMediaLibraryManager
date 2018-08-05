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

#ifndef ENTITYBASE_H
#define ENTITYBASE_H

#include <QtGlobal>
#include <QObject>
#include <QUrl>

#include <src/logic/DirScanResult.h>


namespace AMLM
{
	class EntityBase
	{
		Q_GADGET

	public:
		EntityBase() = default;
		EntityBase(const EntityBase& other) = default;
		virtual ~EntityBase() = default;

//	public:
//		static QString schema() { return ""; };
	};



	/**
	 * E.g. A single mp3 file, a directory containing a single CD rip, etc.
	 */
	class CollectionMedium : public EntityBase
	{
		Q_GADGET

	public:
		CollectionMedium() = default;
		CollectionMedium(const CollectionMedium& other) = default;
		explicit CollectionMedium(const DirScanResult& dsr);
		~CollectionMedium() override = default;

		static QString schema();

	protected:

		/// Absolute URL to the directory.
		ExtUrl m_dir_url;

		/// Properties of the directory.
		DirScanResult::DirProps m_dir_props { DirScanResult::DirProp::Unknown };

		/// The URLs of the media files plus Info for detecting changes
		QVector<ExtUrl> m_media_urls;

		/// URL to a sidecar cuesheet.  May be empty if none was found.
		ExtUrl m_cue_url;
	};

	class ISRC : public EntityBase {};
	class Artist : public EntityBase {};
	class Release : public EntityBase {};
	class ReleaseGroup : public EntityBase {};
	class Medium : public EntityBase {};
	class Track : public EntityBase {};


}; // END namespace AMLM

Q_DECLARE_METATYPE(AMLM::EntityBase);
Q_DECLARE_METATYPE(AMLM::CollectionMedium);
Q_DECLARE_METATYPE(AMLM::ISRC);


#endif // ENTITYBASE_H
