/*
 * Copyright 2017, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include <QObject>
#include <QLineEdit>
#include <QRegularExpression>

class QAction;
class QActionGroup;


class FilterWidget : public QLineEdit
{
	Q_OBJECT

Q_SIGNALS:
	void filterChanged();

public:
	explicit FilterWidget(QWidget *parent = nullptr);

	QRegularExpression::PatternOptions caseSensitive() const;
    void setCaseSensitive(bool case_sensitive);

    /// @todo QT6 broke this, fix.
    // QRegExp::PatternSyntax patternSyntax() const;
    // void setPatternSyntax(QRegExp::PatternSyntax s);

public Q_SLOTS:

private:
	Q_DISABLE_COPY(FilterWidget)

	QAction *m_caseSensitivityAction;
	QActionGroup *m_patternGroup;
};

#endif // FILTERWIDGET_H
