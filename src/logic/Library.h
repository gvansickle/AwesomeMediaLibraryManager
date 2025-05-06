/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef LIBRARY_H
#define LIBRARY_H

/// @file

// Std C++
#include <utility>

// Qt
#include <QUrl>

// Ours.
#include <logic/serialization/ISerializable.h>

#include "LibraryEntry.h"

class QFileDevice;

class Library : public virtual ISerializable
{
	Q_GADGET
public:
	M_GH_RULE_OF_FIVE_DEFAULT_C21(Library);
	~Library() override = default;

	void clear();
	void setRootUrl(const QUrl& url) { m_root_url = url; }
	QUrl getRootUrl() const { return m_root_url; }
	QString getLibraryName() const;

	void addNewEntries(std::vector<std::shared_ptr<LibraryEntry> > entries);
	void removeRow(int row);
	void removeRows(int row, int count);
	void insertEntry(int row, std::shared_ptr<LibraryEntry> entry);
	void replaceEntry(int row, std::shared_ptr<LibraryEntry> entry);

	std::shared_ptr<LibraryEntry> operator[](size_t index) const;

	bool areAllEntriesFullyPopulated() const;
	qint64 getNumEntries() const;
	qint64 getNumPopulatedEntries() const;

	/// @name Serialization
	/// @{
	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;
	/// @}

private:
	void addingEntry(const LibraryEntry* entry);
	void removingEntry(const LibraryEntry* entry);

//private:
public:

	QUrl m_root_url;

	qint64 m_num_unpopulated {0};
	qint64 m_num_populated {0};

	std::vector<std::shared_ptr<LibraryEntry>> m_lib_entries;
};

Q_DECLARE_METATYPE(Library);
Q_DECLARE_METATYPE(Library*);

// QDataStream &operator<<(QDataStream &out, const Library &myObj);
// QDataStream &operator>>(QDataStream &in, Library &myObj);

#endif // LIBRARY_H
