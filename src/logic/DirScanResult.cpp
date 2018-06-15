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

#include "DirScanResult.h"

#include <config.h>

/// Qt5
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

DirScanResult::DirScanResult()
{

}

DirScanResult::DirScanResult(const QUrl &found_url, const QFileInfo &found_url_finfo)
{
    m_found_url = found_url;
    m_found_url_finfo = found_url_finfo;
}

DirScanResult::~DirScanResult()
{
}

void DirScanResult::determineDirProps()
{
    if(false) // local file
    {
        QDir dir_url_qdir = m_found_url_finfo.dir();
        m_dir_url = QUrl::fromLocalFile(dir_url_qdir.absolutePath());
    }
    else // Works for any URL.
    {
        m_dir_url = m_found_url.adjusted(QUrl::RemoveFilename);
    }

    // Sidecar cue sheet?
    // Create the *.cue URL.
    m_cue_url = m_found_url;
    QString cue_url_as_str = m_cue_url.toString();
    Q_ASSERT(!cue_url_as_str.isEmpty());
    cue_url_as_str.replace(QRegularExpression("\\.[[:alnum:]]+$"), ".cue");
    m_cue_url = cue_url_as_str;
    Q_ASSERT(m_cue_url.isValid());
}




