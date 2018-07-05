/*
 * Copyright 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#include "AMLMJobTests.h"

// Std C++
#include <type_traits>
#include <atomic>

#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>


// Qt5
#include <QString>
#include <QTest>


// Ours
#include "../future_type_traits.hpp"
#include "../function_traits.hpp"
#include <tests/TestHelpers.h>
#include "../AMLMJob.h"
#include <src/concurrency/DirectoryScanJob.h>


void AMLMJobTests::SetUp()
{
	GTEST_COUT << "SetUp()" << std::endl;
}

void AMLMJobTests::TearDown()
{
	GTEST_COUT << "TearDown()" << std::endl;
}

///
/// Test Cases
///


TEST_F(AMLMJobTests, ThisShouldPass)
{
	ASSERT_FALSE(has_finished(__PRETTY_FUNCTION__));
	ASSERT_TRUE(true);
	finished(__PRETTY_FUNCTION__);
	ASSERT_TRUE(has_finished(__PRETTY_FUNCTION__));
}

TEST_F(AMLMJobTests, CancelTest)
{
    ExtFutureWatcher<DirScanResult> watcher;

	// Dummy dir so the dir scanner job has something to chew on.
	QUrl dir_url = QUrl::fromLocalFile("~/");
    DirectoryScannerAMLMJobPtr dsj = DirectoryScannerAMLMJob::make_job(nullptr, dir_url,
	                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
	                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    // Connect signals and slots.
    connect_or_die(dsj, &DirectoryScannerAMLMJob::finished, [=](KJob* kjob){
        ASSERT_EQ(kjob, dsj);
        EXPECT_EQ(kjob->error(), KJob::KilledJobError);

        ;});
    connect_or_die(dsj, &DirectoryScannerAMLMJob::result, [=](KJob* kjob){
        ASSERT_EQ(kjob, dsj);

        EXPECT_EQ(kjob->error(), KJob::KilledJobError);

		;});

    watcher.setFuture(dsj->get_extfuture_ref());

    // Start the job.
    dsj->start();

    // Spin waiting for the job to complete.
    /// Don't think this is waiting.
    while(watcher.isRunning())
    {
        qDb() << "isRunning()";
        qApp->processEvents();
    }
    watcher.waitForFinished();

    // Cancel the job.
    dsj->kill();

//    ASSERT_EQ(dsj->get_extfuture_ref(), );
}
