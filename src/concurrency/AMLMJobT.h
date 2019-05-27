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

#ifndef AWESOMEMEDIALIBRARYMANAGER_AMLMJOBT_H
#define AWESOMEMEDIALIBRARYMANAGER_AMLMJOBT_H

// Std C++

// Qt5

#ifdef QT_NO_EXCEPTIONS
#error "WE NEED A QT COMPILED WITH EXCEPTIONS ENABLED"
#endif

// Qt 5 / KDE backfill.
#include <utils/QtHelpers.h>

// Ours
#include "AMLMJob.h"
#include "ExtFutureProgressInfo.h"
#include "ExtFuture.h"

/**
 * CRTP class template for wrapping ExtAsync jobs returning an ExtFuture<T>.
 * Use like this:
 * @code
 *      class Derived : public AMLMJobT<ExtFuture<int>>
 *      {
 *      };
 * @endcode
 */
template <class ExtFutureT>
class AMLMJobT : public AMLMJob
{
	using BASE_CLASS = AMLMJob;

public:
	using ExtFutureType = ExtFutureT;
    using ExtFutureWatcherT = QFutureWatcher<typename ExtFutureType::value_type>;

	explicit AMLMJobT(QObject* parent = nullptr) : BASE_CLASS(parent)
	{
        qDbo() << "Constructor, m_ext_future:" << m_ext_future.state();
        // Watcher creation is here vs. in start() to mitigate against cancel-before-start races and segfaults.  Seems to work.
	    // We could get a doKill() call at any time after we leave this constructor.
		m_ext_watcher = new ExtFutureWatcherT(this);

		// Create a new 1 sec speed update QTimer.
		m_speed_timer = new QTimer(this);
	}

	/**
	 * Constructor for make_async_AMLMJobT<>().
	 * @param extfuture
	 * @param parent
	 */
	explicit AMLMJobT(ExtFutureType extfuture, QObject* parent = nullptr,
					  const char* jobname = nullptr,
					  KJob::Unit units = KJob::Unit::Files,
					  KJob::Capabilities capabilities = KJob::Capability::Killable | KJob::Capability::Suspendable)
			: BASE_CLASS(parent), m_ext_future(extfuture), m_hacky_way_to_ignore_start(true)
	{
		qDbo() << "Constructor, m_ext_future:" << m_ext_future.state();

		/// @todo This should be coming through the ExtFuture.
		setProgressUnit(units);

		// Set our object name.
		/// @todo Again better through ExtFuture.
		if(jobname != nullptr)
		{
			this->setObjectName(jobname);
		}
		else
		{
			this->setObjectName("UNKNOWN_JOB");
		}

		// Set our capabilities.
		this->setCapabilities(capabilities);
		this->setAutoDelete(true);

		// Watcher creation is here vs. in start() to mitigate against cancel-before-start races and segfaults.  Seems to work.
		// We could get a doKill() call at any time after we leave this constructor.
		m_ext_watcher = new ExtFutureWatcherT(this);

		// Create a new 1 sec speed update QTimer.
		m_speed_timer = new QTimer(this);

		// Hook up watcher->{other} signal/slots, start speed timer.
		pre_start();

		// Connect future to watcher last, to avoid losing signals we connected to in pre_start().
		m_ext_watcher->setFuture(m_ext_future);
	}

	~AMLMJobT() override
	{
		qDb() << "DELETING AMLMJobT";
	}


    /**
     * Return a copy of the future.
     */
    ExtFutureT get_extfuture()
    {
        return m_ext_future;
    }

