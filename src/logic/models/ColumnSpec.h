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

#ifndef LOGIC_COLUMNSPEC_H_
#define LOGIC_COLUMNSPEC_H_

#include <QMetaEnum>
#include <QString>
#include <QStringList>



class SectionID
{
	Q_GADGET

public:
	enum Enumerator
	{
		Status,
		Title,
		Artist,
		Album,
		Length,
		MIMEType,
		Filename,
		PLAYLIST_1
	};

	Q_ENUM(Enumerator)

	SectionID() = default;
	explicit SectionID(int val) : m_val(val) {}
	SectionID(SectionID::Enumerator e) { m_val = e; }

	virtual operator int() const { return m_val; }

//	template<typename DerivedType>
//	operator SectionID() { return DerivedType(); }

	static const char* valueToKey(Enumerator val) { return QMetaEnum::fromType<SectionID::Enumerator>().valueToKey(val); }

	const char * valueToKey() const { return SectionID::valueToKey(static_cast<Enumerator>(m_val)); }

protected:
	int m_val {0};
};


struct ColumnSpec
{
	ColumnSpec(SectionID s, const QString& display_name, QStringList ml = {}, bool fit_col_width = false, bool default_to_hidden = false)
    {
        m_section_id = s;
		m_display_name = display_name;
		m_metadata_list = ml;
		m_should_fit_column_width_to_contents = fit_col_width;
		m_default_to_hidden = default_to_hidden;
	}

    SectionID m_section_id;
    QString m_display_name;
	QStringList m_metadata_list;
	bool m_should_fit_column_width_to_contents {false};
	bool m_default_to_hidden {false};
};

#endif /* LOGIC_COLUMNSPEC_H_ */
