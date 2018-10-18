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

#ifndef SRC_CONCURRENCY_AMLMJOB_H_
#define SRC_CONCURRENCY_AMLMJOB_H_

#include <config.h>

// Std C++
#include <deque>

// Qt5
#include <QObject>
#include <QPointer>
#include <QWeakPointer>
#include <QSharedPointer>
#include <QTime>
#include <QMutex>
#include <QWaitCondition>
#include <QSemaphore>

// KF5
#include <KJob>
#include <KJobUiDelegate>

// Ours
#include <src/future/function_traits.hpp>
#include <src/future/static_if.hpp>
#include <src/utils/UniqueIDMixin.h>
#include "utils/ConnectHelpers.h"
#include "concurrency/ExtFutureProgressInfo.h"
#include "concurrency/ExtAsync.h"


class AMLMJob;
/// Use the AMLMJobPtr alias to pass around refs to AMLMJob-derived jobs.
using AMLMJobPtr = QPointer<AMLMJob>;

Q_DECLARE_METATYPE(AMLMJobPtr);


template <class T>
struct AMLMJob_traits
{
    using type = T;
    using ExtFutureType = typename T::ExtFutureT;

    ExtFutureType get_future() { return T::m_ext_future; }

};

/**
 * Unbelieveable PITA.
 * https://stackoverflow.com/questions/39186348/connection-of-pure-virtual-signal-of-interface-class?rq=1
 * https://stackoverflow.com/questions/17943496/declare-abstract-signal-in-interface-class?noredirect=1&lq=1
 */
class IExtFutureWatcher
{
public:
    virtual ~IExtFutureWatcher() = default;

//Q_SIGNALS:
    virtual void SIGNAL_resultsReadyAt(int begin, int end) = 0;

    // KJob signals.
    virtual void finished(KJob *job) = 0;
    virtual void result(KJob *job) = 0;

    // QObject signals.
//    virtual void destroyed(QObject* obj) = 0;
};
Q_DECLARE_INTERFACE(IExtFutureWatcher, "IExtFutureWatcher")

/**
* Where Does The State Live?
*
* KJobPrivate itself contains what should be what's needed and canonical:
*
* class KCOREADDONS_EXPORT KJobPrivate
* {
* public:
* [...]
*   QString errorText;
   int error;
   KJob::Unit progressUnit;
   QMap<KJob::Unit, qulonglong> processedAmount;
   QMap<KJob::Unit, qulonglong> totalAmount;
   unsigned long percentage;
*
* Most/all of this data can be accessed from protected or public KJob members.  E.g.:
* class KJob
* protected:
*     KJob::setProcessedAmount(Unit unit, qulonglong amount)
*      Sets the processed size. The processedAmount() and percent() signals
*      are emitted if the values changed. The percent() signal is emitted
*      only for the progress unit.
*     void setProcessedAmount(Unit unit, qulonglong amount);
*
* The code looks like this:
*
* @code
* protected: void KJob::setProcessedAmount(Unit unit, qulonglong amount)
    {
        Q_D(KJob);
        bool should_emit = (d->processedAmount[unit] != amount);

        d->processedAmount[unit] = amount;

        if (should_emit) {
            emit processedAmount(this, unit, amount);
            if (unit == d->progressUnit) {
                emit processedSize(this, amount);
                emitPercent(d->processedAmount[unit], d->totalAmount[unit]);
            }
        }
    }

    Q_SIGNAL: // Private, don't emit directly, call setProcessedAmount().
    void processedAmount(KJob *job, KJob::Unit unit, qulonglong amount);

    // Public read accessor.
     * Returns the processed amount of a given unit for this job.
     *
     * @param unit the unit of the requested amount
     * @return the processed size
    //
    Q_SCRIPTABLE qulonglong processedAmount(Unit unit) const;

* @endcode
*
* So it would appear that there's no need to maintain any such state in the widget.
* Except...
*
/// Amount vs. Size
/// KJob looks somewhat broken when it comes to "Size".  It supports "Amount" units of Kjob::Bytes,
/// KJob::Files, and KJob::Directories, and while there is a "KJob::Unit progressUnit;" member, it's
/// hidden behind the pImpl and there's no way to see or change it as far as I can tell.  It's defaulted
/// to "KJob::Bytes", and is the unit used as the "Size" for both read and write.
/// All KJob Amount and Size updates come from (protected) calls to setProcessedAmount() and
/// setSizeAmount().  They set the amount of the given unit in the qmap, and emit processed/totalAmount()
/// signals.  Additionally, if the unit matches "progressUnit" (==Bytes), processed/totalSize() signals
/// are emitted, the percent complete mechanism is updated, and percent() is emitted.
/// So bottom line, it looks like if you want to use percent at all, you have to update the "KJob::Units::Bytes"
/// *Amount()s.  I guess that's ok, you can always call it whatever you want in the UI.
///
*/

