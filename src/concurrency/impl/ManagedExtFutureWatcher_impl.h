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

/**
 * @file ManagedExtFutureWatcher_impl.h
 */
#ifndef SRC_CONCURRENCY_IMPL_MANAGEDEXTFUTUREWATCHER_IMPL_H_
#define SRC_CONCURRENCY_IMPL_MANAGEDEXTFUTUREWATCHER_IMPL_H_

// Std C++
#include <exception>

// Qt5
#include <QFutureWatcher>
#include <QThread>

// Ours
#include <utils/DebugHelpers.h>
#include <logic/PerfectDeleter.h>
#include "../ExtFuture_traits.h"

template <class T>
class ExtFuture;

namespace ManagedExtFutureWatcher_detail
{

	/**
	 * Mostly just a stub QObject-derived class at the moment for use as a parent in the future watcher thread.
	 */
	class FutureWatcherParent : public QObject
	{
		Q_OBJECT

	public:
		explicit FutureWatcherParent(QObject* parent = nullptr) : QObject(parent) {};

	public Q_SLOTS:
		void doWork(int param)
		{
			/// @todo Do something in the other thread...

			// Emit the result as a signal.
	//			Q_EMIT resultReady(fw);
		};

	Q_SIGNALS:
		void resultReady(int result);
	};

	/// This is semi-gross, it's the QObject which will be the parent of all managed future watchers.
	static inline FutureWatcherParent* f_the_managed_fw_parent = nullptr;

	/// Returns the pointer to the QThread which is to be used for running QFutureWatchers which
	/// implement inter-future status propagation (cancellation and exceptions).
	static inline QThread* get_backprop_qthread()
	{
		static QThread* backprop_thread = []{
			QThread* new_thread = new QThread;
			new_thread->setObjectName("ExtFutureBackpropThread");
			PerfectDeleter::instance().addQThread(new_thread, [](QThread* the_qthread){
				// Call exit(0) on the QThread.  We use Qt's invokeMethod() here.
				ExtAsync::detail::run_in_event_loop(the_qthread, [the_qthread](){
					qDb() << "Calling quit()+wait() on managed FutureWatcher QThread, FWParent has num children:" << f_the_managed_fw_parent->children().size();
					the_qthread->quit();
					the_qthread->wait();
					qDb() << "Finished quit()+wait() on managed FutureWatcher QThread";
				});
			});

			// No parent, this will be deleted by the signal below.
			f_the_managed_fw_parent = new FutureWatcherParent();
			// Create an push the future watcher parent object into the new thread.
			f_the_managed_fw_parent->moveToThread(new_thread);
			connect_or_die(new_thread, &QThread::finished, f_the_managed_fw_parent, &QObject::deleteLater);

			// Start the thread.
			new_thread->start();
			return new_thread;
		}();
		return backprop_thread;
	}



	/**
	 * Part of the system by which we get an exception from one ExtFuture<> into the state of another.
	 * This rethrows @a eptr, catches it, and finally reports it to @a future.
	 * @test Has test coverage, does work as expected.
	 * @param eptr
	 * @param future
	 */
	template <class R>
	void propagate_eptr_to_future(std::exception_ptr eptr, ExtFuture<R> future)
	{
		if(eptr)
		{
			// We did catch an exception.  Propagate it to the downstream_future.
			try
			{
				std::rethrow_exception(eptr);
			}
			catch(QException& e)
			{
				future.reportException(e);
			}
			catch(...)
			{
				future.reportException(QUnhandledException());
			}
			// future state here is (Started|Canceled|Running)/has_exception == true.
			// We need to finish it or any waits will block forever.
			future.reportFinished();
		}
		else
		{
			// eptr == null, there was no exception.
			Q_ASSERT(0);
		}
	}

	/**
	 * The other part of the exception propagation mechanism.
	 *
	 * @test Has test coverage, does work as expected.
	 */
	/// @note Only use this if you know upstream has an exception to throw to downstream.  Calls .wait() on upstream_future.
	template <class T, class R>
	void trigger_exception_and_propagate(ExtFuture<T> upstream_future, ExtFuture<R> downstream_future)
	{
		std::exception_ptr eptr;
		try
		{
			// Trigger the throw.
			upstream_future.wait();
		}
		catch(...)
		{
			// Capture the exception.
			eptr = std::current_exception();
		}
		propagate_eptr_to_future(eptr, downstream_future);
	}


