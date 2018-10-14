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

#include <utils/DebugHelpers.h>

PixmapLabel::PixmapLabel(QWidget* parent, Qt::WindowFlags flags) : QLabel(parent, flags)
{
	/**
	 * More evidence that it's not really the 21st century.  From @link https://www.ics.com/designpatterns/solutions/threads.html:
	 * "Do not access the GUI (this includes any QWidget-derived class, QPixmap, and other graphics-card specific classes) from any thread other than the main thread.
	 * This includes read access like querying the text entered into a QLineEdit.
	 * For processing images in other threads, use QImage instead of QPixmap."
	 * Official docs say the same thing.
	 * So we'll assert if we're not in the GUI thread.
	 */
	AMLM_ASSERT_IN_GUITHREAD();

	setScaledContents(true);
	setFrameStyle((QFrame::Panel | QFrame::Sunken));
	setAlignment(Qt::AlignCenter);
}

void PixmapLabel::setPixmap(const QPixmap& pixmap)
{
	AMLM_ASSERT_IN_GUITHREAD();

	m_pixmap_w = pixmap.width();
	m_pixmap_h = pixmap.height();

	updateMargins();
	QLabel::setPixmap(pixmap);
}

QSize PixmapLabel::minimumSizeHint() const
{
	AMLM_ASSERT_IN_GUITHREAD();

	return QSize(0,0);
}

bool PixmapLabel::hasHeightForWidth() const
{
	AMLM_ASSERT_IN_GUITHREAD();

	return pixmap() != nullptr;
}

int PixmapLabel::heightForWidth(int) const
{
	AMLM_ASSERT_IN_GUITHREAD();

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
	AMLM_ASSERT_IN_GUITHREAD();

	updateMargins();
	QLabel::resizeEvent(event);
}

void PixmapLabel::updateMargins()
{
	AMLM_ASSERT_IN_GUITHREAD();

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


