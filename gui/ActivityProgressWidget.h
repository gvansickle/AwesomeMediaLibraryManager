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


#include <QtWidgets/QWidget>

class QProgressBar;
class QLabel;

class ActivityProgressWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ActivityProgressWidget(QWidget *parent, const Qt::WindowFlags &f = Qt::WindowFlags());
	virtual ~ActivityProgressWidget() override;

public slots:
	void onProgressRangeChanged(int minimum, int maximum);
	void onProgressTextChanged(const QString &progressText);
	void onProgressValueChanged(int progressValue);

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