/**
 * Base class for jobs which bridges the gap between an ExtAsync job and a KJob-derived class.
 *
 * @note Multiple inheritance in effect here.  Ok since only KJob inherits from QObject.
 *
 */
class AMLMJob: public KJob, public IExtFutureWatcher, public UniqueIDMixin<AMLMJob>
{

    Q_OBJECT
	Q_INTERFACES(IExtFutureWatcher)

    /// KCoreAddons::KJob
    /// - Subclasses must implement start(), which should trigger the execution of the job (although the work should be done asynchronously).
    /// - errorString() should also be reimplemented by any subclasses that introduce new error codes.
    /// - KJob and its subclasses are meant to be used in a fire-and-forget way. Jobs will delete themselves when they finish using
    ///   deleteLater() (although this behaviour can be changed), so a job instance will disappear after the next event loop run.

    /**
     * @note CRTP: Still need this to avoid ambiguous name resolution.
     * @see https://stackoverflow.com/a/46916924
     */
    using UniqueIDMixin<AMLMJob>::uniqueQObjectName;

	using BASE_CLASS = KJob;

Q_SIGNALS:

    /// @name ExtFuture<T> signals we want to expose to the outside world.
    /// @{

    void SIGNAL_resultsReadyAt(int begin, int end) override;

    /// @}

    /// @warning Qt5 signals are always public in the C++ sense.  Slots are similarly public when called
    ///          via the signal->slot mechanism, on direct calls they have the normal public/protected/private rules.


    /// @name User-public/subclass-private internal KJob signals.
    /// Here for reference only, these are KJob-private, i.e. can't be emitted directly.
    /// @{

    /// "Emitted when the job is finished, in any case. It is used to notify
    /// observers that the job is terminated and that progress can be hidden."
    /// Call emitResult(job) to emit.
    void finished(KJob *job) override;
    /// "Emitted when the job is suspended."
    /// No direct way to emit this?
//    void suspended(KJob *job);
    /// "Emitted when the job is resumed."
    /// No direct way to emit this?
//    void resumed(KJob *job);
    /// "Emitted when the job is finished (except when killed with KJob::Quietly).
    /// Use error to know if the job was finished with error."
    /// Call emitResult(job) to emit.
    void result(KJob *job) override;

    // QObject signals.
//	void destroyed(QObject* obj) override;

    /// @}

    /// @name Public KJob signals, quite a few:
    ///
	// void 	description (KJob *job, const QString &title, const QPair< QString, QString > &field1=QPair< QString, QString >(), const QPair< QString, QString > &field2=QPair< QString, QString >())

    /// "Emitted when the job is finished, in any case.
    /// It is used to notify observers that the job is terminated and that progress can be hidden.
    /// *** This is a private signal, it can't be emitted directly by subclasses of KJob, use emitResult() instead.
    /// In general, to be notified of a job's completion, client code should connect to result() rather than finished(), so that kill(Quietly) is indeed quiet. However if you store a list of jobs
    /// and they might get killed silently, then you must connect to this instead of result(), to avoid dangling pointers in your list."
    // void finished(KJob *job);

    // void 	infoMessage (KJob *job, const QString &plain, const QString &rich=QString())
    // void 	warning (KJob *job, const QString &plain, const QString &rich=QString())


    // void result (KJob *job)
	// void 	resumed (KJob *job)
    /// KJob::Speed
    /// I'm not at all clear on how to really use the KJob::speed functionality.  You call emitSpeed(value),
    /// with whatever "value" is, it starts a 5 second timer, then... the timer times out... then I totally lose the plot.
    /// Ah well.
	// void 	speed (KJob *job, unsigned long speed)
    // void suspended (KJob *job)

