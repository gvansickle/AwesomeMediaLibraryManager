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

/**
 * @file SerializationExceptions.h
 */
#ifndef SRC_LOGIC_SERIALIZATION_SERIALIZATIONEXCEPTIONS_H_
#define SRC_LOGIC_SERIALIZATION_SERIALIZATIONEXCEPTIONS_H_

// Std C++
#include <string>

// Qt
#include <QException>

/**
 * These could probably be better done with templates.  This is quicker and dirtier though.
 */
#define M_AMLM_EXCEPTION_CTORS_DTORS(exception_class_name, base_class_name) \
	exception_class_name (const std::string & what_arg) : base_class_name (what_arg) {}; \
	exception_class_name (const char* what_arg) : base_class_name (what_arg) {}; \
	~exception_class_name () override = default;

#define M_QT_EXCEPTION_BOILERPLATE(exception_class_name) \
	void raise() const override { throw *this; } \
	exception_class_name *clone() const override { return new exception_class_name(*this); }

/**
 * Base class for any exceptions we'll derive.  Adds @a what() functionality to Qt's, which still
 * doesn't have it as of Qt6.9.
 */
class AMLMException : public QException
{
public:
	AMLMException(const std::string& what_arg) : m_what_str(what_arg) {};
	AMLMException(const char * what_arg) : m_what_str(what_arg) {};
	 ~AMLMException() override = default;

	const char* what() const noexcept override { return m_what_str.c_str(); }

	/// @name These two are for Qt.  Allows it to throw across threads.
	/// @{
	void raise() const override { throw *this; }
	AMLMException *clone() const override { return new AMLMException(*this); }
	/// @}

private:
	std::string m_what_str {};
};

/*
 *
 */
class SerializationException: public AMLMException
{
public:
	M_AMLM_EXCEPTION_CTORS_DTORS(SerializationException, AMLMException);

	M_QT_EXCEPTION_BOILERPLATE(SerializationException);
};

#endif /* SRC_LOGIC_SERIALIZATION_SERIALIZATIONEXCEPTIONS_H_ */
