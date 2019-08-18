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

#include "LibraryEntryMimeData.h"

//class AdditionalMimeTypesHelper
//{
//public:
//	const QStringList g_additional_supported_mimetypes { {"application/x-grvs-libraryentryref"} };
//};
//class GSHelper
//{
//public:
//	GSHelper();
//};

//Q_GLOBAL_STATIC(AdditionalMimeTypesHelper, s_additional_supported_mimetypes);
Q_GLOBAL_STATIC_WITH_ARGS(QStringList, g_additional_supported_mimetypes, { {"application/x-grvs-libraryentryref"} });

LibraryEntryMimeData::LibraryEntryMimeData() : QMimeData ()
{

}

bool LibraryEntryMimeData::hasFormat(const QString& mimetype) const
{
	if(mimetype == (*g_additional_supported_mimetypes)[0] // s_additional_supported_mimetypes()->g_additional_supported_mimetypes[0]
			&& !m_lib_item_list.empty())
	{
		return true;
	}
	return false;
}

QStringList LibraryEntryMimeData::formats() const
{
	QStringList retval;

	if(!m_lib_item_list.empty())
	{
		retval.append(g_additional_supported_mimetypes[0]);
	}

	retval += this->QMimeData::formats();

	return retval;
}
