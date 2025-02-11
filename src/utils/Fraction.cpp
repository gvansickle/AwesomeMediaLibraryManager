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

#include <config.h>

#include "Fraction.h"

/// Std C++
#include <numeric>

/// Qt5
#include <QString>
#include <QDataStream>

/// Ours
#include "utils/TheSimplestThings.h"
#include "utils/RegisterQtMetatypes.h"


AMLM_QREG_CALLBACK([](){
    qIn() << "Registering Fraction";
    /// @todo This isn't working (in or out) with QVariant.
    qRegisterMetaType<Fraction>();
	QMetaType::registerConverter<Fraction, QString> (&Fraction::toQString);
	//	qRegisterMetaTypeStreamOperators<Fraction>();
//	QMetaType::registerConverter<Fraction, QString>([](const Fraction& frac){ return frac.toQString(); });
//	QMetaType::registerConverter<QString, Fraction>([](const QString& str){
//		return Fraction(str);
//	});
});

/// Calculate the Greatest Common Divisor.
static qint64 gcd(qint64 x, qint64 y)
{
	for (;;)
	{
		if (x == 0) return y;
		y %= x;
		if (y == 0) return x;
		x %= y;
	}
}

static qint64 lcm(qint64 x, qint64 y)
{
	qint64 temp = gcd(x,y);
	return temp ? (x/temp*y) : 0;
}

Fraction::Fraction()
{
	m_numerator = 0;
	m_denominator = 1;
	m_cheater = 0.0;
}

Fraction::Fraction(const QString& s)
{
	QString num_s, den_s;
	num_s = s.section('/', 0, 0);
	den_s = s.section('/', 1, 1);
	m_numerator = num_s.toLongLong();
	m_denominator = den_s.toLongLong();
	//Q_ASSERT(m_denominator != 0);
	//Q_ASSERT(m_denominator == 1 || m_denominator == 25 || m_denominator == 75);
	m_cheater = num_s.toDouble()/den_s.toDouble();
	//qDebug() << "From string:" << num_s << den_s << m_numerator << "/" << m_denominator << "(" << m_cheater << ")";
}

Fraction::Fraction(const Fraction& other)
	: m_numerator(other.m_numerator), m_denominator(other.m_denominator), m_cheater(other.m_cheater)
{
	//Q_ASSERT(m_denominator != 0);
	//Q_ASSERT(m_denominator == 1 || m_denominator == 25 || m_denominator == 75);

}

Fraction::Fraction(long num, long denom) : m_numerator(num), m_denominator(denom), m_cheater(double(num)/double(denom))
{
	//Q_ASSERT(m_denominator != 0);
	//Q_ASSERT(m_denominator == 1 || m_denominator == 25 || m_denominator == 75);
}

Fraction::~Fraction()
{

}

QString Fraction::toQString() const
{
	QString retval;
	retval = QString("%1/%2").arg(m_cheater).arg("1");
	//qDebug() << "toQString:" << retval;
	return retval; //QString("%1/%2").arg(m_cheater).arg("1");//.arg(m_numerator).arg(m_denominator);
}

Fraction::operator double() const
{
	return m_cheater;
}

void Fraction::reduce()
{
	auto greatest_common_factor = gcd(m_numerator, m_denominator);
	qDebug() << "num/denum/gcf:" << m_numerator << m_denominator << greatest_common_factor;
	m_numerator /= greatest_common_factor;
	m_denominator /= greatest_common_factor;
}

Fraction::operator qint64() const
{
	//qDebug() << std::gcd();
	return qint64(m_cheater);//m_numerator/m_denominator);
}

Fraction::operator QString() const
{
	return toQString();
}

QDebug operator<<(QDebug dbg, const Fraction& f)
{
	dbg << QString(f);
	return dbg;
}

Fraction operator+(Fraction f1, Fraction f2)
{
	Fraction retval = Fraction(f1.m_cheater+f2.m_cheater, 1);
	Q_ASSERT(retval.m_cheater >= 0.0);
	return retval;
//	qint64 common_denominator = f1.m_denominator * f2.m_denominator;

//	auto retval = Fraction((f1.m_numerator*f2.m_denominator + f2.m_numerator*f1.m_denominator)/common_denominator, 1);

//	retval.reduce();

//	return retval;
}

Fraction operator-(Fraction f1, Fraction f2)
{
	Fraction retval = Fraction(f1.m_cheater-f2.m_cheater, 1);
	Q_ASSERT(retval.m_cheater >= 0.0);
	return retval;
//	qint64 common_denominator = f1.m_denominator * f2.m_denominator;

//	auto retval = Fraction((f1.m_numerator*f2.m_denominator - f2.m_numerator*f1.m_denominator)/common_denominator, 1);

//	retval.reduce();

//	return retval;
}

Fraction operator*(Fraction f1, Fraction f2)
{
	Fraction retval(f1.m_cheater*f2.m_cheater, 1);
	Q_ASSERT(retval.m_cheater >= 0.0);
	return retval;
//	Fraction retval;

//	retval.m_numerator = f1.m_numerator * f2.m_numerator;
//	retval.m_denominator = f1.m_denominator * f2.m_denominator;
//	retval.reduce();
//	return retval;
}

bool operator<(Fraction f1, Fraction f2)
{
	return f1.m_cheater < f2.m_cheater;
	return qint64(f1) < qint64(f2);
}


QDataStream& operator<<(QDataStream& out, const Fraction& f)
{
	out << f.m_numerator << f.m_denominator << f.m_cheater;
	return out;
}

QDataStream& operator>>(QDataStream& in, Fraction& f)
{
	in >> f.m_numerator >> f.m_denominator >> f.m_cheater;
	return in;
}
