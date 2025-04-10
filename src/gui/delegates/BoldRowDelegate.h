/*
 * Copyright 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef BOLDROWDELEGATE_H
#define BOLDROWDELEGATE_H

// Std C++
#include <set>

// Qt
#include <QStyledItemDelegate>


/**
 * Delegate which sets a given row's font to bold.
 */
class BoldRowDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit BoldRowDelegate(QObject* parent = nullptr);
	~BoldRowDelegate() override;



    void setRow(ssize_t row);

    void clearAll();
    void clearAllButOne();

protected:

	void initStyleOption(QStyleOptionViewItem *option,
						 const QModelIndex &index) const override;

Q_SIGNALS:
    void updateRequested();

private:
    ssize_t m_row {-1};

    std::set<int> m_bolded_rows;
};

Q_DECLARE_METATYPE(BoldRowDelegate)

#endif //BOLDROWDELEGATE_H
