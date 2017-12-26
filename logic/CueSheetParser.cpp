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

#include "CueSheetParser.h"

#include <QtGlobal>
#include <utils/StringHelpers.h>


CueSheetParser::CueSheetParser()
{

}

Cd *CueSheetParser::parse_cue_sheet_string(const char *bytes)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	// libcue (actually flex) can't handle invalid UTF-8.
	Q_ASSERT_X(isValidUTF8(bytes), __func__, "Invalid UTF-8 cuesheet string.");

	Cd* cd = cue_parse_string(bytes);

	Q_ASSERT_X(cd != nullptr, "cuesheet", "failed to parse cuesheet string");

	return cd;
}

#if 0
/// @todo Old code.
///
	@pyqtSlot()
	def scanLibrary(self):
		dlg = NetworkAwareFileDialog(self, "Select directory to scan")
		dlg.setFileMode(QFileDialog.Directory)
		dlg.setAcceptMode(QFileDialog.AcceptOpen)
		dlg.setOption(QFileDialog.ShowDirsOnly, true)
		if not dlg.exec_():
			return
		qDebug() << QString("Selected directories: {}".format(dlg.selectedUrls()))
		////// Scan the directory tree
		cuefiles = []
		for url in dlg.selectedUrls():
			dtw = DirTreeWalker([url], ["*.cue"])
			for fullname in dtw.walk():
				qDebug() << QString("FILE: {}".format(fullname))
				cuefiles.append(fullname)
		print("Names: {}".format(cuefiles))
		chardet_output = []
		for path in cuefiles:
			with open(path.toLocalFile(), "rb") as f:
				rawdata = f.read()
				charset = chardet.detect(rawdata)
				charset["path"] = path
				qDebug() << QString(charset)
				chardet_output.append(charset)
		print(chardet_output)
		bad_cuesheets = []
		for entry in chardet_output:
			if entry["confidence"] < 0.66:
				logger.warning("Low chardet confidence: {}".format(entry))
				continue
			if entry["encoding"] not in ["ascii", "utf-8"]:
				print("Found non-UTF-8 encoded file: {}".format(entry))
				bad_cuesheets.append(entry)
		cuesheet_fixer = CueSheetFixDialog(self, bad_cuesheets)
		cuesheet_fixer.exec_()
#endif
