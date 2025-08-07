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

/**
 * @file ntp.cpp
 * Implementation of ntp, a Normal Play Time support class for QUrl.
 */

#include "ntp.h"

// Qt
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

using namespace std::literals;

ntp::ntp(const QUrl& url)
{
	// Get the query out of the fragment area.
	QUrlQuery qurlquery = QUrlQuery(url.fragment());
	if(!qurlquery.isEmpty())
	{
		// There was a fragment, does it have a Media Fragment Temporal Dimension?
		/// @todo This returns the first "t" element.  The standard says the last is the one which should be used.
		QString mf_temporal_val;
		if(qurlquery.hasQueryItem("t"))
		{
			// Yep, let's see what we have.
			mf_temporal_val = qurlquery.queryItemValue("t");

			// Full syntax of the value is as described here: https://www.w3.org/TR/media-frags/#naming-time (transitively
			// through http://www.ietf.org/rfc/rfc2326.txt section 3.6).
			// At the moment we will only handle the ntp: format (actual specifier is optional, since it's the default)
			// and the npt-sec format of npttimedef.  Expressed as a regex:
			// (ntp:)?(\d+\.\d*)?,(\d+\.\d*)?

            static const QRegularExpression re{R"!((?:ntp:)?(\d+\.\d*)?,(\d+\.\d*)?)!"};
			auto mo = re.match(mf_temporal_val);
			if(!mo.hasMatch())
			{
				qCritical() << "Malformed or unsupported Media Fragment/Temporal Dimension:" << mf_temporal_val;
			}
			else
			{
				auto start_str = mo.captured(1);
				auto end_str = mo.captured(2);
				bool start_ok = false;
				bool end_ok = false;
				if(start_str == "")
				{
					// Start was empty, spec says this means 0.
					m_start_offset_secs = 0.0;
				}
				else
				{
					m_start_offset_secs = start_str.toDouble(&start_ok);
				}
				if(end_str == "")
				{
					// Start was empty, spec says this means "end of the track".
					// We don't know the track length, so we'll set this negative (which is invalid)
					// to tell the user that.
					m_end_offset_secs = -1.0;
				}
				else
				{
					m_end_offset_secs = end_str.toDouble(&end_ok);
				}
				if(!(start_ok || end_ok))
				{
					qCritical() << "Malformed or unsupported Media Fragment/Temporal Dimension:" << mf_temporal_val;
				}

				qDebug() << "ntp start:" << m_start_offset_secs << "end:" << m_end_offset_secs;
			}
		}
	}
}

ntp::ntp(double start_secs, double end_secs)
{
	m_start_offset_secs = start_secs;
	m_end_offset_secs = end_secs;
}

keyvalue ntp::toKeyValPair() const
{
	return keyvalue{"t"s, QString("%1,%2").arg(m_start_offset_secs,0,'f',4).arg(m_end_offset_secs,0,'f',4).toStdString()};
}

