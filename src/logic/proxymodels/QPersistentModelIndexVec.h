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

#ifndef LOGIC_PROXYMODELS_QPERSISTENTMODELINDEXVEC_H_
#define LOGIC_PROXYMODELS_QPERSISTENTMODELINDEXVEC_H_

/// @file

#include <QVector>
#include <QPersistentModelIndex>

/*
 *
 */
class QPersistentModelIndexVec : public QVector<QPersistentModelIndex>
{
public:
	QPersistentModelIndexVec() = default;

	/// Convert from a const QModelIndexList.
	QPersistentModelIndexVec(const QModelIndexList& mil);

	/// Conversion operator to a const QModelIndexList.
	operator const QModelIndexList() const;
};

/**
 * Convert a collection of QModelIndex's (e.g. a QModelIndexList) into a QPersistentModelIndexVec (a QVector of QPersistentIndexes).
 */
template <template<class> class CollectionType>
QPersistentModelIndexVec toQPersistentModelIndexVec(const CollectionType<QModelIndex>& mil)
{
	QPersistentModelIndexVec retval;

	for(auto i : mil)
	{
		retval.push_back(i);
	}
	return retval;
}

#endif /* LOGIC_PROXYMODELS_QPERSISTENTMODELINDEXVEC_H_ */
