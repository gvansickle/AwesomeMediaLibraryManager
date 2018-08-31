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
#include <QSignalSpy>
#include <QObject>

// Ours
#include <future/future_type_traits.hpp>
#include <future/function_traits.hpp>
#include <tests/TestHelpers.h>
#include "../AMLMJob.h"
#include <src/concurrency/DirectoryScanJob.h>


////////////////////////

#include "AMLMApp.h"

class ProjectFolderItem;
class IProjectFileManager;

class ImportProjectJob: public KJob
{
    Q_OBJECT
public:
    ImportProjectJob(ProjectFolderItem *folder, IProjectFileManager *importer);
    ~ImportProjectJob() override;

public:
    void start() override;
    bool doKill() override;

public: // Added for test development.
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
    QFutureWatcher<void> *m_watcher;
//    QPointer<IProject> m_project;
    bool cancel = false;

    void import(ProjectFolderItem* /*folder*/)
    {
        int loop_counter = 0;
        int max_loops = 10;
        while(!cancel)
        {
            TC_Sleep(100);
            GTEST_COUT_qDB << "LOOP:" << loop_counter;
            loop_counter++;
            if(loop_counter > max_loops)
            {
                GTEST_COUT_qDB << "EXITING LOOP";
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
    GTEST_COUT_qDB << "Running import job, returned future state:" << state(f);
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

    TestAMLMJob1* asDerivedTypePtr() override { return this; }

    std::atomic_int m_counter {0};

protected:

    void runFunctor() override
    {
        SCOPED_TRACE("InFunctor");

        // Do some work...
        for(int i = 0; i<10; i++)
        {
            GTEST_COUT_qDB << "Sleeping for 1 second\n";
            TC_Sleep(1000);

            GTEST_COUT_qDB << "Incementing counter\n";
            m_counter++;

            GTEST_COUT_qDB << "Reporting counter result\n";
            m_ext_future.reportResult(m_counter);

            if(functorHandlePauseResumeAndCancel())
            {
                // We've been cancelled.
                qIno() << "CANCELLED";
                m_ext_future.reportCanceled();
                m_ext_future.reportFinished();
                break;
            }
        }

        setSuccessFlag(!wasCancelRequested());

        m_ext_future.reportFinished();

        GTEST_COUT_qDB << "Returning, m_ext_future:" << m_ext_future;
    }
};

///
/// Test Cases
///

INSTANTIATE_TEST_CASE_P(TrueFalseParameterizedTests,
                        AMLMJobTestsParameterized,
                        ::testing::Bool());

TEST_F(AMLMJobTests, ThisShouldPass)
{
    TC_ENTER();

    EXPECT_TRUE(true);

    TC_EXIT();
}

TEST_F(AMLMJobTests, IPJSynchronousExec)
{
    TC_ENTER();

    auto j = new ImportProjectJob(nullptr, nullptr);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    EXPECT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    EXPECT_TRUE(kjob_result_spy.isValid());
    QSignalSpy kjob_destroyed_spy(j, &QObject::destroyed);

    bool status = j->exec();

    // j has probably deleted itself here.

    EXPECT_EQ(status, true);

    GTEST_COUT_qDB << "FINISHED CT:" << kjob_finished_spy.count();
    GTEST_COUT_qDB << "RESULT CT:" << kjob_result_spy.count();

    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 1);

    EXPECT_TRUE(kjob_destroyed_spy.wait());

    TC_EXIT();
}

TEST_P(AMLMJobTestsParameterized, SynchronousExecTestAMLMJob1PAutoDelete)
{
    TC_ENTER();

    //    QFETCH(bool, autodelete);
    bool autodelete = GetParam();

    GTEST_COUT_qDB << "Autodelete?:" << autodelete;

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(qApp);
    j->setAutoDelete(autodelete);

    M_QSIGNALSPIES_SET(j);

    bool status = j->exec();

    EXPECT_EQ(status, true);

    GTEST_COUT_qDB << "FINISHED CT:" << kjob_finished_spy.count();
    GTEST_COUT_qDB << "RESULT CT:" << kjob_result_spy.count();

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

TEST_P(AMLMJobTestsParameterized, AsynchronousExecTestAMLMJob1PAutoDelete)
{
    TC_ENTER();

    //    QFETCH(bool, autodelete);
    bool autodelete = GetParam();

    GTEST_COUT_qDB << "Autodelete?:" << autodelete;

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(amlmApp);
    j->setAutoDelete(autodelete);

    M_QSIGNALSPIES_SET(j);

    // Start the job asynchronously.
    j->start();

    // Wait for successful KJob completion.
    EXPECT_TRUE(kjob_finished_spy.wait());
//    EXPECT_TRUE(kjob_result_spy.wait());

//    EXPECT_EQ(status, true);

    GTEST_COUT_qDB << "FINISHED CT:" << kjob_finished_spy.count();
    GTEST_COUT_qDB << "RESULT CT:" << kjob_result_spy.count();

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

TEST_F(AMLMJobTests, DISABLED_ThenTest)
{
    TC_ENTER();

    QObject* ctx = new QObject(qApp);
    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(qApp);
    j->setAutoDelete(false);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    EXPECT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    EXPECT_TRUE(kjob_result_spy.isValid());

    j->then(ctx, [=](TestAMLMJob1* j_kjob) -> void {
        if(j->error())
        {
            // Error.
            GTEST_COUT << "ASYNC JOB FAILED:\r\n"; // << j_kjob->error() << ":" << j_kjob->errorText() << ":" << j_kjob->errorString();
        }
        else
        {
            // Succeeded, update the model.
            GTEST_COUT << "ASYNC JOB COMPLETE:\r\n";// << j_kjob;
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

    GTEST_COUT << "JOB EXTFUTURE:" << j->get_extfuture().state() << "\n";
    QList<int> extf_int = j->get_extfuture().get();

    EXPECT_EQ(extf_int.size(), 10);

    TC_EXIT();
}


TEST_P(AMLMJobTestsParameterized, DirScanCancelTestPAutodelete)
{
    TC_ENTER();

    std::atomic_bool got_job_destroyed_signal {false};

//    QFETCH(bool, autodelete);
    bool autodelete = GetParam();

	// Dummy dir so the dir scanner job has something to chew on.
    QUrl dir_url = QUrl::fromLocalFile("/");

    DirectoryScannerAMLMJobPtr dsj = DirectoryScannerAMLMJob::make_job(amlmApp, dir_url,
	                                    QStringList({"*.flac", "*.mp3", "*.ogg", "*.wav"}),
	                                    QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    M_QSIGNALSPIES_SET(dsj);

    ASSERT_TRUE(dsj->isAutoDelete());

    // Set autodelete or not.
    GTEST_COUT_qDB << "Setting Autodelete to:" << autodelete;
    dsj->setAutoDelete(autodelete);

    // Connect signals and slots.
    connect_or_die(dsj, &DirectoryScannerAMLMJob::finished, qApp, [=](KJob* kjob){
        GTEST_COUT_qDB << "GOT FINISHED";
        EXPECT_EQ(kjob, dsj);
        EXPECT_EQ(kjob->error(), KJob::KilledJobError);
        ;});
    connect_or_die(dsj, &DirectoryScannerAMLMJob::result, qApp, [=](KJob* kjob){
        GTEST_COUT_qDB << "GOT RESULT";
        EXPECT_EQ(kjob, dsj);
        EXPECT_EQ(kjob->error(), KJob::KilledJobError);
        ;});
    connect_or_die(dsj, &QObject::destroyed, qApp, [&](QObject* obj){
        GTEST_COUT_qDB << "GOT DESTROYED SIGNAL:" << obj;
        got_job_destroyed_signal = true;
    });

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

    GTEST_COUT_qDB << "FINISHED CT:" << kjob_finished_spy.count();
    GTEST_COUT_qDB << "RESULT CT:" << kjob_result_spy.count();

//    EXPECT_EQ(kjob_finished_spy.count(), 1);
    EXPECT_EQ(kjob_result_spy.count(), 1);

    if(!autodelete)
    {
        dsj->deleteLater();
    }

    GTEST_COUT_qDB << "Waiting for destroyed signal";
    //amlmApp->processEvents(QEventLoop::AllEvents, 1000);
    TC_Wait(100);
    // Wait for the (auto-)delete to happen.
//    M_QSIGNALSPIES_EXPECT_IF_DESTROY_TIMEOUT();
    EXPECT_TRUE(got_job_destroyed_signal);
//    bool didnt_time_out = kjob_destroyed_spy.wait();
    GTEST_COUT_qDB << "Done Waiting for destroyed signal";
//    EXPECT_TRUE(didnt_time_out);

    TC_EXIT();
}

TEST_F(AMLMJobTests, CancelTest)
{
    TC_ENTER();

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(nullptr);
    j->setAutoDelete(false);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    EXPECT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    EXPECT_TRUE(kjob_result_spy.isValid());

    connect_or_die(j, &KJob::finished, qApp, [&](KJob* kjob){
        qDb() << "GOT SIGNAL FINISHED:" << kjob;
                });

    ExtFuture<int> ef = j->get_extfuture();

    // Start the job.
    GTEST_COUT << "START\n";
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
    GTEST_COUT << "CANCELING JOB\n";
    bool kill_succeeded = j->kill(KJob::KillVerbosity::Quietly);

    // j is now probably going to be deleteLater()'ed.

    EXPECT_TRUE(kill_succeeded) << ef.state();
    EXPECT_TRUE(ef.isCanceled()) << ef.state();
    EXPECT_EQ(j->error(), KJob::KilledJobError);

    // Wait for the KJob to signal that it's finished.
    // Won't get a result() signal here because it's kill()'ed Quietly.
    EXPECT_TRUE(kjob_finished_spy.wait());
    EXPECT_FALSE(kjob_result_spy.wait(500));

    TC_EXIT();
}

TEST_F(AMLMJobTests, CancelBeforeStart)
{
    TC_ENTER();

    RecordProperty("amlmtestproperty", "Test of the RecordProperty() system");

    TestAMLMJob1Ptr j = TestAMLMJob1::make_job(nullptr);
//    j->setAutoDelete(false);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    EXPECT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    EXPECT_TRUE(kjob_result_spy.isValid());

    connect_or_die(j, &KJob::finished, qApp, [&](KJob* kjob){
        qDb() << "GOT SIGNAL FINISHED:" << kjob;
                });

    ExtFuture<int> ef = j->get_extfuture();

    EXPECT_TRUE(ef.isStarted()) << ef.state();
//    EXPECT_FALSE(ef.isRunning()) << ef.state();

    // Job hasn't started yet (we never called start()), kill it.
    bool kill_succeeded = j->kill();

    // j is now probably going to be deleteLater()'ed.

    EXPECT_TRUE(kill_succeeded) << ef;
    /// @note No notification to the Future since no watcher has been set up yet.
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

    TC_EXIT();
}

TEST_F(AMLMJobTests, IPJCancelBeforeStart)
{
    TC_ENTER();

    auto j = new ImportProjectJob(nullptr, nullptr);

//    j->setAutoDelete(false);

    QSignalSpy kjob_finished_spy(j, &KJob::finished);
    EXPECT_TRUE(kjob_finished_spy.isValid());
    QSignalSpy kjob_result_spy(j, &KJob::result);
    EXPECT_TRUE(kjob_result_spy.isValid());
//    QSignalSpy kjob_destroyed_spy(j, &QObject::destroyed);
    connect_or_die(j, &KJob::finished, qApp, [&](KJob* kjob){
        qDb() << "GOT SIGNAL FINISHED:" << kjob;
                });

    // No watcher to get a future from.
//    /*ExtFuture<int>&*/ const QFuture<void>& ef = j->get_extfuture_ref();

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

    TC_EXIT();
}

#include "AMLMJobTests.moc"
