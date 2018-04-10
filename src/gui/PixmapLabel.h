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

#ifndef PIXMAPLABEL_H
#define PIXMAPLABEL_H

#include <QLabel>

class PixmapLabel : public QLabel
{
	Q_OBJECT

public:
	PixmapLabel(QWidget* parent, Qt::WindowFlags flags = Qt::WindowFlags());

	// Note: Base class has a non-virtual method of this signature.
	void setPixmap(const QPixmap& pixmap);

	QSize minimumSizeHint() const override;

	bool hasHeightForWidth() const override;

	virtual int heightForWidth(int) const override;

protected:

	void resizeEvent(QResizeEvent *event) override;

	void updateMargins();

private:
	Q_DISABLE_COPY(PixmapLabel)

	int m_pixmap_w {0};
	int m_pixmap_h {0};

};

#endif // PIXMAPLABEL_H
