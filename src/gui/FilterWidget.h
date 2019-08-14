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

#ifndef FILTERWIDGET_H
#define FILTERWIDGET_H

//#include <nomocdefs.h>

#include <QObject>
#include <QLineEdit>
#include <QRegExp>

class QAction;
class QActionGroup;

Q_DECLARE_METATYPE(QRegExp::PatternSyntax)

class FilterWidget : public QLineEdit
{
	//W_OBJECT(FilterWidget)
	Q_OBJECT

/// EXP
	Q_PROPERTY(Qt::CaseSensitivity caseSensitivity READ caseSensitivity WRITE setCaseSensitivity NOTIFY filterChanged)
	Q_PROPERTY(QRegExp::PatternSyntax patternSyntax READ patternSyntax WRITE setPatternSyntax NOTIFY filterChanged)
//	Q_PROPERTY(Qt::CaseSensitivity caseSensitivity MEMBER m_caseSensitivityAction NOTIFY filterChanged)
//	Q_PROPERTY(QRegExp::PatternSyntax patternSyntax MEMBER m_READ patternSyntax WRITE setPatternSyntax)


Q_SIGNALS:
	void filterChanged();
    //W_SIGNAL(filterChanged)

public:
	explicit FilterWidget(QWidget *parent = nullptr);

	Qt::CaseSensitivity caseSensitivity() const;
	void setCaseSensitivity(Qt::CaseSensitivity cs);

	QRegExp::PatternSyntax patternSyntax() const;
	void setPatternSyntax(QRegExp::PatternSyntax s);

	//W_PROPERTY(Qt::CaseSensitivity, caseSensitivity READ caseSensitivity WRITE setCaseSensitivity)
	//W_PROPERTY(QRegExp::PatternSyntax, patternSyntax READ patternSyntax WRITE setPatternSyntax)

public Q_SLOTS:

private:
	Q_DISABLE_COPY(FilterWidget)

	QAction *m_caseSensitivityAction;
	QActionGroup *m_patternGroup;
};

#endif // FILTERWIDGET_H
