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

#include <QLineEdit>
#include <QRegExp>

class QAction;
class QActionGroup;

Q_DECLARE_METATYPE(QRegExp::PatternSyntax)

class FilterWidget : public QLineEdit
{
	Q_OBJECT

	Q_PROPERTY(Qt::CaseSensitivity caseSensitivity READ caseSensitivity WRITE setCaseSensitivity)
	Q_PROPERTY(QRegExp::PatternSyntax patternSyntax READ patternSyntax WRITE setPatternSyntax)

public:
	explicit FilterWidget(QWidget *parent = 0);

	Qt::CaseSensitivity caseSensitivity() const;
	void setCaseSensitivity(Qt::CaseSensitivity);

	QRegExp::PatternSyntax patternSyntax() const;
	void setPatternSyntax(QRegExp::PatternSyntax);

signals:
	void filterChanged();

public slots:

private:
	QAction *m_caseSensitivityAction;
	QActionGroup *m_patternGroup;
};

#endif // FILTERWIDGET_H