	Q_SCRIPTABLE void start() override
	{
//M_WARNING("THIS IS NOW SOMEWHAT INCORRECT, ESP. THE run_class_noarg()");

		if(m_hacky_way_to_ignore_start == true)
		{
			/// We don't have a callback to run(), we were created by make_async_AMLMJobT().
			/// @obsolete this as soon as possible.
			return;
		}

		// Hook up signals and such to the ExtFutureWatcher<T>,
		// start the speed calculation timer.
		pre_start();

		// Just let ExtAsync run the run() function, which will in turn run the runFunctor().
		// Note that we do not use the returned ExtFuture<Unit> here; that control and reporting
		// role is handled by the ExtFuture<> m_ext_future and m_ext_watcher.
		// Note that calling the destructor of (by deleting) the returned future is ok:
		// http://doc.qt.io/qt-5/qfuture.html#dtor.QFuture
		// "Note that this neither waits nor cancels the asynchronous computation."

		// Run.
		ExtFutureT future = ExtAsync::run_class_noarg(this, &std::remove_reference_t<decltype(*this)>::run /*&AMLMJobT::run*/);

		m_ext_future = future;

		// All connections have already been made, so set the watched future.
		// "To avoid a race condition, it is important to call this function after doing the connections."
		m_ext_watcher->setFuture(m_ext_future);
	}

protected: //Q_SLOTS:

    /**
     * @todo This is what happens when your framework is stuck in 199-late.
     * This slot shouldn't even exist, it would be much better to have a signal with this signature in the
     * base class.  But you can't have templated Q_SIGNALs.
     */
//    virtual void SLOT_onResultsReadyAt(const ExtFutureT& ef, int begin, int end)
//    {
//        qWro() << "Base class override called, should never happen.  ef/begin/end:" << ef << begin << end;
////        Q_ASSERT_X(0, __func__, "Base class override called, should never happen.");
//    }

    virtual void SLOT_extfuture_started()
    {
        // The watcher has started watching the future.
        qDbo() << "GOT EXTFUTURE STARTED";
    }

    virtual void SLOT_extfuture_finished()
    {
		/// @note KDevelop's ImportProjectJob::importDone() which is connected to wtcher::finished
		///       does the watcher delete and the emit.
        // The ExtFuture<T> and hence the Job is finished.  Delete the watcher and emit the KJob result.
        // The emitResult() call will send out a KJob::finished() signal.
		qDbo() << "GOT EXTFUTURE FINISHED, calling emitResult()";
//        m_ext_watcher->deleteLater();
		this->emitResult();
    }

    virtual void SLOT_extfuture_canceled()
    {
		/// @note KDevelop's ImportProjectJob::importCanceled() (watcher->canceled) does just this deleteLater().
		/// I can't say I actually follow why they're bothering, don't see a destroyed() connection.
    	// The ExtFuture<T> was cancelled (hopefully through the AMLMJobT interface).
		qDbo() << "GOT EXTFUTURE CANCELED, calling deleteLater() on the watcher.";
        m_ext_watcher->deleteLater();
    }

    virtual void SLOT_extfuture_paused()
    {
        qDbo() << "GOT EXTFUTURE PAUSED";
    }

    virtual void SLOT_extfuture_resumed()
    {
        qDbo() << "GOT EXTFUTURE RESUMED";
    }

    virtual void SLOT_extfuture_progressRangeChanged(int min, int max)
    {
        this->setTotalAmountAndSize(this->progressUnit(), max-min);
    }

