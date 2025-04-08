/*
 * Copyright 2018, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file IfExistsAskForOverwrite.h
 */

#ifndef SRC_GUI_HELPERS_IFEXISTSASKFOROVERWRITE_H_
#define SRC_GUI_HELPERS_IFEXISTSASKFOROVERWRITE_H_

#include <QUrl>

#include "future/guideline_helpers.h"


/**
 *
 */
class IfExistsAskForOverwrite
{
public:
	IfExistsAskForOverwrite();
	virtual ~IfExistsAskForOverwrite();
    M_GH_POLYMORPHIC_SUPPRESS_COPYING_C67(IfExistsAskForOverwrite)

	static bool IfExistsAskForDelete(const QUrl &filename);

};

#endif /* SRC_GUI_HELPERS_IFEXISTSASKFOROVERWRITE_H_ */
