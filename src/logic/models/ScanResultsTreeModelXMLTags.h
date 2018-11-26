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

#ifndef AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
#define AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H

// Std C++
#include <map>
#include <string>

// Qt5
#include <QObject>
#include <QtXml>

class DSRTag
{
	Q_GADGET
public:
	enum TagName
	{
		EXTURL_DIR,
		EXTURL_MEDIA,
		EXTURL_CUESHEET
	};
	Q_ENUM(TagName)

	/**
	 * Forwards DSRTag operator[] to the std::map's .at() function, so we don't populate the map but rather throw.
	 */
	const std::string operator[](DSRTag::TagName i) const { return m_dsrtag_to_string.at(i); };

protected:
	static std::map<DSRTag::TagName, std::string> m_dsrtag_to_string;

};



#endif //AWESOMEMEDIALIBRARYMANAGER_SCANRESULTSTREEMODELXMLTAGS_H
