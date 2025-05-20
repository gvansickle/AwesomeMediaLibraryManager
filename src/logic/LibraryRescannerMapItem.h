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

#ifndef SRC_LOGIC_LIBRARYRESCANNERMAPITEM_H_
#define SRC_LOGIC_LIBRARYRESCANNERMAPITEM_H_

/// @file

// Std C++
#include <memory>

// Qt
#include <QDebug>
#include <QPersistentModelIndex>
#include <QVector>

// Ours
#include "LibraryEntry.h"

/**
 * Represents a single item in the LibraryModel which needs its data refreshed.
 */
struct LibraryRescannerMapItem
{
private:
    Q_GADGET

public:
	QPersistentModelIndex pindex {QPersistentModelIndex()};
	std::shared_ptr<LibraryEntry> item {nullptr};
};

//inline static QDebug operator<<(QDebug dbg, const LibraryRescannerMapItem &item)
//{
//    return dbg << QStringLiteral("LibraryRescannerMapItem(") << item.pindex << "," << item.item << ")";
//}

using VecLibRescannerMapItems = QVector<LibraryRescannerMapItem>;

Q_DECLARE_METATYPE(LibraryRescannerMapItem)

#endif /* SRC_LOGIC_LIBRARYRESCANNERMAPITEM_H_ */
