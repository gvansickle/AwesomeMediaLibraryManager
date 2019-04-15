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

//#include <gtest/gtest.h>
//#include <gmock/gmock-matchers.h>


// Qt5
#include <QString>
#include <QTest>
#include <QSignalSpy>
#include <QObject>

// Ours
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <tests/TestHelpers.h>
#include "../AMLMJob.h"
#include <logic/jobs/DirectoryScanJob.h>


////////////////////////

#include "AMLMApp.h"

class ProjectFolderItem;
class IProjectFileManager;

/**
 * Test KJob-derived class.
 */
class ImportProjectJob: public KJob
{
    Q_OBJECT
public:
    ImportProjectJob(ProjectFolderItem *folder, IProjectFileManager *importer);
    ~ImportProjectJob() override;

public:
    void start() override;
    bool doKill() override;

public:
	// Added for test development.
	/// @note Currently unused.
    const QFuture<void> get_extfuture() const;

private Q_SLOTS:
    void importDone();
    void importCanceled();
    void aboutToShutdown();

private:
    class ImportProjectJobPrivate* const d;
    friend class ImportProjectJobPrivate;
};

class ProjectFolderItem
{
public:
    int m_num_subdirs = 4;
};

class IProjectFileManager
{

};

class ImportProjectJobPrivate
{
public:
    ImportProjectJobPrivate() {}

//    ProjectFolderItem *m_folder;
//    IProjectFileManager *m_importer;
    QFutureWatcher<void> *m_watcher{};
//    QPointer<IProject> m_project;
    bool cancel = false;

    void import(ProjectFolderItem* /*folder*/)
    {
        int loop_counter = 0;
        int max_loops = 10;
        while(!cancel)
        {
            TC_Sleep(100);
			AMLMTEST_COUT << "LOOP:" << loop_counter;
            loop_counter++;
            if(loop_counter > max_loops)
            {
				AMLMTEST_COUT << "EXITING LOOP";
                break;
            }
        }
#if 0
        Q_FOREACH(ProjectFolderItem* sub, m_importer->parse(folder))
        {
            if(!cancel) {
                import(sub);
            }
        }
#endif
    }
};

ImportProjectJob::ImportProjectJob(ProjectFolderItem *folder, IProjectFileManager *importer)
    : KJob(nullptr), d(new ImportProjectJobPrivate )
{
//    d->m_importer = importer;
//    d->m_folder = folder;
//    d->m_project = folder->project();

//    setObjectName(QString("Project Import: %1", d->m_project->name()));

    /// @note GRVS Moved here to try to mitigate against cancel-before-start segfaults.
    /// Seems to work.  We could get a doKill() call at any time after we leave this constructor.
    d->m_watcher = new QFutureWatcher<void>();

    setObjectName(QString("ImportProjectJob"));
    connect(AMLMApp::instance(), &AMLMApp::aboutToShutdown,
            this, &ImportProjectJob::aboutToShutdown);
}

ImportProjectJob::~ImportProjectJob()
{
    delete d;
}

void ImportProjectJob::start()
{
    connect(d->m_watcher, &QFutureWatcher<void>::finished, this, &ImportProjectJob::importDone);
    connect(d->m_watcher, &QFutureWatcher<void>::canceled, this, &ImportProjectJob::importCanceled);
    // Starting QtConcurrent::run().
    QFuture<void> f = QtConcurrent::run(d, &ImportProjectJobPrivate::import, nullptr /*d->m_folder*/);
    /// @note State appears to be Running|Started here.
//    AMLMTEST_COUT << "Running import job, returned future state:" << state(f);
    d->m_watcher->setFuture(f);
}

void ImportProjectJob::importDone()
{
    d->m_watcher->deleteLater(); /* Goodbye to the QFutureWatcher */

    emitResult();
}

bool ImportProjectJob::doKill()
{
    d->m_watcher->cancel();
    d->cancel=true;

    setError(1);
    setErrorText("Project import canceled.");

    d->m_watcher->waitForFinished();
    return true;
}

void ImportProjectJob::aboutToShutdown()
{
    kill();
}

void ImportProjectJob::importCanceled()
{
    d->m_watcher->deleteLater();
}

const QFuture<void> ImportProjectJob::get_extfuture() const
{
    return d->m_watcher->future();
}

////////////////////////


class TestAMLMJob1;
using TestAMLMJob1Ptr = QPointer<TestAMLMJob1>;
/**
 * Test job derived from AMLMJob.
 */
class TestAMLMJob1 : public AMLMJobT<ExtFuture<int>>, public UniqueIDMixin<TestAMLMJob1>
{
	Q_OBJECT

    using BASE_CLASS = AMLMJobT<ExtFuture<int>>;