	/**
	 * This slot is "overloaded" in an attempt to match KJob-style progress reporting to
	 * QFuture's more limited progress interface.  In short, progress_text will come in here as one of at least:
	 * - An encoded representation of KJob's:
	 * -- "description()" signal text
	 * -- "infoMessage()" signal text
	 * -- "warning()" signal text.
	 * -- And probably more (e.g. KJob's multi-unit progress info).
	 */
	virtual void SLOT_extfuture_progressTextChanged(const QString& progress_text)
	{
		ExtFutureProgressInfo pi;

		QPair<ExtFutureProgressInfo::EncodedType, QStringList> pi_info = pi.fromQString(progress_text);

		QStringList strl = pi_info.second;

		switch(pi_info.first)
		{
		case ExtFutureProgressInfo::EncodedType::DESC:
			AMLM_ASSERT_EQ(strl.size(), 5);
			Q_EMIT this->description(this, strl[0], QPair(strl[1], strl[2]), QPair(strl[3], strl[4]));
			break;
		case ExtFutureProgressInfo::EncodedType::INFO:
			AMLM_ASSERT_EQ(strl.size(), 2);
			Q_EMIT this->infoMessage(this, strl[0], strl[1]);
			break;
		case ExtFutureProgressInfo::EncodedType::WARN:
			AMLM_ASSERT_EQ(strl.size(), 2);
			Q_EMIT this->warning(this, strl[0], strl[1]);
			break;
		case ExtFutureProgressInfo::EncodedType::SET_PROGRESS_UNIT:
		{
			AMLM_ASSERT_EQ(strl.size(), 1);
			int prog_unit = strl[0].toInt();
			// This isn't a slot.
			this->setProgressUnit(prog_unit);
		}
			break;
		case ExtFutureProgressInfo::EncodedType::UNKNOWN:
			/// @todo What should we do with this text?
			qWr() << "UNKNOWN progress text" << progress_text;
			break;
		default:
			Q_ASSERT(0);
			break;
		}
	}

    virtual void SLOT_extfuture_progressValueChanged(int progress_value)
    {
        this->setProcessedAmountAndSize(this->progressUnit(), progress_value);
//        this->emitSpeed(progress_value);
    }

	/**
	 * Calculate the job's speed in bytes per sec.
	 */
	virtual void SLOT_UpdateSpeed()
	{
		auto current_processed_size = this->processedSize();
		auto progress_units_processed_in_last_second = current_processed_size - m_speed_last_processed_size;
		// A little low-pass filtering never hurt anyone.
		m_speed = (3*m_speed/4) + (progress_units_processed_in_last_second/4);
		m_speed_last_processed_size = current_processed_size;
		this->emitSpeed(m_speed);
	}

protected:

	/**
	 * FBO make_async_AMLMJobT<>().
	 */
	void pre_start()
	{
		// Hook up signals and such to the ExtFutureWatcher<T>.
		this->HookUpExtFutureSignals(m_ext_watcher);

		// Start the speed calculation timer.
		m_speed_timer->setTimerType(Qt::TimerType::PreciseTimer);
		m_speed_timer->setInterval(1000);
		m_speed_timer->start();
	}

