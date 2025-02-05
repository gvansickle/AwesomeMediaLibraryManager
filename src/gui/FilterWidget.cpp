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

#include "FilterWidget.h"

//#include <nomocimpl.h>

#include <QIcon>
//#include <QPixmap>
#include <QImage>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QToolButton>
#include <QWidgetAction>
#include <QDebug>
#include <QRegularExpression>



FilterWidget::FilterWidget(QWidget *parent) : QLineEdit(parent), m_patternGroup(new QActionGroup(this))
{
	setPlaceholderText(tr("Enter search terms here"));

	setClearButtonEnabled(true);
	connect(this, &QLineEdit::textChanged, this, &FilterWidget::filterChanged);

	QMenu *menu = new QMenu(this);
	m_caseSensitivityAction = menu->addAction(tr("Case Sensitive"));
	m_caseSensitivityAction->setCheckable(true);
	connect(m_caseSensitivityAction, &QAction::toggled, this, &FilterWidget::filterChanged);

	menu->addSeparator();
	m_patternGroup->setExclusive(true);
	QAction *patternAction = menu->addAction(tr("Fixed String"));
	patternAction->setData(QVariant(int(QRegExp::FixedString)));
	patternAction->setCheckable(true);
	patternAction->setChecked(true);
	m_patternGroup->addAction(patternAction);
	patternAction = menu->addAction(tr("Regular Expression"));
	patternAction->setCheckable(true);
	patternAction->setData(QVariant(int(QRegExp::RegExp2)));
	m_patternGroup->addAction(patternAction);
	patternAction = menu->addAction("Wildcard");
	patternAction->setCheckable(true);
	patternAction->setData(QVariant(int(QRegExp::Wildcard)));
	m_patternGroup->addAction(patternAction);
	connect(m_patternGroup, &QActionGroup::triggered, this, &FilterWidget::filterChanged);

	const QIcon icon = QIcon::fromTheme("edit-find");
	QToolButton *optionsButton = new QToolButton;
#ifndef QT_NO_CURSOR
	optionsButton->setCursor(Qt::ArrowCursor);
#endif
	optionsButton->setFocusPolicy(Qt::NoFocus);
	optionsButton->setStyleSheet("* { border: none; }");
	optionsButton->setIcon(icon);
	optionsButton->setMenu(menu);
	optionsButton->setPopupMode(QToolButton::InstantPopup);

	QWidgetAction *optionsAction = new QWidgetAction(this);
	optionsAction->setDefaultWidget(optionsButton);
	addAction(optionsAction, QLineEdit::LeadingPosition);
}

QRegularExpression::PatternOptions FilterWidget::caseSensitivity() const
{
    return m_caseSensitivityAction->isChecked() ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption;
}

void FilterWidget::setCaseSensitivity(Qt::CaseSensitivity cs)
{
	m_caseSensitivityAction->setChecked(cs == Qt::CaseSensitive);
}

static inline QRegExp::PatternSyntax patternSyntaxFromAction(const QAction *a)
{
	return static_cast<QRegExp::PatternSyntax>(a->data().toInt());
}

QRegExp::PatternSyntax FilterWidget::patternSyntax() const
{
	return patternSyntaxFromAction(m_patternGroup->checkedAction());
}

void FilterWidget::setPatternSyntax(QRegExp::PatternSyntax s)
{
	auto actions = m_patternGroup->actions();
	for(QAction *a : std::as_const(actions))
	{
		if (patternSyntaxFromAction(a) == s)
		{
			a->setChecked(true);
			break;
		}
	}
}