	// void 	totalAmount (KJob *job, KJob::Unit unit, qulonglong amount)
	// void 	totalSize (KJob *job, qulonglong size)
    // void 	processedAmount (KJob *job, KJob::Unit unit, qulonglong amount)
    // void 	processedSize (KJob *job, qulonglong size)
    // void 	percent (KJob *job, unsigned long percent)

    /// KJobs are supported by KJobWidgets
	/// https://api.kde.org/frameworks/kjobwidgets/html/namespaceKJobWidgets.html
	///

    /// KJobTrackerInterface (== Watcher for KJob*s):
    /// https://cgit.kde.org/kcoreaddons.git/tree/src/lib/jobs/kjobtrackerinterface.h
    /// On call to registerJob(), The default implementation connects the following KJob signals
    /// to the respective protected slots of this class:
    //    *  - finished() (also connected to the unregisterJob() slot) // Emitted in KJob destructor.
    //    *  - suspended()
    //    *  - resumed()
    //    *  - description()
    //    *  - infoMessage()
    //    *  - totalAmount()
    //    *  - processedAmount() ///< @note KJob's xxxSize() signals aren't reflected in this interface.
    //    *  - percent()
    //    *  - speed()
    //    *
    //    * If you re-implement this method [registerJob()], you may want to call the default
    //    * implementation or add at least:
    //    *
    //    * @code
    //    * connect(job, &KJob::finished, this, &MyJobTracker::unregisterJob);
    //    * @endcode
    //    *
    //    * so that you won't have to manually call unregisterJob().

protected:
    /// Protected KJob-like constructor.
    /// Derive from and defer to this from derived classes, possibly as part of a two-stage constructor:
    /// Once the derived constructor is called and returns, we'll have a valid AMLMJob this and a valid derived this.
    /// We can then call virtual functions in subsequent constructors.
    /// @note Don't try that at home.
    ///
    /// @note KJob's constructor has this same signature, in particular nonconst pointer to parent.
    explicit AMLMJob(QObject* parent = nullptr);

public:
    AMLMJob() = delete;
    /// Destructor.
    ~AMLMJob() override;

    /// @name KJob overrides.
    /// @{

    /**
     * Starts the job asynchronously.
     *
     * When the job is finished, result() is emitted.
     *
     * Warning: Never implement any synchronous workload in this method. This method
     * should just trigger the job startup, not do any work itself. It is expected to
     * be non-blocking.
     *
     * This is the method all subclasses need to implement.
     * It should setup and trigger the workload of the job. It should not do any
     * work itself. This includes all signals and terminating the job, e.g. by
     * emitResult(). The workload, which could be another method of the
     * subclass, is to be triggered using the event loop, e.g. by code like:
     * \code
     * void ExampleJob::start()
     * {
     *  QTimer::singleShot(0, this, SLOT(doWork()));
     * }
     * \endcode
     *
     * @note GRVS: Per comments, KF5 KIO::Jobs autostart; this is overridden to be a no-op.
     */
//    Q_SCRIPTABLE void start() override;

    /// @}

public:
	/// @name New public interfaces FBO derived classes and ExtFuture<T>s wrapped in an AMLMJobT<T>.
	/// Need to be public so they can be accessed from the this pointer passed to run() or the async
	/// function connected to the returned ExtFuture<T>.
	/// @warning Do not use these from client code.
    /// @{

	/// Give derived classes write access to progressUnit.
	/// Sets the Unit which will be used for percent complete and total/processedSize calculations.
	/// Defaults to KJob::Unit::Bytes.
	void setProgressUnit(int prog_unit) /*override*/;
	void setProcessedAmountAndSize(int unit, qulonglong amount) /*override*/;
	void setTotalAmountAndSize(int unit, qulonglong amount) /*override*/;

    /// @}

	/// Returns the currently set progress unit for this AMLMJob.
	KJob::Unit progressUnit() const;

    virtual qulonglong totalSize() const;
    virtual qulonglong processedSize() const;

public:
    /// Dump info about the given KJob.
    static void dump_job_info(KJob* kjob, const QString &header = QString());

public:

    /// @name Callback/pseudo-std-C++17+ interface.
    /// @{

