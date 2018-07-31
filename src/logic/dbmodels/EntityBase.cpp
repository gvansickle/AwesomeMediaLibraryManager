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

#include "EntityBase.h"

#include "SQLHelpers.h"

#include <utils/RegisterQtMetatypes.h>
#include <src/logic/DirScanResult.h>
#include <utils/DebugHelpers.h>


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering Database Entity types";
	qRegisterMetaType<AMLM::EntityBase>();
	qRegisterMetaType<AMLM::CollectionMedium>();
	qRegisterMetaType<AMLM::ISRC>();
});

namespace AMLM
{

	CollectionMedium::CollectionMedium(const DirScanResult& dsr)
	{
//		m_dir_url = dsr.getDirProps();
		m_dir_props = dsr.getDirProps();
//		m_media_urls = dsr.getMediaQUrl();
		m_cue_url = dsr.getSidecarCuesheetExtUrl();
	}

	QString CollectionMedium::schema()
	{
		return "m_dir_url" TEXT NOT_NULL ","
				"m_dir_props" BLOB ","
				"m_media_urls" BLOB ","
				"m_cue_url" TEXT;
	}



} // namespace AMLM
