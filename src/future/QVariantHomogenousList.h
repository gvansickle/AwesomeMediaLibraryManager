/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
 * @file QVariantHomogenousList.h
 */


#ifndef QVARIANTHOMOGENOUSLIST_H
#define QVARIANTHOMOGENOUSLIST_H

// Std C++
#include <utility> // For swap().

// Qt5
#include <QString>
#include <QVariantList>

// Ours.
#include <utils/DebugHelpers.h>

class QVariantHomogenousList : public QVariantList
{
public:
	// Rule-of-Zero doesn't work here, probably Qt5.
	QVariantHomogenousList() = default;
	QVariantHomogenousList(const QVariantHomogenousList& other) = default;
	QVariantHomogenousList& operator=(QVariantHomogenousList other)
	{
		// allow use of std::swap()...
		qDb() << "SWAPPING";
		using std::swap;
		// ...but select overloads first.
		swap(*this, other);
		return *this;
	}
	~QVariantHomogenousList() = default;

	QVariantHomogenousList(const QString& list_tag, const QString& list_item_tag)
	{
		set_tag_names(list_tag, list_item_tag);
	}

	void set_tag_names(const QString& list_tag, const QString& list_item_tag)
	{
		m_list_tag = list_tag;
		m_list_item_tag = list_item_tag;
	}

	const QString& get_list_tag() const
	{
		return m_list_tag;
	}

	const QString& get_list_item_tag() const
	{
		return m_list_item_tag;
	}

protected:
	QString m_list_tag;
	QString m_list_item_tag;
};

Q_DECLARE_METATYPE(QVariantHomogenousList);

#endif // QVARIANTHOMOGENOUSLIST_H
