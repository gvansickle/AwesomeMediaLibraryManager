/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef FRACTION_H
#define FRACTION_H

#include <QDebug>
#include <QMetaType>
#include <QString>

class Fraction
{
public:
    Fraction();
	Fraction(const QString& s);
	Fraction(const Fraction& other);
	Fraction(long num, long denom);
	~Fraction();


	QString toQString() const;

	operator QString() const;
	explicit operator qint64() const;
	explicit operator double() const;

	friend Fraction operator+(Fraction f1, Fraction f2);
	friend Fraction operator-(Fraction f1, Fraction f2);
	friend Fraction operator*(Fraction f1, Fraction f2);
	friend bool operator<(Fraction f1, Fraction f2);

private:
	qint64 m_numerator;
	qint64 m_denominator;
	double m_cheater;

	void reduce();
};

Q_DECLARE_METATYPE(Fraction);

Fraction operator+(Fraction f1, Fraction f2);
Fraction operator-(Fraction f1, Fraction f2);
Fraction operator*(Fraction f1, Fraction f2);
bool operator<(Fraction f1, Fraction f2);

QDebug operator<<(QDebug dbg, const Fraction& f);

#endif // FRACTION_H
