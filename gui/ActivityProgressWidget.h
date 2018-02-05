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

#ifndef AWESOMEMEDIALIBRARYMANAGER_ACTIVITYPROGRESSWIDGET_H
#define AWESOMEMEDIALIBRARYMANAGER_ACTIVITYPROGRESSWIDGET_H

#include <nomocdefs.h>

#include <QtWidgets/QWidget>

class QProgressBar;
class QLabel;

class ActivityProgressWidget : public QWidget
{
	W_OBJECT(ActivityProgressWidget)

public:
	explicit ActivityProgressWidget(QWidget *parent, const Qt::WindowFlags &f = Qt::WindowFlags());
	virtual ~ActivityProgressWidget() override;

public Q_SLOTS:

	void onProgressChanged(int min, int val, int max, QString text);

	/// @note This is only needed for the old-style connection syntax and/or QML exposure.
//	W_SLOT(onProgressChanged)

private:
	Q_DISABLE_COPY(ActivityProgressWidget)

	QLabel* m_current_activity_label;
	QLabel* m_text_status_label;
	QProgressBar* m_progress_bar;

	int m_last_min {0};
	int m_last_max {0};
	int m_last_val {0};

	QString createTextStatusString();

};


#endif //AWESOMEMEDIALIBRARYMANAGER_ACTIVITYPROGRESSWIDGET_H