	/**
	 * @note CRTP: Still need this to avoid ambiguous name resolution.
	 * @see https://stackoverflow.com/a/46916924
	 */
	using UniqueIDMixin<TestAMLMJob1>::uniqueQObjectName;

protected:
    explicit TestAMLMJob1(QObject* parent) : BASE_CLASS(parent)
	{
		// Set our object name.
		setObjectName(uniqueQObjectName());
        setCapabilities(KJob::Capability::Killable);
	}

public:
    /// @name Public types
    /// @{
    using ExtFutureType = ExtFuture<int>;
    /// @}

    ~TestAMLMJob1() override = default;

    static TestAMLMJob1Ptr make_job(QObject *parent)
    {
        TestAMLMJob1Ptr retval = new TestAMLMJob1(parent);
        return retval;
    }

    std::atomic_int m_counter {0};

protected:

    void runFunctor() override
    {
        SCOPED_TRACE("InFunctor");

        // Do some work...
        for(int i = 0; i<10; i++)
        {
			AMLMTEST_COUT << "Sleeping for 1 second\n";
            TC_Sleep(1000);

			AMLMTEST_COUT << "Incementing counter\n";
            m_counter++;

			AMLMTEST_COUT << "Reporting counter result\n";
            m_ext_future.reportResult(m_counter);

			if(m_ext_future.HandlePauseResumeShouldICancel())
            {
                // We've been cancelled.
                qIno() << "CANCELLED";
                m_ext_future.reportCanceled();
                break;
            }
        }

//        setSuccessFlag(!wasCancelRequested());

        m_ext_future.reportFinished();

		AMLMTEST_COUT << "Returning, m_ext_future:" << m_ext_future;
    }
};

///
/// Test Cases
///

INSTANTIATE_TEST_CASE_P(TrueFalseParameterizedTests,
                        AMLMJobTestsParameterized,
                        ::testing::Bool());

TEST_F(AMLMJobTests, ThisShouldPass) // NOLINT
{
    TC_ENTER();

    EXPECT_TRUE(true);

    TC_EXIT();
}

TEST_P(AMLMJobTestsParameterized, ImportProjectJobSyncExecPAutoDelete) // NOLINT
{
    TC_ENTER();

	//    QFETCH(bool, autodelete);
	bool autodelete = GetParam();
	AMLMTEST_COUT << "Autodelete?:" << autodelete;

    auto j = new ImportProjectJob(nullptr, nullptr);
	j->setAutoDelete(autodelete);

	M_QSIGNALSPIES_SET(j);

	// Run the job, should block.
    bool status = j->exec();

    EXPECT_EQ(status, true);

	// Pump the event loop for a while to let the signals propagate.
	TC_Wait(10000);

	AMLMTEST_COUT << "FINISHED CT:" << kjob_finished_spy.count();
	AMLMTEST_COUT << "RESULT CT:" << kjob_result_spy.count();

    EXPECT_EQ(kjob_finished_spy.count(), 1);
	// May not get a KJob result signal.
//    EXPECT_EQ(kjob_result_spy.count(), 1);

	if(!autodelete)
	{
		// Not autodelete.
		j->deleteLater();
	}

	// Wait for the delete to happen.
	M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();

    TC_EXIT();
}

TEST_P(AMLMJobTestsParameterized, DISABLED_SynchronousExecTestAMLMJob1PAutoDelete)
{
    TC_ENTER();

    //    QFETCH(bool, autodelete);
    bool autodelete = GetParam();

	AMLMTEST_COUT << "Autodelete?:" << autodelete;

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(qApp);
    j->setAutoDelete(autodelete);

    M_QSIGNALSPIES_SET(j);

    bool status = j->exec();

    EXPECT_EQ(status, true);

	// Pump the event loop for a while to let the signals propagate.
	TC_Wait(10000);

	AMLMTEST_COUT << "FINISHED CT:" << kjob_finished_spy.count();
	AMLMTEST_COUT << "RESULT CT:" << kjob_result_spy.count();

    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 1);

    if(!autodelete)
    {
        // Not autodelete.
        j->deleteLater();
    }

    // Wait for the delete to happen.
    M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();

    TC_EXIT();
}

TEST_P(AMLMJobTestsParameterized, DISABLED_AsynchronousExecTestAMLMJob1PAutoDelete)
{
    TC_ENTER();

    //    QFETCH(bool, autodelete);
    bool autodelete = GetParam();

	AMLMTEST_COUT << "Autodelete?:" << autodelete;

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(amlmApp);
    j->setAutoDelete(autodelete);

    M_QSIGNALSPIES_SET(j);

    // Start the job asynchronously.
    j->start();

    // Wait for successful KJob completion.
    EXPECT_TRUE(kjob_finished_spy.wait());
//    EXPECT_TRUE(kjob_result_spy.wait());

	AMLMTEST_COUT << "FINISHED CT:" << kjob_finished_spy.count();
	AMLMTEST_COUT << "RESULT CT:" << kjob_result_spy.count();

    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 1);

    if(!autodelete)
    {
        // Not autodelete.
        j->deleteLater();
    }

    // Wait for the delete to happen.
    M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();

    TC_EXIT();
}