    /// Last-stage wrapper around the runFunctor().
    /// Handles most of the common ExtFuture<T> start/finished/canceled/exception code.
    /// Should not need to be overridded in derived classes.
    virtual void run()
    {
        /// @note We're in an arbitrary thread here probably without an event loop.

		qDb() << "ExtFuture<T> state:" << m_ext_future.state();

        // Check if we were canceled before we were started.
        if(m_ext_future.isCanceled())
        {
            // We were canceled before we were started.
            // Report (STARTED | CANCELED | FINISHED) and just return.
            /// @note Canceling alone won't finish the extfuture, so we finish it manually here.
            m_ext_future.reportCanceled();
            m_ext_future.reportFinished();
            AMLM_ASSERT_EQ(ExtFutureState::state(m_ext_future), (ExtFutureState::Started | ExtFutureState::Canceled | ExtFutureState::Finished));

			/// @todo Do we really need an emitResult() here?
            emitResult();
            return;
        }
        try
        {
            // Start the work by calling the functor.  We should be in the Running state if we're in here.
            /// @todo But we're not Running here.  Not sure why.
    //        AMLM_ASSERT_EQ(ef.isRunning(), true);
//            qDb() << "Pre-functor ExtFutureState:" << ExtFutureState::state(m_ext_future);
            this->runFunctor();
//            qDb() << "Functor complete, ExtFutureState:" << ExtFutureState::state(m_ext_future);
        }
        catch(QException &e)
        {
            /// @note RunFunctionTask has QFutureInterface<T>::reportException(e); here.
            m_ext_future.reportException(e);
        }
        catch(...)
        {
            /// @note RunFunctionTask has QFutureInterface<T>::reportException(e); here.
            m_ext_future.reportException(QUnhandledException());
        }

        /// @note Ok, runFunctor() has either completed successfully, been canceled, or thrown an exception, so what do we do here?
        /// QtCreator::runextensions.h::AsyncJob::run() calls runHelper(), which then does this here:
        /// @code
        /// // invalidates data, which is moved into the call
        /// runAsyncImpl(futureInterface, std::move(std::get<index>(data))...); // GRVS: The runFunctor() above.
        /// if (futureInterface.isPaused())
        ///         futureInterface.waitForResume();
        /// futureInterface.reportFinished();
        /// @endcode
        /// So it seems we should be safe doing the same thing.

        if(m_ext_future.isPaused())
        {
            // ExtAsync::run<> is paused, so wait for it to be resumed.
            // It won't be paused here if the future was canceled.
            Q_ASSERT(m_ext_future.isFinished() || m_ext_future.isCanceled());
			qWr() << "ExtAsync<> is paused, waiting for it to be resumed....";
            m_ext_future.waitForResume();
        }

		qDb() << "AMLMJOBT REPORTING FINISHED";
        m_ext_future.reportFinished();

        // We should only have two possible states here, excl. exceptions for the moment:
        // - Started | Finished
        // - Started | Canceled | Finished if job was canceled.
        AMLM_ASSERT_EQ(m_ext_future.isStarted(), true);
        AMLM_ASSERT_EQ(m_ext_future.isFinished(), true);

        // Do the post-run work.

        // Set the three KJob error fields.
        setKJobErrorInfo(!m_ext_future.isCanceled());

//        qDbo() << "Calling emitResult():" << "isAutoDelete?:" << isAutoDelete();
//        if(isAutoDelete())
//        {
//            // emitResult() may result in a this->deleteLater(), via finishJob().
//            qWro() << "emitResult() may have resulted in a this->deleteLater(), via finishJob().";
//        }

        /// @note We rely on the ExtFutureWatcher::finished() signal -> this->SLOT_extfuture_finished() to call emitResult().
    }

    /// @name Virtual overrides to forward these calls to their templated counterparts.
    /// @{

    /**
    * doKill(): Kill the KJob.
    * Abort this job quietly.
    * Simply kill the job, no error reporting or job deletion should be involved.
    *
    * @note This is not a slot.  You're looking for KJob::kill(), which is a public slot.
    *
    * @note KJob::doKill() does nothing, simply returns false.
    *
    * What our override here does:
    * Tells the runFunctor() to kill itself by calling .cancel() on its ExtFuture<>, then
    * waits for it to finish via waitForFinished().
    *
    * @note It does look like this should block until the job is really confirmed to be killed.
    *       KAbstractWidgetJobTracker::slotStop() does this:
    * @code
    *         job->kill(KJob::EmitResult); // notify that the job has been killed
    *         emit stopped(job);
    * @endcode
    *
    * @warning this may/will be deleteLater()'ed at any time after this function returns true.
    *
    * @return true if job successfully killed, false otherwise.
    */
    bool doKill() override { return this->doKillT(); }

    bool doSuspend() override { return this->doSuspendT(); }
    bool doResume() override { return this->doResumeT(); }
    /// @}

