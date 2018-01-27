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

#include "PixmapLabel.h"

PixmapLabel::PixmapLabel(QWidget* parent, Qt::WindowFlags flags) : QLabel(parent, flags)
{
	setScaledContents(true);
	setFrameStyle((QFrame::Panel | QFrame::Sunken));
	setAlignment(Qt::AlignCenter);
}

void PixmapLabel::setPixmap(const QPixmap& pixmap)
{
	m_pixmap_w = pixmap.width();
	m_pixmap_h = pixmap.height();

	updateMargins();
	QLabel::setPixmap(pixmap);
}

QSize PixmapLabel::minimumSizeHint() const
{
	return QSize(0,0);
}

bool PixmapLabel::hasHeightForWidth() const
{
	return pixmap() != nullptr;
}

int PixmapLabel::heightForWidth(int) const
{
	if(pixmap() != nullptr && pixmap()->width() > 0)
	{
		return int(width() * pixmap()->height() / pixmap()->width());
	}
	else
	{
		return 0;
	}
}

void PixmapLabel::resizeEvent(QResizeEvent* event)
{
	updateMargins();
	QLabel::resizeEvent(event);
}

void PixmapLabel::updateMargins()
{
	if(m_pixmap_w <= 0 || m_pixmap_h <= 0)
	{
		return;
	}

	auto w = width();
	auto h = height();

	if(w <= 0 || h <= 0)
	{
		return;
	}

	if(w * m_pixmap_h > h * m_pixmap_w)
	{
		auto m = (w - (m_pixmap_w * h / m_pixmap_h))/2;
		setContentsMargins(m, 0.0, m, 0.0);
	}
	else
	{
		auto m = (h - (m_pixmap_h * w / m_pixmap_w))/2;
		setContentsMargins(0, m, 0 , m);
	}
}


