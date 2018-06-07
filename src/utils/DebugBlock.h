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

#ifndef SRC_UTILS_DEBUGBLOCK_H_
#define SRC_UTILS_DEBUGBLOCK_H_

/// Std C++
#include <string>

/**
 * Intent here is to Bring Debugging Back(tm) to Qt5/KF5.
 * This class in particular is intended to backfill some of the functionality
 * of KDE4's KDebug::Block.
 * https://api.kde.org/4.x-api/kdelibs-apidocs/kdecore/html/classKDebug_1_1Block.html
 */
class DebugBlock
{
public:
    explicit DebugBlock(const char* function_name);
	virtual ~DebugBlock();

protected:

    std::string get_indent();

private:
    const char *m_function_name;
};

#define DEBUGBLOCK() DebugBlock(__PRETTY_FUNCTION__)

#endif /* SRC_UTILS_DEBUGBLOCK_H_ */