	/**
	 * Factory function for creating managed QFutureWatcher<>s.
	 * @returns A pointer to the new watcher.  This is a raw pointer, and the watcher has no parent.
	 */
	template <class T>
	QFutureWatcher<T>* get_managed_qfuture_watcher(const char* watcher_name = "[watcher]")
	{
		QThread* bp_thread = get_backprop_qthread();
		// Create the new watcher, unparented so we can move it to the backprop thread.
		QFutureWatcher<T>* watcher = new QFutureWatcher<T>();

		watcher->setObjectName(watcher_name);

		// Move the watcher to the backprop thread.
		/// @note Per Qt5: "The object cannot be moved if it has a parent."
		watcher->moveToThread(bp_thread);

		// Ok, now it's in the other thread but has no parent.
		// We'll give it one.
		watcher->setParent(f_the_managed_fw_parent);
		qDb() << "FutureWatcher" << watcher << "has parent:" << watcher->parent();

		return watcher;
	}

	/**
	 * Connect a watcher between @a up and @a down which will propagate a cancel and any
	 * exception held by down to up, then deleteLater() itself.
	 */
	template <class T, class R, class StapCallback = std::nullptr_t>
	void connect_or_die_backprop_cancel_watcher(ExtFuture<T> up, ExtFuture<R> down, StapCallback&& stap_callback = nullptr)
	{
		auto* fw = get_managed_qfuture_watcher<R>("[down->up]");
		auto* fw_up = get_managed_qfuture_watcher<T>("[up->down]");

		/// @todo Still need to delete on R-finished, ...? T finished?

		/// @todo resultsReadyAt() watcher.
		constexpr bool has_stap_callback = !std::is_null_pointer_v<StapCallback>;
		if constexpr(!std::is_null_pointer_v<StapCallback>)
		{
			connect_or_die(fw_up, &QFutureWatcher<T>::resultsReadyAt,
					[upc=up, downc=down, fw_up, stap_callback_cp=FWD_DECAY_COPY(StapCallback, stap_callback)](int begin, int end) mutable {
				std::invoke(std::move(stap_callback_cp), upc, begin, end);
				/// @note We're temporarily copying to the output future here, we should change that to use a separate thread.
				///       Or maybe we can just return the upstream_future here....
				for(int i = begin; i < end; ++i)
				{
					downc.reportResult(fw_up->resultAt(i), i);
				}
			});
		}
		/// @todo resultsReadyAt() watcher.

		// down->up canceled.
		connect_or_die(fw, &QFutureWatcher<R>::canceled, [upc=DECAY_COPY(up), downc=down, fw]() mutable {
			// Propagate the cancel upstream, possibly with an exception.
			// Not a race here, since we'll have been canceled by the exception when we get here.
			qDb() << "down->up canceled";
			if(downc.has_exception())
			{
				// Note: Order flipped here, function propagates exception from param1 to param2.
			    trigger_exception_and_propagate(downc, upc);
			}
			else
			{
				upc.cancel();
				upc.reportFinished();
			}
			// Delete this watcher, it's done all it can.
			fw->deleteLater();
		});
		// up->down finished.
		connect_or_die(fw_up, &QFutureWatcher<T>::finished, [upc=up, downc=down, fw_up]() mutable {
			// Propagate the finish downstream, possibly with an exception.
			// Not a race here, since we'll have been canceled by the exception when we get here.
			qDb() << "up->down finished";
			if(upc.has_exception())
			{
				trigger_exception_and_propagate(upc, downc);
			}
			else
			{
				downc.reportFinished();
			}
			// Delete this watcher, it's done all it can.
			fw_up->deleteLater();
		});

		fw->setFuture(down);
		fw_up->setFuture(up);
	}