    bool doKillT()
    {
        // KJob::doKill().

        qDbo() << "START AMLMJobT::DOKILL";

        /**
        /// @note The calling thread has to have an event loop, and actually AFAICT should be the main app thread.
        /// @note Kdevelop::ImportProjectJob does this:
        /// @code
            bool ImportProjectJob::doKill()
            {
            d->m_watcher->cancel();
            d->cancel=true;

            setError(1);
            setErrorText(i18n("Project import canceled."));

            d->m_watcher->waitForFinished();
            return true;
            }
        /// @endcode
        */
        AMLM_ASSERT_IN_GUITHREAD();

        if(!(capabilities() & KJob::Killable))
        {
            Q_ASSERT_X(0, __func__, "Trying to kill an unkillable AMLMJob.");
        }

		// Stop the speed timer if it's running.
		m_speed_timer->stop();

        if(this->isAutoDelete())
	    {
		    /// @warning At any point after we return here, this may have been deleteLater()'ed by KJob::finishJob().
		    qWr() << "doKill() about to cancel AMLMJobT with autoDelete()==true, may result in a this->deleteLater(), via finishJob().";
	    }

        // Tell the Future and hence job to Cancel.
		/// @todo Valgrind says that when we get an aboutToShutdown(), this is an 'invalid read of size 8'.
//		m_ext_watcher->cancel();
		m_ext_future.cancel();


        /// Kdevelop::ImportProjectJob::doKill() sets the KJob error info here on a kill.
        /// @todo Is it possible for us to have been deleted before this call due to the cancel() above?
		this->setError(KJob::KilledJobError);
        /// @todo This text should probably be set somehow by the underlying async job.
		this->setErrorText(tr("Job killed"));

#if 0
        //    Q_ASSERT(ef.isStarted() && ef.isCanceled() && ef.isFinished());
#else

		// Wait for the runFunctor() to report Finished.
//		m_ext_watcher->waitForFinished();
		m_ext_future.waitForFinished();

        // We should never get here before the undelying ExtAsync job is indicating canceled and finished.
        /// @note Seeing the assert below, sometimes not finished, sometimes is?  Started | Canceled always.
        ///       Kdevelop::ImportProjectJob does this through a QFutureWatcher set up in start().
//        AMLM_ASSERT_EQ(m_ext_future.state(), ExtFutureState::Started | ExtFutureState::Canceled | ExtFutureState::Finished);
        /// @todo Difference here between cancel before and after start.
        /// Before: Started | Canceled, After: S|F|C.
//        qDbo() << "POST-CANCEL FUTURE STATE:" << ExtFutureState::state(m_ext_future);
#endif

        return true;
    }

    bool doSuspendT()
    {
        /// KJob::doSuspend().
        Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to suspend an unsuspendable AMLMJob.");
        m_ext_future.setPaused(true);
        return true;
    }

    bool doResumeT()
    {
        /// KJob::doResume().
        Q_ASSERT_X(capabilities() & KJob::Capability::Suspendable, __func__, "Trying to resume an unresumable AMLMJob.");
        m_ext_future.setPaused(false);
        return true;
    }

	void HookUpExtFutureSignals(ExtFutureWatcherT* watcher)
	{
		using ThisType = std::remove_reference_t<decltype(*this)>;
//		auto watcher = watcher_sp.data();

		// If AMLMJobT is destroyed, ... do we need to do anything?
//		connect_or_die(this, &QObject::destroyed, );

        // FutureWatcher signals to this->SLOT* connections.
        // Regarding canceled QFuture<>s: "Any QFutureWatcher object that is watching this future will not deliver progress
        // and result ready signals on a canceled future."
        // This group corresponds ~1:1 with KJob functionality.
		connect_or_die(watcher, &ExtFutureWatcherT::started, this, &ThisType::SLOT_extfuture_started);
		connect_or_die(watcher, &ExtFutureWatcherT::canceled, this, &ThisType::SLOT_extfuture_canceled);
		connect_or_die(watcher, &ExtFutureWatcherT::paused, this, &ThisType::SLOT_extfuture_paused);
		connect_or_die(watcher, &ExtFutureWatcherT::resumed, this, &ThisType::SLOT_extfuture_resumed);
		connect_or_die(watcher, &ExtFutureWatcherT::finished, this, &ThisType::SLOT_extfuture_finished);


		// FutureWatcher progress signals -> this slots.
		connect_or_die(watcher, &ExtFutureWatcherT::progressRangeChanged, this, &ThisType::SLOT_extfuture_progressRangeChanged);
		connect_or_die(watcher, &ExtFutureWatcherT::progressTextChanged, this, &ThisType::SLOT_extfuture_progressTextChanged);
		connect_or_die(watcher, &ExtFutureWatcherT::progressValueChanged, this, &ThisType::SLOT_extfuture_progressValueChanged);

        // Signal-to-signal connections.
        // forward resultsReadyAt() signal.
		connect_or_die(watcher, &ExtFutureWatcherT::resultsReadyAt, this, &ThisType::SIGNAL_resultsReadyAt);
        // KJob signal forwarders.
        /// @todo Don't need/want these as long as we don't override the base class versions of the signals.
//        connect_or_die(this, &KJob::finished, this, &ThisType::finished);
//        connect_or_die(this, &KJob::result, this, &ThisType::result);

		// Speed update timer.
		connect_or_die(m_speed_timer, &QTimer::timeout, this, &ThisType::SLOT_UpdateSpeed);

		/// @todo EXP: Throttling.
//		m_ext_watcher.setPendingResultsLimit(2);

	}