    /**
     * .then(ctx, continuation) -> void
     */
    template <typename ContextType, typename Func,
              REQUIRES(std::is_base_of_v<QObject, ContextType>)>
    void then(const ContextType *ctx, Func&& f)
    {
        qDb() << "ENTERED THEN";

//        QPointer<KJob> pkjob = kjob;

        // result(KJob*) signal:
        // "Emitted when the job is finished (except when killed with KJob::Quietly)."
		connect_or_die(this, &KJob::result, ctx, [=](KJob* kjob){

//            qDbo() << "IN THEN CALLBACK, KJob:" << kjob;

            // Need to determine if the result was success, error, or cancel.
            // In the latter two cases, we need to make sure any chained AMLMJobs are either
            // cancelled (or notified of the failure?).
            switch(kjob->error())
            {
            // "[kjob->error()] Returns the error code, if there has been an error.
            // Only call this method from the slot connected to result()."
            case NoError:
                break;
            case KilledJobError:
                break;
            default:
                // UserDefinedError or some other error.
                break;
            }

            if(kjob->error())
            {
                // Report the error.
                qWr() << "Reporting error via uiDelegate():" << kjob->error() << kjob->errorString() << ":" << kjob->errorText();
                kjob->uiDelegate()->showErrorMessage();
            }
            else
            {
                // Cast to the derived job type.
                using JobType = std::remove_pointer_t<argtype_t<Func, 0>>;
                auto* jobptr = dynamic_cast<JobType*>(kjob);
                Q_ASSERT(jobptr);
                // Call the continuation.
                f(jobptr);
            }
        });
    }

    /// @}



public Q_SLOTS:

    /// @name KJob job control slots
    /// @note Default KJob implementations appear to be sufficient.  They call
    ///       the protected "doXxxx()" functions (which we do need to override below),
    ///       and then emit the proper signals to indicate the deed is done.
    /// @link https://api.kde.org/frameworks/kcoreaddons/html/kjob_8cpp_source.html#l00117
    /// @{

    /**
     * KJob::kill() does this:
     * https://cgit.kde.org/kcoreaddons.git/tree/src/lib/jobs/kjob.cpp
     * @code
     * if (doKill()) {
     *      setError(KilledJobError);
     *      finishJob(verbosity != Quietly);
     *      return true;
     *  } else {
     *      return false;
     *  }
     * @endcode
     * void finishJob(bool emitResult); is a private non-virt member, also called by emitResult() with param==true,
     * https://cgit.kde.org/kcoreaddons.git/tree/src/lib/jobs/kjob.cpp#n96
     * which does this:
     * @code
     * Q_D(KJob);
        d->isFinished = true;

        if (d->eventLoop) {
            d->eventLoop->quit();
        }

        // If we are displaying a progress dialog, remove it first.
        emit finished(this, QPrivateSignal());

        if (emitResult) {
            emit result(this, QPrivateSignal());
        }

        if (isAutoDelete()) {
            deleteLater();
        }
     * @endcode
     *
     */
//    bool kill(KJob::KillVerbosity verbosity=KJob::Quietly);

//    bool resume();
//    bool suspend();

    /// @}

protected:

    /// @name ExtAsync job support functions / function templates.
    /// @{

    /**
     * The function which is run by ExtAsync::run() to do the work.
     * Must be overridden in derived classes.
     * Reporting and control should be handled via the derived class's m_ext_future member.
     *
     */
    virtual void runFunctor() = 0;

    /// @}

    /// @name Override of KJob protected functions.
    /// @{

    /**
     * Not a slot.
     * @note KJob::doSuspend() simply returns false.
     */
//    bool doSuspend() override;

    /**
     * Not a slot.
     * @note KJob::doResume() simply returns false.
     */
//    bool doResume() override;

    /**
     * KJob::emitResult()
     * Emit the result signal, and if job is autodelete, suicide this job.
     * @note Deletes this job using deleteLater(). It first notifies the observers to hide the
     *       progress for this job using the finished() signal.
     *       KJob implementation calls finshJob(), which:
     *       - Sets isFinished = true
     *       - quit()s the internal event loop
     *       - emits finished(this);
     *       - if(emitResult) emit result(this) //< emitResult == true in this case.
     *       - if(isAutoDelete) deleteLater();
     *       This is probably sufficient behavior and we don't need to overload this (non-virtual) function.
     */
    /// void emitResult();

    /**
     * KJob::emitPercent()
     */
//    void KJob::emitPercent(qulonglong processedAmount, qulonglong totalAmount);

