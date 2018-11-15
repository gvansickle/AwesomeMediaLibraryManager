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

#if 0 /// @todo !(No KF5 or deprecated KDE4 support)

// Std C++
#include <string>
#include <any>

/*
class KDebug;
// C++ doesn't let us do this.
class KDebug::Block;
*/

/**
 * Intent here is to Bring Debugging Back(tm) to Qt5/KF5.
 * This class in particular is intended to backfill some of the functionality
 * of KDE4's KDebug::Block.
 * https://api.kde.org/4.x-api/kdelibs-apidocs/kdecore/html/classKDebug_1_1Block.html
 *
 * That's actually "class KDebug { class Block; }".
 *
 */
class DebugBlock
{
public:
	explicit DebugBlock(const char* section, int area = 0);
	virtual ~DebugBlock();

//protected:

//    std::string get_indent();

private:
	const char *m_section_name;

	/*KDebug::Block**/ std::any m_kdebug_block;
};

/**
 * KDE's macro looks like this:
 * #define KDEBUG_BLOCK KDebug::Block _kDebugBlock(Q_FUNC_INFO);
 */
#define DEBUG_BLOCK DebugBlock DONTUSETHISPREFIX_DebugBlock(Q_FUNC_INFO);

#endif

#endif /* SRC_UTILS_DEBUGBLOCK_H_ */