    /// @name KJob-related support functions.
    /// @{

    /**
     * Sets the KJob error code / string.
     *
     * @param success  true if the underlying job completed successfully and wasn't cancelled.  false otherwise.
     */
    void setKJobErrorInfo(bool success)
    {
        // We're still in the underlying ExtAsync::run() context and don't have an event loop here.
        /// @note GRVS: Threadsafety not clear.  KJob doesn't do any locking FWICT around these variable sets.
    //    AMLM_ASSERT_IN_GUITHREAD();

        // Convert underlying finished to a KJob::result(KJob*) signal, but only in the success case.
        // We have to be careful we don't call KF5::emitResult() twice.
        // We'll similarly deal with the fail case in onTWFailed().
        if(success)
        {
            // Set the KJob::error() code.
            setError(NoError);
        }
        else
        {
            // Set the KJob error info.
            if(m_ext_future.isCanceled())
            {
                // Cancelled.
                // KJob
                setError(KilledJobError);
            }
            else
            {
                // Some other error.
                // KJob
                setError(KJob::UserDefinedError);
                setErrorText(QString("Unknown, non-Killed-Job error on %1: %2")
                             .arg(this->metaObject()->className())
                             .arg(this->objectName()));
            }
        }
    }


	/// @} /// END KJob-related support functions.

	bool m_hacky_way_to_ignore_start {false};

    /// The ExtFuture<T>.
    /// This is always a copy of an ExtFuture<T> created somewhere outside this class instance.
	ExtFutureT m_ext_future { ExtAsync::make_started_only_future<typename ExtFutureT::inner_t>() };

	/// The watcher for the ExtFuture.
	/// @note Would like to use a std::unique_ptr here, but it screws with the QObject parent/child delete mechanism
	///       (we get double deletes).
	ExtFutureWatcherT* m_ext_watcher { nullptr };

	/// KJob::emitSpeed() support, which we apparently have to maintain ourselves.
	/// KJob::emitSpeed() takes an unsigned long.
	/// QTimer inherits QObject.
	QTimer* m_speed_timer { nullptr };
	unsigned long m_speed {0};
	qulonglong m_speed_last_processed_size {0};
	std::deque<int64_t> m_speed_history;
};

/**
 * Create a new AMLMJobT wrapped around an ExtFuture<T>.
 * Does not autodelete by default.
 * @todo Does this really need a parent?  AMLMJob[T] takes one, but....
 */
template<class ExtFutureT>
AMLMJobT<ExtFutureT>*
make_async_AMLMJobT(ExtFutureT ef, const char* jobname = nullptr, QObject* parent = nullptr)
{
	/// @todo I think we don't care if the future has already started/canceled here,
	/// as long as we hook up fut<->job and job<->everything-else signal/slots, we're ok.
//	Q_ASSERT(!ef.isFinished() && !ef.isCanceled());
	return new AMLMJobT<ExtFutureT>(ef, parent, jobname, KJob::Unit::Files);
}


#endif //AWESOMEMEDIALIBRARYMANAGER_AMLMJOBT_H