    /// Defaults of these seem to be suitable.
//    void setError(int errorCode);
//    void setErrorText(const QString &errorText);
//    void setProcessedAmount(Unit unit, qulonglong amount);
//    void setTotalAmount(Unit unit, qulonglong amount);
//    void setPercent(unsigned long percentage);
    /// speed is in bytes/sec
//    void emitSpeed(unsigned long speed);

    /// @} /// END Override of KJob protected functions.

protected Q_SLOTS:

    /// @name Internal slots
    /// @{

//    void SLOT_extfuture_finished();
//    void SLOT_extfuture_canceled();

    void SLOT_kjob_finished(KJob* kjob);
    void SLOT_kjob_result(KJob* kjob);

    // Really would like to be able to do this, but Qt5 doesn't.
//    template <class T>
//    void SLOT_onResultsReadyAt(T ef, int begin, int end);
    /// @}

private:
    Q_DISABLE_COPY(AMLMJob)

private Q_SLOTS:

    /**
     * Connected to the App's aboutToShutdown() signal in the constructor.
     * Simply calls kill().
     * Similar to mechanism in KDevelop's ::ICore & ::ImportProjectJob.
     */
    void SLOT_onAboutToShutdown();

private:

    /// Wishful thinking at the moment, but maybe I'll figure out how to separate "Size" from KJob::Bytes.
    KJob::Unit m_progress_unit { KJob::Unit::Bytes };
};

/**
 * Class template for wrapping ExtAsync jobs returning an ExtFuture<T>.
 */
template <class ExtFutureT>
class AMLMJobT : public AMLMJob
{
	using BASE_CLASS = AMLMJob;

public:
	using ExtFutureType = ExtFutureT;
    using ExtFutureWatcherT = QFutureWatcher<typename ExtFutureT::value_type>;

    explicit AMLMJobT(QObject* parent = nullptr)
        : BASE_CLASS(parent)
	{
        qDbo() << "Constructor, m_ext_future:" << m_ext_future.state();
        // Watcher creation is here vs. in start() to mitigate against cancel-before-start races and segfaults.  Seems to work.
	    // We could get a doKill() call at any time after we leave this constructor.
        m_ext_watcher = new ExtFutureWatcherT();
		// Create a new 1 sec speed update QTimer.
		m_speed_timer = QSharedPointer<QTimer>::create(this);
	}

    /**
     * Return a copy of the future.
     */
    ExtFutureT get_extfuture()
    {
        return m_ext_future;
    }

    /**
     * Factory function for creating AMLMJobT's wrapping the passed-in ExtFuture<T>.
     * @returns AMLMJobT, which is not started.
     */
    static std::unique_ptr<AMLMJobT> make_amlmjobt(ExtFutureT ef, QObject* parent = nullptr)
    {
        auto job = std::make_unique<AMLMJobT>(ef, parent);
        return job;
    }

    Q_SCRIPTABLE void start() override
    {
        // Hook up signals and such to the ExtFutureWatcher<T>.
        HookUpExtFutureSignals(m_ext_watcher);

		// Start the speed calculation timer.
		m_speed_timer->setTimerType(Qt::TimerType::PreciseTimer);
		m_speed_timer->setInterval(1000);
		m_speed_timer->start();

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
        // The ExtFuture<T> and hence the Job is finished.  Delete the watcher and emit the KJob result.
        // The emitResult() call will send out a KJob::finished() signal.
		qDbo() << "GOT EXTFUTURE FINISHED, calling deleteLater() on the watcher.";
        m_ext_watcher->deleteLater();
        emitResult();
    }

    virtual void SLOT_extfuture_canceled()
    {
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
		m_speed = progress_units_processed_in_last_second;
		m_speed_last_processed_size = current_processed_size;
		this->emitSpeed(m_speed);
	}

protected:

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
    #ifdef QT_NO_EXCEPTIONS
    #error "WE NEED EXCEPTIONS"
    #else
        try
        {
    #endif
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
            // ExtAsync<> is paused, so wait for it to be resumed.
			qWr() << "ExtAsync<> is paused, waiting for it to be resumed....";
            m_ext_future.waitForResume();
        }

		qDb() << "REPORTING FINISHED";
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

