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

/** @file Interface for ntp, a Normal Play Time support class for QUrl. */

#ifndef NTP_H
#define NTP_H

#include <string>


class QUrl;

struct keyvalue
{
	std::string key;
	std::string value;
};

/**
 * Class implementing an abstraction for Normal Play Time per http://www.ietf.org/rfc/rfc2326.txt "Real Time Streaming Protocol (RTSP)",
 * section 3.6 Normal Play Time, and used in Media Fragments URI 1.0 (basic), https://www.w3.org/TR/media-frags/.
 *
 * We use this for communicating the start and end times of subtracks of a single audio file to the Media Player.
 */
class ntp
{
public:
	/// Create an ntp object from the fragment area of @a url, if any.
	/// Note that since this is from the fragment, it's up to the user-agent to interpret and use the encoded info.
	ntp(const QUrl& url);

	ntp(double start_secs, double end_secs);

	bool empty() const { return m_start_offset_secs < 0.0 && m_end_offset_secs < 0.0; }

	double start_secs() const { return m_start_offset_secs; }
	double end_secs() const { return m_end_offset_secs; }

	keyvalue toKeyValPair() const;

private:

	double m_start_offset_secs { -1.0 };
	double m_end_offset_secs { -1.0 };
};

#endif // NTP_H