#if 0 // OBSOLETE
TEST_F(AMLMJobTests, DISABLED_ThenTest) // NOLINT
{
    TC_ENTER();

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(qApp);
    j->setAutoDelete(false);

	M_QSIGNALSPIES_SET(j);

	j->then(qApp, [=](TestAMLMJob1* j_kjob) -> void {
		if(j->error() != 0)
        {
            // Error.
			AMLMTEST_COUT << "ASYNC JOB FAILED:\r\n"; // << j_kjob->error() << ":" << j_kjob->errorText() << ":" << j_kjob->errorString();
        }
        else
        {
			// Succeeded, get the data.
			AMLMTEST_COUT << "ASYNC JOB COMPLETE:\r\n";// << j_kjob;
            int new_val = j_kjob->get_extfuture().qtget_first();
        }
    });
    // Start the job.
    j->start();

    // Wait for job to finish.
    EXPECT_TRUE(QTest::qWaitFor([&](){return kjob_result_spy.count() == 1; }, 15000));
//    while(!j->isFinished())
//    {
//    }
    j->get_extfuture().waitForFinished();

	AMLMTEST_COUT << "JOB EXTFUTURE:" << j->get_extfuture().state() << "\n";
    QList<int> extf_int = j->get_extfuture().get();

    EXPECT_EQ(extf_int.size(), 10);

    TC_EXIT();
}
#endif