        if(!(capabilities() & KJob::Capability::Killable))
        {
            Q_ASSERT_X(0, __func__, "Trying to kill an unkillable AMLMJob.");
        }

		// Stop the speed timer if it's running.
		m_speed_timer->stop();

        // Tell the Future and hence job to Cancel.
M_WARNING("Valgrind says that when we get an aboutToShutdown(), this is an 'invalid read of size 8'");
//        m_ext_watcher->cancel();
		m_ext_future.cancel();

        /// Kdevelop::ImportProjectJob::doKill() sets the KJob error info here on a kill.
        /// @todo Is it possible for us to have been deleted before this call due to the cancel() above?
		this->setError(KJob::KilledJobError);
        /// @todo This text should probably be set somehow by the underlying async job.
		this->setErrorText(tr("Job killed"));

#if 0
        //    Q_ASSERT(ef.isStarted() && ef.isCanceled() && ef.isFinished());
#else
		if(this->isAutoDelete())
		{
			/// @warning At any point after we return here, this may have been deleteLater()'ed by KJob::finishJob().
			qWr() << "doKill() returning, AMLMJob is autoDelete(), may result in a this->deleteLater(), via finishJob().";
		}

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

    template <class WatcherType>
    void HookUpExtFutureSignals(WatcherType* watcher)
	{
		using ThisType = std::remove_reference_t<decltype(*this)>;

        // FutureWatcher signals to this->SLOT* connections.
        // Regarding canceled QFuture<>s: "Any QFutureWatcher object that is watching this future will not deliver progress
        // and result ready signals on a canceled future."
        // This group corresponds ~1:1 with KJob functionality.
		connect_or_die(watcher, &ExtFutureWatcherT::started, this, &ThisType::SLOT_extfuture_started);
		connect_or_die(watcher, &ExtFutureWatcherT::finished, this, &ThisType::SLOT_extfuture_finished);
		connect_or_die(watcher, &ExtFutureWatcherT::canceled, this, &ThisType::SLOT_extfuture_canceled);
		connect_or_die(watcher, &ExtFutureWatcherT::paused, this, &ThisType::SLOT_extfuture_paused);
		connect_or_die(watcher, &ExtFutureWatcherT::resumed, this, &ThisType::SLOT_extfuture_resumed);

		// FutureWatcher progress signals -> this slots.
		connect_or_die(watcher, &ExtFutureWatcherT::progressRangeChanged, this, &ThisType::SLOT_extfuture_progressRangeChanged);
		connect_or_die(watcher, &ExtFutureWatcherT::progressTextChanged, this, &ThisType::SLOT_extfuture_progressTextChanged);
		connect_or_die(watcher, &ExtFutureWatcherT::progressValueChanged, this, &ThisType::SLOT_extfuture_progressValueChanged);

        // Signal-to-signal connections.
        // forward resultsReadyAt() signal.
        connect_or_die(watcher, &WatcherType::resultsReadyAt, this, &ThisType::SIGNAL_resultsReadyAt);
        // KJob signal forwarders.
        connect_or_die(this, &KJob::finished, this, &ThisType::finished);
        connect_or_die(this, &KJob::result, this, &ThisType::result);

        // QObject forwarders.
		/// @note Does this actually make sense?
//		connect_queued_or_die(this, &QObject::destroyed, this, &ThisType::destroyed);

		// Speed update timer.
		connect_or_die(m_speed_timer.data(), &QTimer::timeout, this, &ThisType::SLOT_UpdateSpeed);

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

    /// The ExtFuture<T>.
    ExtFutureT m_ext_future;

	/// The watcher for the ExtFuture.
    ExtFutureWatcherT* m_ext_watcher;

	/// KJob::emitSpeed() support, which we apparently have to maintain ourselves.
	int64_t m_speed {0};
	QSharedPointer<QTimer> m_speed_timer { nullptr };
	qulonglong m_speed_last_processed_size {0};
	std::deque<int64_t> m_speed_history;

};

/**
 * Create a new AMLMJobT from an ExtFuture<>.
 */
template<class ExtFutureT>
inline static auto* make_amlmjobt(ExtFutureT ef, QObject* parent = nullptr)
{
	auto job = new AMLMJobT<ExtFutureT>(ef, parent);
	return job;
}

#endif /* SRC_CONCURRENCY_AMLMJOB_H_ */
