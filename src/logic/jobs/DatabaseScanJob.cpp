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
 * @file DatabaseScanJob.cpp
 */
#include <jobs/DatabaseScanJob.h>

// Qt5
#include <QUrl>

// Ours
#include <utils/DebugHelpers.h>
#include <concurrency/ExtAsync.h>
#include <models/ScanResultsTreeModel.h>
#include <Core.h>
#include <gui/activityprogressmanager/ActivityProgressStatusBarTracker.h>
#include <gui/MainWindow.h>
#include <logic/SupportedMimeTypes.h>
#include <AMLMApp.h>
#include <utils/ext_iterators.h>
#include <models/SRTMItemLibEntry.h>
#include "DirectoryScanJob.h"

/**
 * (Re-)Load the given tree model with a scan of the contents of @a dir_url.
 * @param dir_url
 * @param srtm
 */
void DatabaseScanJob::startAsyncDirectoryTraversal_ForDB(const QUrl& dir_url, std::shared_ptr<ScanResultsTreeModel> srtm)
{
	/// @todo We can't use srtm currently, since it seems to be changing out from under us after we capture it but before we
	/// use it in the lambdas.

	Q_ASSERT(srtm);

	// To save some typing.
	using ItemContType = std::vector<std::shared_ptr<AbstractTreeModelItem>>;
	using SharedItemContType = std::shared_ptr<ItemContType>;

	std::shared_ptr<ScanResultsTreeModel> original_srtm = srtm;

	// Get a ptr to the app for use if we need a GUI thread object.
	auto app_this = amlmApp;// QCoreApplication::instance()->thread();

	qDb() << "START:" << dir_url;

//	expect_and_set(0, 1);

	// Time how long all this takes.
	m_timer.start("############ startAsyncDirectoryTraversal()");

	// Clear it out.
	srtm->clear();

	// Set the root URL of the scan results model.
	/// @todo Should this really be done here, or somewhere else?
	srtm->setBaseDirectory(dir_url);
	// Set the default columnspecs.
	/// @todo I don't know why I hate this so much.  So very very much.
	srtm->setColumnSpecs(AMLM::Core::self()->getDefaultColumnSpecs());

	auto master_job_tracker = MainWindow::master_tracker_instance();
	Q_CHECK_PTR(master_job_tracker);

	// Get a list of the file extensions we're looking for.
	auto extensions = SupportedMimeTypes::instance().supportedAudioMimeTypesAsSuffixStringList();

	// Run the directory scan in another thread.
	ExtFuture<DirScanResult> dirresults_future = ExtAsync::qthread_async_with_cnr_future(DirScanFunction, nullptr,
	                                                                                     dir_url,
	                                                                                     extensions,
	                                                                                     QDir::Filters(QDir::Files |
	                                                                                                   QDir::AllDirs |
	                                                                                                   QDir::NoDotAndDotDot),
	                                                                                     QDirIterator::Subdirectories);
	// Create/Attach an AMLMJobT to the dirscan future.
	QPointer<AMLMJobT<ExtFuture<DirScanResult>>> dirtrav_job = make_async_AMLMJobT(dirresults_future, "DirResultsAMLMJob", AMLMApp::instance());

	// Create a future so we can attach a continuation to get the results to the main thread.
	ExtFuture<SharedItemContType> tree_model_item_future = ExtAsync::make_started_only_future<SharedItemContType>();

	/// @note Need this to capture shared ptr?
	auto dirresults_future_stap_callback = [
			tree_model_item_future
		](ExtFuture<DirScanResult> tap_future, int begin, int end) mutable -> void {
		// Start of the dirtrav tap callback.  This should be a non-main thread.
		AMLM_ASSERT_NOT_IN_GUITHREAD();
//		AMLM_ASSERT_IN_GUITHREAD();

		auto tree_model_sptr = AMLM::Core::self()->getScanResultsTreeModel();

		Q_ASSERT(tree_model_sptr == AMLM::Core::self()->getScanResultsTreeModel());

		if(!tree_model_sptr)
		{
			qCr() << "Shared ptr problem";
//			qCr() << M_ID_VAL(tree_model_sptr.unique()) << M_ID_VAL(srtm.unique());
		}

		if(begin == 0)
		{
//			expect_and_set(1, 2);
		}

		// Create a new container instance we'll use to pass the incoming values to the GUI thread below.
		/// @todo We should find a better way to do this sort of thing.
		/// Maybe a multi-output .tap()?
		/// @note This is really populated with as type vector<shared_ptr<ScanResultsTreeModelItem>>
		SharedItemContType new_items = std::make_shared<ItemContType>();

		int original_end = end;
		for(int i = begin; i < end; i++)
		{
			DirScanResult dsr = tap_future.resultAt(i);

			// Add another entry to the vector we'll send to the model.
//			new_items->emplace_back(std::make_shared<ScanResultsTreeModelItem>(dsr, tree_model));
			/// @todo This is in a non-GUI thread.
//			Q_ASSERT(tree_model_sptr);
//			auto new_item = ScanResultsTreeModelItem::construct(dsr);
			auto new_item = std::make_shared<ScanResultsTreeModelItem>(dsr);
			new_items->emplace_back(new_item);

			if(i >= end)
			{
				// We're about to leave the loop.  Check if we have more ready results now than was originally reported.
				if(tap_future.isResultReadyAt(end + 1) || tap_future.resultCount() > (end + 1))
				{
					// There are more results now than originally reported.
					qIn() << "##### MORE RESULTS:" << M_NAME_VAL(original_end) << M_NAME_VAL(end)
					      << M_NAME_VAL(tap_future.resultCount());
					end = end + 1;
				}
			}

			if(tap_future.HandlePauseResumeShouldICancel())
			{
				tap_future.reportCanceled();
				break;
			}
		}
		// Broke out of loop, check for problems.
		if(tap_future.isCanceled())
		{
			// We've been canceled.
			/// @todo Not sure if we should see that in here or not, probably not.
			qWr() << "tap_callback saw canceled";
			return;
		}
		if(tap_future.isFinished())
		{
			qIn() << "tap_callback saw finished";
			if(new_items->empty())
			{
				qWr() << "tap_callback saw finished/empty new_items";

				return;
				qIn() << "tap_callback saw finished, but with" << new_items->size() << "outstanding results.";
			}
		}

		// Shouldn't get here with no incoming items.
		Q_ASSERT_X(!new_items->empty(), "DIRTRAV CALLBACK", "NO NEW ITEMS BUT HIT TAP CALLBACK");

		// Got all the ready results, send them to the model(s).
		// Because of Qt5's model/view system not being threadsafeable, we have to do at least the final
		// model item entry from the GUI thread.
//		qIn() << "Sending" << new_items->size() << "scan results to models";

		/// @note This could also be a signal emit.
		Q_ASSERT(new_items->size() == 1);
		tree_model_item_future.reportResult(new_items);

//		qDb() << "END OF DSR TAP:" << M_ID_VAL(tree_model_item_future);
	};

	// Attach a streaming tap to the dirscan future.
	ExtFuture<Unit> tail_future
			= dirresults_future.stap(dirresults_future_stap_callback)
        .then([=](ExtFuture<DirScanResult> fut_ignored) -> Unit {
            // The dirscan is complete.
			qDb() << "DIRSCAN COMPLETE .then()";

            return unit;
		})
    /// @then Finish the two output futures.
	.then([=, tree_model_item_future=tree_model_item_future](ExtFuture<Unit> future) mutable {

	   // Finish a couple futures we started in this, and since this is done, there should be no more
	   // results coming for them.

	//				                   expect_and_set(2, 3);
	   AMLM_ASSERT_NOT_IN_GUITHREAD();

	   qDb() << "FINISHING TREE MODEL FUTURE:" << M_ID_VAL(tree_model_item_future); // == (Running|Started)
	   tree_model_item_future.reportFinished();
	   qDb() << "FINISHED TREE MODEL FUTURE:" << M_ID_VAL(tree_model_item_future); // == (Started|Finished)
	//				                   m_timer.lap("Finished tree_model_item_future");


	});

#if 1
#define TREE_ITEM_MODEL_POP_NON_GUI_THREAD 1

	/// Append TreeModelItems to the ScanResultsTreeModel tree_model.
//	Q_ASSERT(tree_model_weak);
	tree_model_item_future.stap(
#if TREE_ITEM_MODEL_POP_NON_GUI_THREAD != 1
		                      app_this,
#endif
	[=](ExtFuture<SharedItemContType> new_items_future,
	                                           int begin_index, int end_index) mutable {
		// Get the current ScanResultsTreeModel.
		std::shared_ptr<ScanResultsTreeModel> tree_model_sptr = AMLM::Core::self()->getScanResultsTreeModel();

		Q_ASSERT(tree_model_sptr);
#if TREE_ITEM_MODEL_POP_NON_GUI_THREAD != 1
      AMLM_ASSERT_IN_GUITHREAD();
#else
      AMLM_ASSERT_NOT_IN_GUITHREAD();
#endif

//		qDb() << "START: tree_model_item_future.stap(), new_items_future count:" << new_items_future.resultCount();

      // For each QList<SharedItemContType> entry.
      for(int index = begin_index; index < end_index; ++index)
      {
          auto result = new_items_future.resultAt(index);
          const SharedItemContType& new_items_vector_ptr = result;

          // Append ScanResultsTreeModelItem entries to the ScanResultsTreeModel.
          for(std::shared_ptr<AbstractTreeModelItem>& entry : *new_items_vector_ptr)
          {
              // Make sure the entry wasn't moved from.
              Q_ASSERT(bool(entry) == true);
//				Q_ASSERT(entry->isInModel());
	          auto entry_dp = std::dynamic_pointer_cast<ScanResultsTreeModelItem>(entry);
	          Q_ASSERT(entry_dp);

              // Here we're only dealing with the per-file LibraryEntry's.
              M_WARNING("THIS POPULATE CAN AND SHOULD BE DONE IN ANOTHER THREAD");
              std::shared_ptr<LibraryEntry> lib_entry = LibraryEntry::fromUrl(entry_dp->data(1).toString());
              lib_entry->populate(true);

              std::shared_ptr<SRTMItem_LibEntry> new_child = std::make_shared<SRTMItem_LibEntry>(lib_entry);
              Q_ASSERT(new_child);

              /// NEW: Give the incoming ScanResultTreeModelItem entry a parent.
              M_WARNING("TODO: This needs rework.");
//				entry_dp->changeParent(tree_model_sptr->getRootItem());
              tree_model_sptr->getRootItem()->appendChild(entry_dp);
              entry_dp->appendChild(new_child);
          }

          // Finally, move the new model items to their new home.
          Q_ASSERT(new_items_vector_ptr->at(0));

          /// @temp
          bool ok = tree_model_sptr->checkConsistency();
          Q_ASSERT(ok);
			qDb() << "########################### TREE MODEL CHECK checkConsistency:" << ok;

			qDb() << "TREEMODELPTR:" << M_ID_VAL(tree_model_sptr->rowCount());
          }

      })
#endif
			.then([&](ExtFuture<SharedItemContType> f) mutable {
              Q_ASSERT(f.isFinished());
//		                      Q_ASSERT(m_model_ready_to_save_to_db == false);
//		                      m_model_ready_to_save_to_db = true;
              /// @todo This is happening immediately, and also before "Finished tree_model_item_future" & qurl_future.
//		                      m_timer.lap("TreeModelItems stap() finished.");
              return unit;
          })
          .then(qApp, [=, this,
			                      tree_model_ptr=srtm,
								  kjob=/*FWD_DECAY_COPY(QPointer<AMLMJobT<ExtFuture<DirScanResult>>>,*/ dirtrav_job/*)*///,
//			                      &lib_rescan_job
	                      ](ExtFuture<Unit> future_unit) mutable {

				AMLM_ASSERT_IN_GUITHREAD();

//		                      m_timer.lap("GUI Thread dirtrav over start.");

//		                      expect_and_set(3, 4);

                qDb() << "DIRTRAV COMPLETE, NOW IN GUI THREAD";
				if(0)//kjob.isNull())
				{
				  M_WARNING("kjob is now null here and we fail");
				  Q_ASSERT_X(0, __func__, "Dir scan job was deleted");
				}
				if(0)//kjob->error())
				{
				  qWr() << "DIRTRAV FAILED:" << kjob->error() << ":" << kjob->errorText() << ":" << kjob->errorString();
				  auto uidelegate = kjob->uiDelegate();
				  Q_CHECK_PTR(uidelegate);
				  uidelegate->showErrorMessage();
				}
				else
				{
					// Succeeded, but we may still have outgoing filenames in flight.
					qIn() << "DIRTRAV SUCCEEDED";
					m_timer.lap("DirTrav succeeded");
					qIn() << "Directory scan time params:";
					m_timer.print_results();

					// Save the database out to XML.
					QString database_filename = QDir::homePath() + "/AMLMDatabase.xml";

//			                      Q_ASSERT(m_model_ready_to_save_to_db == true);
//			                      m_model_ready_to_save_to_db = false;

                  // Save the database.
					{
//						m_timer.scoped_lap("tree_model_ptr->SaveDatabase");
						m_timer.lap("SaveDatabase START");
						tree_model_ptr->SaveDatabase(database_filename);
						m_timer.lap("SaveDatabase END");
					}

				////////// EXPERIMENTAL
#if 1 //// PUT BACK
				//// Save the database using the root model method.
				tree_model_ptr->AbstractTreeModel::SaveDatabase(QDir::homePath() +"/AMLMDatabase_AsATM.xml");
				////
			                      /// Try to load it back in and round-trip it.
//				std::initializer_list<ColumnSpec> temp_initlist = {ColumnSpec(SectionID(0), "DirProps"), {SectionID(0), "MediaURL"}, {SectionID(0), "SidecarCueURL"}};
					std::shared_ptr<ScanResultsTreeModel> load_tree	= ScanResultsTreeModel::make_ScanResultsTreeModel({});

					load_tree->LoadDatabase(database_filename);
//				load_tree->clear();
//				dump_map(load_tree);
					load_tree->SaveDatabase(QDir::homePath() +"/AMLMDatabaseRT.xml");
#endif /////
				m_timer.print_results();
				}
			});

	// Add the AMLMJobT to the tracker.
	master_job_tracker->registerJob(dirtrav_job);

	// Make sure the above job gets canceled and deleted.
	AMLMApp::IPerfectDeleter().addQFuture(tail_future);
	AMLMApp::IPerfectDeleter().addExtFuture(tree_model_item_future);
}