TEST_P(AMLMJobTestsParameterized, DISABLED_DirScanCancelTestPAutodelete) // NOLINT
{
    TC_ENTER();

//    QFETCH(bool, autodelete);
    bool autodelete = GetParam();

	// Dummy dir so the dir scanner job has something to chew on.
    QUrl dir_url = QUrl::fromLocalFile("/");

#if 0 // OBSOLETE
    DirectoryScannerAMLMJobPtr dsj = DirectoryScannerAMLMJob::make_job(amlmApp, dir_url,
	                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
	                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
#endif
	// Run the directory scan in another thread.
	ExtFuture<DirScanResult> dirresults_future = ExtAsync::qthread_async_with_cnr_future(DirScanFunction, nullptr,
																						 dir_url,
																						 QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
																						 QDir::Filters(QDir::Files |
																									   QDir::AllDirs |
																									   QDir::NoDotAndDotDot),
																						 QDirIterator::Subdirectories);
	auto dsj = make_async_AMLMJobT(dirresults_future);

    M_QSIGNALSPIES_SET(dsj);

    ASSERT_TRUE(dsj->isAutoDelete());

    // Set autodelete or not.
	AMLMTEST_COUT << "Setting Autodelete to:" << autodelete;
    dsj->setAutoDelete(autodelete);

    // Connect signals and slots.
    connect_or_die(dsj, &DirectoryScannerAMLMJob::finished, qApp, [=](KJob* kjob){
		AMLMTEST_COUT << "GOT FINISHED";
        EXPECT_EQ(kjob, dsj);
        EXPECT_EQ(kjob->error(), KJob::KilledJobError);
        ;});
    connect_or_die(dsj, &DirectoryScannerAMLMJob::result, qApp, [=](KJob* kjob){
		AMLMTEST_COUT << "GOT RESULT";
        EXPECT_EQ(kjob, dsj);
        EXPECT_EQ(kjob->error(), KJob::KilledJobError);
        ;});

    // Dump some info.
    dsj->dumpObjectInfo();

    // Start the job.
    dsj->start();

    // Cancel the job after 1 sec of scanning.
    TC_Wait(1000);

    ASSERT_NE(dsj, nullptr);
    dsj->kill();
    ASSERT_NE(dsj, nullptr);

    // Wait for the cancel to finish.
    // We make sure it was a cancel in the result handler above.
    EXPECT_TRUE(kjob_finished_spy.wait());

	AMLMTEST_COUT << "FINISHED CT:" << kjob_finished_spy.count();
	AMLMTEST_COUT << "RESULT CT:" << kjob_result_spy.count();

//    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 1);

    if(!autodelete)
    {
        dsj->deleteLater();
    }

    // Wait for the (auto-)delete to happen.
	/// @note We have to pump the event loop (with TC_Wait() here) for a little while
	/// to give the deleteLater() event-loop cycles to happen.  QSignalSpy() is broken
	/// in this case for some reason, don't have the link handy.
	AMLMTEST_COUT << "Waiting for destroyed signal...";
//    TC_Wait(100);
	M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();
	AMLMTEST_COUT << "Done Waiting for destroyed signal.";

    TC_EXIT();
}

TEST_P(AMLMJobTestsParameterized, DISABLED_CancelTestPAutoDelete) // NOLINT
{
    TC_ENTER();

	//    QFETCH(bool, autodelete);
	bool autodelete = GetParam();

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(nullptr);

	M_QSIGNALSPIES_SET(j);

	j->setAutoDelete(autodelete);

    connect_or_die(j, &KJob::finished, qApp, [&](KJob* kjob){
        qDb() << "GOT SIGNAL FINISHED:" << kjob;
                });

    ExtFuture<int> ef = j->get_extfuture();

    // Start the job.
	AMLMTEST_COUT << "START\n";
    j->start();

    EXPECT_TRUE(ef.isStarted()) << ef.state();

    // Let it run for a while.
    TC_Sleep(500);
    EXPECT_EQ(j->m_counter, 0);
    TC_Sleep(700);
    EXPECT_EQ(j->m_counter, 1);
    TC_Sleep(500);

    // Should not be canceled or finished yet.
    EXPECT_FALSE(ef.isCanceled()) << ef;
    EXPECT_FALSE(ef.isFinished()) << ef;

    // Cancel the running job.
	AMLMTEST_COUT << "CANCELING JOB\n";
    bool kill_succeeded = j->kill(KJob::KillVerbosity::Quietly);

	/// @todo j is now going to be deleteLater()'ed if it's autoDelete.

    EXPECT_TRUE(kill_succeeded) << ef.state();
    EXPECT_TRUE(ef.isCanceled()) << ef.state();
    EXPECT_EQ(j->error(), KJob::KilledJobError);

    // Wait for the KJob to signal that it's finished.
    // Won't get a result() signal here because it's kill()'ed Quietly.
    EXPECT_TRUE(kjob_finished_spy.wait());
    EXPECT_FALSE(kjob_result_spy.wait(500));

	if(!autodelete)
	{
		j->deleteLater();
	}

	M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();

    TC_EXIT();
}

TEST_F(AMLMJobTests, DISABLED_CancelBeforeStart) // NOLINT
{
    TC_ENTER();

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(nullptr);
	j->setAutoDelete(true);

	M_QSIGNALSPIES_SET(j);

//    ExtFuture<int> ef = j->get_extfuture();

//    EXPECT_TRUE(ef.isStarted()) << ef.state();
//    EXPECT_FALSE(ef.isRunning()) << ef.state();

    // Job hasn't started yet (we never called start()), kill it.
	TCOUT << "CANCELING JOB";
    bool kill_succeeded = j->kill();
	TCOUT << "kill() returned";

    // j is now probably going to be deleteLater()'ed.

	EXPECT_TRUE(kill_succeeded);// << ef;
    /// @note No notification to the Future since no watcher has been set up yet.
//    EXPECT_TRUE(ef.isCanceled()) << ef;
//    EXPECT_EQ(j->error(), KJob::KilledJobError);

//    j->start();

//    TC_Sleep(500);
//    EXPECT_EQ(j->m_counter, 0);
//    TC_Sleep(700);
//    EXPECT_EQ(j->m_counter, 1);
//    TC_Sleep(500);

    // Wait for the KJob to signal that it's finished.
    // Won't get a result() signal here because it's kill()'ed Quietly.
    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 0);

	M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();

    TC_EXIT();
}

TEST_F(AMLMJobTests, ImportProjectJobCancelBeforeStart) // NOLINT
{
    TC_ENTER();

    auto j = new ImportProjectJob(nullptr, nullptr);

//    j->setAutoDelete(false);

	M_QSIGNALSPIES_SET(j);

	connect_or_die(j, &KJob::finished, amlmApp, [&](KJob* kjob){
        qDb() << "GOT SIGNAL FINISHED:" << kjob;
                });

//    EXPECT_TRUE(ef.isStarted()) << state(ef);
//    EXPECT_FALSE(ef.isRunning()) << state(ef);

    // Job hasn't started yet (we never called start()), kill it.
    bool kill_succeeded = j->kill();

    // j is now probably going to be deleteLater()'ed.

    EXPECT_TRUE(kill_succeeded) << j;
//    EXPECT_TRUE(ef.isCanceled()) << ef;
    EXPECT_EQ(j->error(), KJob::KilledJobError);

//    j->start();

//    TC_Sleep(500);
//    EXPECT_EQ(j->m_counter, 0);
//    TC_Sleep(700);
//    EXPECT_EQ(j->m_counter, 1);
//    TC_Sleep(500);

    // Wait for the KJob to signal that it's finished.
    // Won't get a result() signal here because it's kill()'ed Quietly.
    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 0);

	M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();

    TC_EXIT();
}

#include "AMLMJobTests.moc"
