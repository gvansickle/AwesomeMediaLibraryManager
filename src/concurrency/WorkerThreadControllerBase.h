/*
 * Copyright 2019 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

// Qt5
#include <QObject>
#include <QThread>

// Ours
#include <future/guideline_helpers.h>

/**
 * @file WorkerThreadControllerBase.h
 */
#ifndef SRC_CONCURRENCY_WORKERTHREADCONTROLLERBASE_H_
#define SRC_CONCURRENCY_WORKERTHREADCONTROLLERBASE_H_

namespace ExtAsync
{

/*
 *
 */
class WorkerThreadControllerBase : public QObject
{
	Q_OBJECT

Q_SIGNALS:
	void operate(const QString& t);

public:
//	M_GH_RULE_OF_FIVE_DELETE_C21(WorkerThreadControllerBase);
	WorkerThreadControllerBase();
	~WorkerThreadControllerBase() override;

public Q_SLOTS:

	void SLOT_handle_results(const QString&);

private:
	Q_DISABLE_COPY(WorkerThreadControllerBase);
	QThread m_worker_thread;
};

} /* namespace ExtAsync */

#endif /* SRC_CONCURRENCY_WORKERTHREADCONTROLLERBASE_H_ */
