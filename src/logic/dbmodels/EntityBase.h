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

	public:
		static void schema() {};
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

	protected:

		/// Absolute URL to the directory.
		QUrl m_dir_url;

		DirScanResult::DirProps m_dir_props { DirScanResult::DirProp::Unknown };

		/// The media URL which was found.
		QUrl m_media_url;
		/// Info for detecting changes
		FileModificationInfo m_found_url_modinfo;

		/// URL to a sidecar cuesheet.  May be empty if none was found.
		QUrl m_cue_url;

		/// Info for detecting changes
		FileModificationInfo m_cue_url_modinfo;
	};

	class ISRC : public EntityBase {};
	class Artist : public EntityBase {};
	class Release : public EntityBase {};
	class ReleaseGroup : public EntityBase {};
	class Medium : public EntityBase {};
	class Track : public EntityBase {};


}; // END namespace AMLM

Q_DECLARE_METATYPE(AMLM::EntityBase);

#endif // ENTITYBASE_H