	/**
	 * Set up watchers and signals between @a up and @a down for a .then() pair.
	 */
	template <class T, class R, class ThenCallback, class... Args>
	void connect_or_die_then_watchers(ExtFuture<T> up, ExtFuture<R> down, ThenCallback&& then_callback, Args&&... args)
	{
		auto* fw_down = get_managed_qfuture_watcher<R>("[then down->up]");
		auto* fw_up = get_managed_qfuture_watcher<T>("[then up->down]");

		/// @todo Still need to delete on R-finished, ...? T canceled?

		// down->up canceled.
		connect_or_die(fw_down, &QFutureWatcher<R>::canceled, [upc=DECAY_COPY(up), downc=down, fw_down]() mutable {
			// Propagate the cancel upstream, possibly with an exception.
			// Not a race here, since we'll have been canceled by the exception when we get here.
			qDb() << "down->up canceled";
			if(downc.has_exception())
			{
				// Note: Order flipped here, function propagates exception from param1 to param2.
			    trigger_exception_and_propagate(downc, upc);
			}
			else
			{
				upc.cancel();
				upc.reportFinished();
			}
			// Delete this watcher, it's done all it can.
			fw_down->deleteLater();
		});
		// up->down finished.
		/// @todo Probably need a context here.
		connect_or_die(fw_up, &QFutureWatcher<T>::finished,
					   [upc=up, downc=down, fw_up, then_callback_cp=FWD_DECAY_COPY(ThenCallback, then_callback)]() mutable {
			// Propagate the finish downstream, possibly with an exception.
			// Not a race here, since we'll have been canceled by the exception when we get here.
			qDb() << "up->down finished";
			if(upc.has_exception())
			{
				trigger_exception_and_propagate(upc, downc);
			}
			else
			{
				// Normal finish.  Send the results to the .then() callback.
				try
				{
					// Call the callback with the results- or canceled/exception-laden this_future_copy.
					// Could throw, hence we're in a try.
					qDb() << "then_watchers: Calling then_callback_copy(this_future_copy).";
					R retval;

					if constexpr(std::is_same_v<R, Unit>)
					{
						// then_callback_copy returns void, return a Unit separately.
						std::invoke(std::move(then_callback_cp), upc);
						retval = unit;
					}
					else
					{
						// then_callback_copy returns non-void, return the callback's return value.
						retval = std::invoke(std::move(then_callback_cp), upc);
						// Didn't throw, report the result.
						downc.reportResult(retval);
					}

				}
				catch(...)
				{
					std::exception_ptr eptr = std::current_exception();
					propagate_eptr_to_future(eptr, downc);
				}
				downc.reportFinished();
			}
			// Delete this watcher, it's done all it can.
			fw_up->deleteLater();
		});

		fw_down->setFuture(down);
		fw_up->setFuture(up);
	}

	template<class R, class T>
	void connect_or_die_down2up_cancel(QPointer<QFutureWatcher<R>> downstream, QObject* context, ExtFuture<T> upstream_future)
	{
		// If down is canceled, cancel upstream.
		// If it was upstream that did the canceling, this effectively is a no-op.
		connect_or_die(downstream, &QFutureWatcher<R>::canceled, context, [=,upstream_future=upstream_future](){
			if(downstream->has_exception())
			{
				// Throw exception to upstream future.  Takes a bit of shenanigans.
				trigger_exception_and_propagate(downstream, upstream_future);
			}
			upstream_future.cancel();
		});
	}

	template<class R, class T>
	void connect_or_die_down2up_cancel(QPointer<QFutureWatcher<R>> downstream, QPointer<QFutureWatcher<T>> upstream_future_watcher)
	{
		// If down is canceled, cancel upstream.
		// If it was upstream that did the canceling, this effectively is a no-op.
		connect_or_die(downstream, &QFutureWatcher<R>::canceled, upstream_future_watcher,
				[=](){
			ExtFuture<R> downf = downstream->future();
			ExtFuture<T> upf = upstream_future_watcher->future();
			if(downf.has_exception())
			{
				// Throw exception to upstream future.  Takes a bit of shenanigans.
				trigger_exception_and_propagate(downf, upf);
			}
			upstream_future_watcher->cancel();
		});
	}

	template <class T>
	void connect_or_die_deleteLater_on_finished(QPointer<QFutureWatcher<T>> fw)
	{
		auto delete_later_if_canceled_and_finished = [](QPointer<QFutureWatcher<T>> fw)
		{
			if(/*m_watcher->isCanceled() &&*/ fw->isFinished())
			{
				qDb() << "QFutureWatcher<> is finished (maybe canceled), deleting:" << fw;
				fw->deleteLater();
			}
		};

		connect_or_die(fw, &QFutureWatcher<T>::finished,
					   [=,
					   delete_later_if_canceled_and_finished=DECAY_COPY(delete_later_if_canceled_and_finished)]() mutable {
			delete_later_if_canceled_and_finished(fw);
		});
		connect_or_die(fw, &QFutureWatcher<T>::canceled,
				[=]() mutable {
			delete_later_if_canceled_and_finished(fw);
		});
	}

