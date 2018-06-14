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

#ifndef SRC_LOGIC_DIRSCANRESULT_H_
#define SRC_LOGIC_DIRSCANRESULT_H_

#include <config.h>

/// Qt5
#include <QUrl>
#include <QFileInfo>

/**
 *
 */
class DirScanResult
{
public:
	DirScanResult();
    DirScanResult(const QUrl& found_url, const QFileInfo& found_url_finfo);
	virtual ~DirScanResult();

    /**
     * URLs:
     * - Sidecar cuesheet
     * - Sidecar album art (folder.jpg/cover.jpg)
     * Bools:
     * - Is result:
     * -- Dir with only single album/disc rip
     * -- Dir with random files
     */

    bool hasSidecarCuesheet() const;

protected:

    QUrl m_found_url;
    QFileInfo m_found_url_finfo;

    QUrl m_dir_url;

};

Q_DECLARE_METATYPE(DirScanResult);

#endif /* SRC_LOGIC_DIRSCANRESULT_H_ */
