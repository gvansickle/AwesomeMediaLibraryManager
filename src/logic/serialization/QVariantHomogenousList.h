/*
 * Copyright 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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
#include <deque>

// Qt
#include <QString>
#include <QVariantList>

// Ours.
// #include <ISerializable.h> //<< This includes this file.
#include <utils/DebugHelpers.h>
#include <future/guideline_helpers.h>


class QVariantHomogenousList //: public QVariantList
{
public:
	/// @name Member types
	/// @{
	using value_type = QVariant;
	using underlying_container_type = QList<value_type>;//std::deque<value_type>;
	using const_iterator = underlying_container_type::const_iterator;
	using iterator = underlying_container_type::iterator;
	/// @}

public:
	// Rule-of-Zero doesn't work here, probably Qt5.
	M_GH_RULE_OF_FIVE_DEFAULT_C21(QVariantHomogenousList);
	virtual ~QVariantHomogenousList() = default;

	QVariantHomogenousList(const QString& list_tag, const QString& list_item_tag)
	{
		set_tag_names(list_tag, list_item_tag);
	}

	// Assignment from other list types.
	template <class ListLike>
	QVariantHomogenousList& operator=(const ListLike& other)
	{
		this->clear();
		for(const auto& it : other)
		{
			this->push_back(it);
		}
		return *this;
	}

	/**
	 * We need to keep this public so that we can set the tags after e.g. an assignment from another
	 * QVar*List, which will also copy other's tags, which we may not want.
	 * @param list_tag
	 * @param list_item_tag
	 */
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

	void clear() noexcept { m_the_list.clear(); }

	void push_back( const QVariant& value ) { m_the_list.push_back(value); };

	const_iterator cbegin() const noexcept { return std::cbegin(m_the_list); }
	iterator begin() { return m_the_list.begin(); }
	const_iterator begin() const { return m_the_list.begin(); }
	const_iterator cend() const noexcept { return std::cend(m_the_list); }
	iterator end() { return m_the_list.end(); }
	const_iterator end() const { return m_the_list.end(); }

	long size() const noexcept { return m_the_list.size(); }
	bool empty() const noexcept { return m_the_list.empty(); }

	/**
	 * Conversion operator to a QVariant.
	 * @note This is deliberately not explicit so that it is a workalike to QList<QVariant> wrt QVariants.
	 */
	operator QVariant() const
	{
		return QVariant::fromValue(*this);
	}

protected:
	QString m_list_tag {};
	QString m_list_item_tag {};

	underlying_container_type m_the_list;
};

Q_DECLARE_METATYPE(QVariantHomogenousList);
// QVariantHomogenousList isn't a template, so this macro doesn't work:
// Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(QVariantHomogenousList);

#endif // QVARIANTHOMOGENOUSLIST_H