	template <class T, class R, class ResultsReadyAtCallbackType = std::nullptr_t, class WatcherConnectionCallback = std::nullptr_t>
	void SetBackpropWatcher(ExtFuture<T> upstream_future, ExtFuture<R> downstream_future,
							QObject* upstream_context, QObject* downstream_context,
							ResultsReadyAtCallbackType&& results_ready_callback = nullptr,
							WatcherConnectionCallback&& wc_callback = nullptr)
	{
		QThread* bp_thread = get_backprop_qthread();
		if(upstream_context == nullptr)
		{
			upstream_context = f_the_managed_fw_parent;
		}
		if(downstream_context == nullptr)
		{
			downstream_context = f_the_managed_fw_parent;
		}

		using FutureWatcherTypeR = QFutureWatcher<R>;
		using FutureWatcherTypeT = QFutureWatcher<T>;

		// Watchers will live in the backprop thread.
		QPointer<FutureWatcherTypeR> downstream_watcher = get_managed_qfuture_watcher<R>();
		QPointer<FutureWatcherTypeT> upstream_watcher = get_managed_qfuture_watcher<T>();

		// R->T ("upstream")
		// cancel signal.
		connect_or_die_down2up_cancel(downstream_watcher, upstream_watcher);
	//		connect_or_die(downstream_watcher, &FutureWatcherTypeR::canceled, upstream_context, [=,
	//					   upstream_future_copy=DECAY_COPY(std::forward<ExtFuture<T>>(upstream_future))]() mutable {
	//			// Note we directly call cancel() (but from context's thread) because upstream_future may not have an event loop.
	//			upstream_future_copy.cancel();
	//		});
		// finished signal.
		/// @note Should only ever get this due to an exception thrown into R, and then we should probably have gotten a cancel instead.
		connect_or_die(downstream_watcher, &FutureWatcherTypeR::finished, upstream_context, [=,
					   upstream_future_copy=FWD_DECAY_COPY(ExtFuture<T>, upstream_future)]() mutable {
			// Note we directly call cancel() (but from context's thread) because upstream_future may not have an event loop.
			upstream_future_copy.reportFinished();
		});

		// T->R ("downstream") signals.
		// The resultsReadyAt() signal -> results_ready_callback.
	//		if constexpr(std::is_invocable_r_v<void, ResultsReadyAtCallbackType, int, int>)
	//		{
			connect_or_die(upstream_watcher, &FutureWatcherTypeR::resultsReadyAt, downstream_context,
						   [=,
						   upstream_future_copy=DECAY_COPY(/*std::forward<ExtFuture<T>>*/(upstream_future)),
						   callback_cp=DECAY_COPY(std::forward<ResultsReadyAtCallbackType>(results_ready_callback))](int begin, int end) mutable {
				std::invoke(callback_cp, upstream_future, begin, end);
				/// @note We're temporarily copying to the output future here, we should change that to use a separate thread.
				///       Or maybe we can just return the upstream_future here....
				for(int i = begin; i < end; ++i)
				{
					downstream_future.reportResult(upstream_future_copy, i);
				}
			});
	//		}
	//		// The T->R data xfer.
	//		connect_or_die(upstream_watcher, &FutureWatcherTypeR::resultsReadyAt, downstream_context,
	//		               [=,upstream_future_copy=DECAY_COPY(/*std::forward<ExtFuture<T>>*/(upstream_future)),
	//				          callback_cp=DECAY_COPY(std::forward<ResultsReadyAtCallbackType>(results_ready_callback))](int begin, int end) mutable {
	//			               /// @note We're temporarily copying to the output future here, we should change that to use a separate thread.
	//			               ///       Or maybe we can just return the upstream_future here....
	//			               for(int i = begin; i < end; ++i)
	//			               {
	//				               downstream_future.reportResult(upstream_future_copy, i);
	//			               }
	//		               });
		// Canceled.
		connect_or_die(upstream_watcher, &FutureWatcherTypeT::canceled, downstream_context,
				[=, downstream_future_copy=DECAY_COPY(downstream_future)]() mutable {
			if(upstream_future.has_exception())
			{
				// Throw exception to downstream future.  Takes a bit of shenanigans.
				trigger_exception_and_propagate(upstream_future, downstream_future_copy);
			}
			// This probably gets ignored.
			qDb() << "########## IGNORING?";
			downstream_future_copy.reportCanceled();
			qDb() << "########## IGNORING.";
		});
		// Finished.
		connect_or_die(upstream_watcher, &FutureWatcherTypeT::finished, downstream_context,
					   [=]() mutable { downstream_future.reportFinished(); });

		if constexpr(std::is_invocable_r_v<void, WatcherConnectionCallback, QPointer<QFutureWatcher<T>>, QPointer<QFutureWatcher<R>>>)
		{
			// Let the caller do any last-minute connections to the watchers.
			std::invoke(wc_callback, upstream_watcher, downstream_watcher);
		}

		// Connect these up last, they signal the watchers to deleteLater() themselves.
	//		connect_or_die_deleteLater_on_finished(downstream_watcher);
	//		connect_or_die_deleteLater_on_finished(upstream_watcher);

		// And set the futures.  It's just that easy.
		downstream_watcher->setFuture(downstream_future);
		upstream_watcher->setFuture(upstream_future);

	}

} // END ns ExtFutureWatcher_impl


#endif /* SRC_CONCURRENCY_IMPL_MANAGEDEXTFUTUREWATCHER_IMPL_H_ */
