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

#ifndef DRAGDROPTREEVIEWSTYLEPROXY_H
#define DRAGDROPTREEVIEWSTYLEPROXY_H


#include <QStyle>
#include <QStyleOption>
#include <QProxyStyle>
#include <QPainter>

/**
 * A ProxyStyle class to draw the ItemView drop indicator in a more appropriate style for
 * TreeViews which are really more list-like.
 * Drop indicator will be a line spanning all columns.
 *
 * @note This is sort of a hack.  The Style shouldn't be the one figuring out the span of the indicator, but
 *       the QRect calculation of the indicator is buried so deep in QAbstractItemView::dragMoveEvent() (which handles
 *       Above/Below/etc.) that I can't find a better place to do this.
 */
class DragDropTreeViewStyleProxy : public QProxyStyle
{
	Q_OBJECT
public:

	explicit DragDropTreeViewStyleProxy(QStyle* style = Q_NULLPTR) : QProxyStyle(style) {}

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
    {
        if(element == QStyle::PE_IndicatorItemViewItemDrop && !option->rect.isNull())
        {
            // Drawing a drop indicator in a drop view.
            // widget will be the derived QAbstractItemView.

            painter->save();

            QStyleOption opt(*option);
            opt.rect.setLeft(0);
            if(widget)
            {
                opt.rect.setRight(widget->width()-1);
            }

            // Draw the drop indicator.
            drawIndicator(&opt, painter, widget);

            painter->restore();

            return;
        }
        else
        {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    }

private:
	Q_DISABLE_COPY(DragDropTreeViewStyleProxy)


	/**
	* Do the actual draw of the Drop Indicator.
	*/
    void drawIndicator(const QStyleOption *option, QPainter *painter, const QWidget *widget) const
    {
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Use the "Selected item" color.
        QColor c = option->palette.highlight().color();
        QPen pen(c);
        QBrush brush(c);

        pen.setWidth(3);
        painter->setPen(pen);
        painter->setBrush(brush);

        QPoint myPoint = QPoint(option->rect.topLeft().x(), option->rect.topLeft().y()+2);
        painter->drawLine(myPoint, option->rect.topRight());
    }
};

#endif // DRAGDROPTREEVIEWSTYLEPROXY_H
