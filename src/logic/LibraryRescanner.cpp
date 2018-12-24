/*
 * Copyright 2017, 2018 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/** @file Implementation of LibraryRescanner, an asynchronous helper for LibraryModel. */

#include "LibraryRescanner.h"
#include "SupportedMimeTypes.h"

/// Std C++
#include <functional>

/// Qt5
#include <QThread>
#include <QXmlFormatter>
#include <QXmlQuery>
#include <QVariant>
#include <QtConcurrent>
#include <QXmlResultItems>

/// KF5
#include <KJobUiDelegate>
#include <KIO/DirectorySizeJob>

/// Ours
#include <AMLMApp.h>
#include <gui/MainWindow.h>
#include <logic/models/AbstractTreeModelItem.h>
#include <logic/models/ScanResultsTreeModel.h>
#include <utils/DebugHelpers.h>

/// Ours, Qt5/KF5-related
#include <utils/TheSimplestThings.h>
#include <utils/RegisterQtMetatypes.h>
#include <utils/QtHelpers.h>

#include <concurrency/ExtAsync.h>
#include <concurrency/AsyncTaskManager.h>
#include <logic/jobs/DirectoryScanJob.h>

#include "LibraryRescannerJob.h"
#include <gui/activityprogressmanager/ActivityProgressStatusBarTracker.h>
#include <logic/serialization/XmlSerializer.h>
#include <logic/serialization/SerializationExceptions.h>
#include <utils/Stopwatch.h>

#include "logic/LibraryModel.h"


AMLM_QREG_CALLBACK([](){
	qIn() << "Registering LibraryRescanner types";
	// From #include <logic/LibraryRescanner.h>
	qRegisterMetaType<MetadataReturnVal>();
	qRegisterMetaType<QFuture<MetadataReturnVal>>();
	qRegisterMetaType<VecLibRescannerMapItems>();
    });


LibraryRescanner::LibraryRescanner(LibraryModel* parent) : QObject(parent)
{
	setObjectName("TheLibraryRescanner");

	// Somewhat redundant, but keep another pointer to the LibraryModel.
	m_current_libmodel = parent;
}

LibraryRescanner::~LibraryRescanner()
{
M_WARNING("TODO: THIS SHOULD CANCEL THE JOBS, OR THE JOBS SHOULDNT BE OWNED BY THIS");
}


MetadataReturnVal LibraryRescanner::refresher_callback(const VecLibRescannerMapItems& mapitem)
{
    MetadataReturnVal retval;
#if 0
	qDebug() << "Current thread:" << QThread::currentThread()->objectName();

	// If we have more than a single entry in the incoming list, we have a multi-track file to refresh.
	if(mapitem.size() == 1)
	{
		// Only one entry.

		// Get the LibraryEntry* to the existing entry.
M_WARNING("There's no locking here, there needs to be, or these need to be copies.");
		std::shared_ptr<LibraryEntry> item = mapitem[0].item;

		if(!item->isPopulated())
		{
			// Item's metadata has not been looked at.  We may have multiple tracks.

			// Only one pindex though.
			retval.m_original_pindexes.push_back(mapitem[0].pindex);

			auto vec_items = item->populate();
			for (auto i : vec_items)
			{
				if (!i->isPopulated())
				{
					qCritical() << "NOT POPULATED" << i.get();
				}
				retval.push_back(i);
			}
		}
		else if (item->isPopulated() && item->isSubtrack())
		{
			qCritical() << "TODO: FOUND SUBTRACK ITEM, SKIPPING:" << item->getUrl();
			Q_ASSERT(0);
		}
		else
		{
			//qDebug() << "Re-reading metatdata for item" << item->getUrl();
			std::shared_ptr<LibraryEntry> new_entry = item->refresh_metadata();

			if(new_entry == nullptr)
			{
				// Couldn't load the metadata from the file.
				// Only option here is to return the old item, which should now be marked with an error.
				qCritical() << "Couldn't load metadata for file" << item->getUrl();
				retval.m_original_pindexes.push_back(mapitem[0].pindex);
				retval.m_new_libentries.push_back(item);
				retval.m_num_tracks_found = 1;
			}
			else
			{
				// Repackage it and return.
				retval.m_original_pindexes.push_back(mapitem[0].pindex);
				retval.m_new_libentries.push_back(new_entry);
				retval.m_num_tracks_found = 1;
			}
		}
	}
	else if (mapitem.size() > 1)
	{
		// Multiple incoming tracks.
		std::shared_ptr<LibraryEntry> first_item = mapitem[0].item;
		auto subtracks = first_item->populate(true);
		if(subtracks.size() < mapitem.size())
		{
			// We got fewer back than we had before.
			if(subtracks.size() == 1)
			{
				// We only got one back.  This means we weren't able to read the file.
				return retval;
			}
			else
			{
				// Not sure what exactly we got back.
				qCritical() << "Unknown response: mapitem.size()==" << mapitem.size() << "subtracks.size()==" << subtracks.size();
				Q_ASSERT(0);
			}
		}
		for(int i=0; i<mapitem.size(); ++i)
		{
			Q_ASSERT(subtracks[i] != nullptr);
			retval.push_back(mapitem[i].pindex, subtracks[i]);
		}
	}
	else
	{
		qCritical() << "GOT EMPTY LIST OF LIBRARY ENTRIES TO RESCAN";
	}
#endif
    return retval;
}

void LibraryRescanner::startAsyncDirectoryTraversal(QUrl dir_url)
{
    qDb() << "START:" << dir_url;

	// Time how long it takes.
	m_timer.start();

    auto master_job_tracker = MainWindow::master_tracker_instance();
    Q_CHECK_PTR(master_job_tracker);

    auto extensions = SupportedMimeTypes::instance().supportedAudioMimeTypesAsSuffixStringList();
#if 1
    DirectoryScannerAMLMJobPtr dirtrav_job = DirectoryScannerAMLMJob::make_job(this, dir_url, extensions,
									QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
#else
    /// Return type here is: std::unique_ptr<AMLMJobT<ExtFutureT>>
M_TODO("This isn't scanning.");
    SHARED_PTR<AMLMJobT<ExtFuture<DirScanResult>>> dirtrav_job = make_async_AMLMJobT(
    		DirectoryScannerAMLMJob::AsyncDirScan(dir_url, extensions,
    				QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories), this);
#endif

    // Makes a new AMLMJobT.
	LibraryRescannerJobPtr lib_rescan_job = LibraryRescannerJob::make_job(this);

    // Get a pointer to the Scan Results Tree model.
    auto tree_model = AMLMApp::instance()->IScanResultsTreeModel();
    // Set the root URL of the scan results model.
    /// @todo Should this really be done here, or somewhere else?
    tree_model->setBaseDirectory(dir_url);

	// Attach a streaming tap to get the results.
	ExtFuture<DirScanResult> tail_future
		= dirtrav_job->get_extfuture().tap([=](ExtFuture<DirScanResult> tap_future, int begin, int end) {
		std::vector<AbstractTreeModelItem*> new_items;
		int original_end = end;
		for(int i=begin; i<end; i++)
		{
			DirScanResult dsr = tap_future.resultAt(i);
			// Add another entry to the vector we'll send to the model.
			new_items.push_back(new ScanResultsTreeModelItem(dsr));

			if(i >= end)
			{
				// We're about to leave the loop.  Check if we have more ready results now than was originally reported.
				if(tap_future.isResultReadyAt(end+1) || tap_future.resultCount() > (end+1))
				{
					// There are more results now than originally reported.
					qIno() << "##### MORE RESULTS:" << M_NAME_VAL(original_end) << M_NAME_VAL(end) << M_NAME_VAL(tap_future.resultCount());
					end = end+1;
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
			qIno() << "tap_callback saw finished";
			if(new_items.empty())
			{
				qWr() << "tap_callback saw finished/empty new_items";
			}
			return;
		}

		// Shouldn't get here with no incoming items.
		Q_ASSERT_X(!new_items.empty(), "DIRTRAV CALLBACK", "NO NEW ITEMS BUT HIT TAP CALLBACK");

		// Got all the ready results, send them to the model(s).
		// We have to do this from the GUI thread unfortunately.
		qIno() << "Sending" << new_items.size() << "scan results to model";
		run_in_event_loop(this, [=, tree_model_ptr=tree_model](){

			// Append entries to the ScanResultsTreeModel.
			tree_model_ptr->appendItems(new_items);

	        /// @todo Obsoleting... very... slowly.
			for(const auto& entry : new_items)
			{
				// Send the URL ~dsr.getMediaExtUrl().m_url.toString()) to the LibraryModel.
				qIn() << "EMITTING:" << entry->data(1).toString();
				Q_EMIT m_current_libmodel->SLOT_onIncomingFilename(
							entry->data(1).toString());
			}
		});

	});

	// Make sure the above job gets canceled and deleted.
	AMLMApp::IPerfectDeleter()->addQFuture(tail_future);

	// START dirtrav_job->then()
	dirtrav_job->then(this, [=, tree_model_ptr=tree_model](DirectoryScannerAMLMJob* kjob) {
        qDb() << "DIRTRAV COMPLETE";
        if(kjob->error())
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
            m_last_elapsed_time_dirscan = m_timer.elapsed();
            qIn() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";

/// @todo EXPERIMENTAL
			QString filename = QDir::homePath() + "/DeleteMe.xspf";
			qIno() << "Writing model to XML file:" << filename;
			QFile outfile(filename);
			auto status = outfile.open(QFile::WriteOnly | QFile::Text);
			if(!status)
			{
				qCro() << "########## COULDN'T WRITE TO FILE:" << filename;
			}
			else
			{

//				AbstractTreeModelWriter tmw(tree_model_ptr);
//				tmw.write_to_iodevice(&outfile);
//				outfile.close();

				/// NEW Let's also try it with plenty of QVariants.
				QString database_filename = QDir::homePath() + "/DeleteMeNew.xspf";
				{

					qIn() << "###### WRITING" << database_filename;

					XmlSerializer xmlser;
					xmlser.set_default_namespace("http://xspf.org/ns/0/", "1");
					xmlser.save(*tree_model_ptr, QUrl::fromLocalFile(database_filename), "playlist");

					qIn() << "###### WROTE" << database_filename;

					/// Let's now try to read it back.
					ScanResultsTreeModel* readback_tree_model;
					{
						qIn() << "###### READING BACK" << database_filename;

						XmlSerializer xmlser_read;
						xmlser.set_default_namespace("http://xspf.org/ns/0/", "1");
						readback_tree_model = new ScanResultsTreeModel(this);//static_cast<QObject*>(tree_model_ptr)->parent());
						// Load it.
						xmlser_read.load(*readback_tree_model, QUrl::fromLocalFile(database_filename));

						qIn() << "###### READBACK INFO:";
						qIn() << "COLUMNS:" << readback_tree_model->columnCount()
								<< "ROWS:" << readback_tree_model->rowCount();
					}

					/// And lets' try to reserialize it out.
					{
						QString filename = QDir::homePath() + "/DeleteMeNew3.xspf";
						qIn() << "###### WRITING WHAT WE READ TO" << filename;
						XmlSerializer xmlser;
						xmlser.set_default_namespace("http://xspf.org/ns/0/", "1");
						xmlser.save(*readback_tree_model, QUrl::fromLocalFile(filename), "playlist");
						qIn() << "###### WROTE" << filename;
					}
					readback_tree_model->deleteLater();

				}
#ifndef TRY_XQUERY_READ
				// Now let's see if we can XQuery what we just wrote.
				auto outfile_url = QUrl::fromLocalFile(QDir::homePath() + "/DeleteMe_ListOfUrlsFound.xml");
				bool retval = run_xquery(QUrl::fromLocalFile(":/xquery_files/filelist.xq"),
						QUrl::fromLocalFile(database_filename), outfile_url);

				Q_ASSERT(retval);

				{
					Stopwatch sw("XQuery test: two regex queries in loop");

					for(QString ext_regex : {R"((.*\.flac$))", R"((.*\.mp3$))"})
					{
						// Now let's see if we can XQuery what we just wrote as a QStringList.
						QStringList query_results;
						retval = run_xquery(QUrl::fromLocalFile(":/xquery_files/filelist_stringlistout.xq"),
						                    QUrl::fromLocalFile(database_filename), &query_results,
						                    [&](QXmlQuery* xq) {
							                    xq->bindVariable(QString("extension_regex"), QVariant(ext_regex));
						                    });
						Q_ASSERT(retval);
						qDbo() << "###### NUM" << ext_regex << "FILES:" << query_results.count();
					}
				}

				ExpRunXQuery1(database_filename, filename);


				if(0)
				{
					// Open the file with the XQuery (in our resources).
				    QFile queryFile(QString(":/xquery_files/filelist.xq"));
				    queryFile.open(QIODevice::ReadOnly);

				    // Open the ouput file.
					QFile outfile2(QDir::homePath() + "/DeleteMe_ListOfUrlsFound.xml");
					auto status = outfile2.open(QFile::WriteOnly | QFile::Text);

					// Create the QXmlQuery, bind variables, and load the xquery.

				    QXmlQuery query;
				    QUrl in_filepath = QUrl::fromLocalFile(database_filename);
				    Q_ASSERT(in_filepath.isValid());
				    query.bindVariable("input_file_path", QVariant(in_filepath.toString()));

					// Read the XQuery as a QString.
					const QString query_string(QString::fromLatin1(queryFile.readAll()));
					// Set query.
					query.setQuery(query_string);
					Q_ASSERT(query.isValid());

					// String list for list results.
					QStringList xqout;
					// Formatter when we want to write another file.
					QXmlFormatter formatter(query, &outfile2);
					formatter.setIndentationDepth(2);

					// Run the query_string.
					if(!query.evaluateTo(&formatter))
					{

						Q_ASSERT(0);
					}
				}
#endif
			}
/// @todo EXPERIMENTAL


#if 1
            // Directory traversal complete, start rescan.

            QVector<VecLibRescannerMapItems> rescan_items;

            qDb() << "GETTING RESCAN ITEMS";

            rescan_items = m_current_libmodel->getLibRescanItems();

            qDb() << "rescan_items:" << rescan_items.size();
			Q_ASSERT(!rescan_items.empty());

            lib_rescan_job->setDataToMap(rescan_items, m_current_libmodel);

            // Start the metadata scan.
            qDb() << "STARTING RESCAN";
            lib_rescan_job->start();
#endif
        }
    }); // END dirtrav_job->then

    master_job_tracker->registerJob(dirtrav_job);
	master_job_tracker->setAutoDelete(dirtrav_job, false);
    master_job_tracker->setStopOnClose(dirtrav_job, true);
	master_job_tracker->registerJob(lib_rescan_job);
	master_job_tracker->setAutoDelete(lib_rescan_job, false);
	master_job_tracker->setStopOnClose(lib_rescan_job, true);

    // Start the asynchronous ball rolling.
    dirtrav_job->start();

	qDb() << "END:" << dir_url;
}

void LibraryRescanner::cancelAsyncDirectoryTraversal()
{
	m_dirtrav_future.cancel();
}

#if 0

void LibraryRescanner::startAsyncRescan(QVector<VecLibRescannerMapItems> items_to_rescan)
{
	// Send out progress text.
	QString progtext = tr("Rereading metadata");

    m_timer.start();

	ExtFuture<MetadataReturnVal> future = QtConcurrent::mapped(items_to_rescan,
	                                    std::bind(&LibraryRescanner::refresher_callback, this, _1));

	future.tap(this, [this](MetadataReturnVal a) {
		// The result is ready tap.
		this->processReadyResults(a);
	})
	.tap(this, [=](ExtAsyncProgress prog) {
		// Progress update tap.
		Q_EMIT this->progressChanged(prog.min, prog.val, prog.max, progtext);
	;})
	.finally([this](){
		qDb() << "METADATA RESCAN COMPLETE";
		onRescanFinished();
	});
}

#endif

void LibraryRescanner::processReadyResults(MetadataReturnVal lritem_vec)
{
	// We got one of ??? things back:
	// - A single pindex and associated LibraryEntry*, maybe new, maybe a rescan..
	// - A single pindex and more than one LibraryEntry*, the result of the first scan after the file was found.
	// - Multiple pindexs and LibraryEntry*'s.  The result of a multi-track file rescan.

	if(lritem_vec.m_num_tracks_found == 0)
	{
		qCritical() << "RESULT WAS EMPTY";
	}

	if(lritem_vec.m_num_tracks_found > 1
	   && lritem_vec.m_original_pindexes.size() == 1
			&& lritem_vec.m_new_libentries.size() == lritem_vec.m_num_tracks_found)
	{
		// It's a valid, new, multi-track entry.
		m_current_libmodel->SLOT_onIncomingPopulateRowWithItems_Multiple(lritem_vec.m_original_pindexes[0], lritem_vec.m_new_libentries);
	}
	else if(lritem_vec.m_new_libentries.size() == lritem_vec.m_num_tracks_found
			&& lritem_vec.m_original_pindexes.size() == lritem_vec.m_num_tracks_found)
	{
		// It's a matching set of pindexes and libentries.

		for(int i=0; i<lritem_vec.m_num_tracks_found; ++i)
		{
			if (!lritem_vec.m_original_pindexes[i].isValid())
			{
				qWarning() << "Invalid persistent index, ignoring update";
				return;
			}

			// None of the returned entries should be null.
			Q_ASSERT(lritem_vec.m_new_libentries[i] != nullptr);

			qDebug() << "Replacing entry"; // << item->getUrl();
			// item is a single song which has its metadata populated.
			// Reconstruct the QModelIndex we sent out.
			auto initial_row_index = QModelIndex(lritem_vec.m_original_pindexes[i]);
			auto row = initial_row_index.row();
			qDebug() << QString("incoming single item, row %1").arg(row);
			// Metadata's been populated.
			m_current_libmodel->setData(initial_row_index, QVariant::fromValue(lritem_vec.m_new_libentries[i]));
		}
	}
	else
	{
		// Not sure what we got.
		qCritical() << "pindexes/libentries/num_new_entries:" << lritem_vec.m_original_pindexes.size()
															  << lritem_vec.m_new_libentries.size();
															  // lritem_vec.m_new_libentries;
		Q_ASSERT_X(0, "Scanning", "Not sure what we got");
	}
}

void LibraryRescanner::ExpRunXQuery1(const QString& database_filename, const QString& filename)
{
	/// @todo MORE EXPERIMENTS, Linking through QIODevice / Temp file.
{ // For stopwatches.
	Stopwatch sw_par("Parallel XQuery test");

	auto f0 = ExtAsync::run_in_qthread_with_event_loop([&](ExtFuture<Unit>) {

		Stopwatch sw("Parallel XQuery test: Subtest 1: Through temp file");

		// Open the database file.
		QFile database_file(QUrl::fromLocalFile(database_filename).toLocalFile());
		bool status = database_file.open(QFile::ReadOnly | QFile::Text);
		throwif(!status /*"########## COULDN'T OPEN FILE"*/);
		if(!status)
		{
			qCro() << "########## COULDN'T OPEN FILE:" << filename;
		}

		// The tempfile we'll use as a pipe.
		QTemporaryFile tempfile;
		// QTemporaryFile::open() is always RW.
		throwif<SerializationException>(!tempfile.open(), "Couldn't open temp file");
		qDb() << "TEMPFILE NAME:" << tempfile.fileName();

		// Open the terminal output file.
		QFile output_file(QDir::homePath() + "/DeleteMeThroughTempFile.xspf");
		throwif<SerializationException>(!output_file.open(QIODevice::WriteOnly), "Couldn't open output file");

		// Here we'll manually prepare the two queries.
		QXmlQuery first_xquery, second_xquery;

		// Open the file containing the XQuery (could be in our resources).
		auto xquery_qurl = QUrl::fromLocalFile(":/xquery_files/database_filter_by_href_regex.xq");
		QFile xquery_file(xquery_qurl.toLocalFile());
		status = xquery_file.open(QIODevice::ReadOnly);
		if(!status)
		{
			throw SerializationException("Couldn't open xquery source file");
		}
		// Read in the XQuery as a QString.
		const QString query_string(QString::fromLatin1(xquery_file.readAll()));

		first_xquery.bindVariable("input_file_path", &database_file);
//					first_xquery.bindVariable("output_file_path", &tempfile);
		first_xquery.bindVariable("extension_regex", QVariant(R"((.*\.(flac|mp3)$))"));
		second_xquery.bindVariable("input_file_path", &tempfile);
//					second_xquery.bindVariable("output_file_path", &output_file);
		second_xquery.bindVariable("extension_regex", QVariant(R"((.*\.mp3$))"));

		// Set the xqueries.
		// @note This is correct, var binding should be before the setQuery() call.
		first_xquery.setQuery(query_string);
		second_xquery.setQuery(query_string);

		status = run_xquery(first_xquery, &database_file, &tempfile);
		throwif(!status);
		tempfile.reset();
		status = run_xquery(second_xquery, &tempfile, &output_file);
		throwif(!status);

	});

	// Run a parallel XQuery.
	auto f0_2 = ExtAsync::run_in_qthread_with_event_loop([&](ExtFuture<Unit>) {

		Stopwatch sw("Parallel XQuery test: Subtest 2: String literal xquery");

		// Open the database file.
		QFile database_file(QUrl::fromLocalFile(database_filename).toLocalFile());
		bool status = database_file.open(QFile::ReadOnly | QFile::Text);
		throwif(!status /*"########## COULDN'T OPEN FILE"*/);
		if(!status)
		{
			qCro() << "########## COULDN'T OPEN FILE:" << filename;
		}

		// Open the terminal output file.
		QFile output_file(QDir::homePath() + "/DeleteMeSecondThread.xspf");
		throwif<SerializationException>(!output_file.open(QIODevice::WriteOnly), "Couldn't open output file");

		// Here we'll manually prepare the two queries.
		QXmlQuery first_xquery;

//		// Open the file containing the XQuery (could be in our resources).
//		auto xquery_qurl = QUrl::fromLocalFile(":/xquery_files/database_filter_by_href_regex.xq");
//		QFile xquery_file(xquery_qurl.toLocalFile());
//		status = xquery_file.open(QIODevice::ReadOnly);
//		if(!status)
//		{
//			throw SerializationException("Couldn't open xquery source file");
//		}
		// Try hardcoded XQuery text.
		const QString query_string(QString::fromLatin1(R"xq(
(: http://www.w3.org/2005/xpath-functions :)
(: The XSPF namespace. :)
declare default element namespace "http://xspf.org/ns/0/";
declare namespace functx = "http://www.functx.com";

(: Path to the AMLM database, will be passed in as a bound variable. :)
declare variable $input_file_path as xs:anyURI external;


(: Local copy function.  Returns a deep copy of $n and all sub-nodes.  Use as a basis for whole-doc filters. :)
(: declare function local:copy($n as node()) as node() {
    typeswitch($n)
        case $e as element()
            return
                element {fn:name($e)}
                    {$e/@*,
                     for $c in $e/(* | text())
                         return local:copy($c) }
      default return $n
}; :)

(: let $x := fn:doc($input_file_path)//href
return
 <output>
    <original>{$x}</original>
    <fullcopy> {local:copy($x)}</fullcopy>
 </output>
:)

(: From http://www.xqueryfunctions.com/xq/functx_replace-element-values.html. :)
declare function functx:replace-element-values
  ( $elements as element()* ,
    $values as xs:anyAtomicType* )  as element()* {

   for $element at $seq in $elements
   return element { node-name($element)}
             { $element/@*,
               $values[$seq] }
};

for $x in fn:doc($input_file_path)//href
return functx:replace-element-values($x,concat($x,'.APPENDED'))

)xq"));

		first_xquery.bindVariable("input_file_path", &database_file);

		// Set the xqueries.
		// @note This is correct, var binding should be before the setQuery() call.
		first_xquery.setQuery(query_string);
		throwif<SerializationException>(!first_xquery.isValid(), "Bad XQuery");

		status = run_xquery(first_xquery, &database_file, &output_file);
		throwif<SerializationException>(!status, "XQuery failed");
	});

	/// @todo wait_for_all().
	f0.wait();
	f0_2.wait();
}
	qDb() << "PARALLEL XQUERY TEST COMPLETE";
}
/// END @todo MORE EXERIMENTS, QIODevice.

#if 0
void LibraryRescanner::onRescanFinished()
{
    auto elapsed = m_timer.elapsed();
	qDebug() << "Async Rescan reports finished.";
	qInfo() << "Directory scan took" << m_last_elapsed_time_dirscan << "ms";
	qInfo() << "Metadata rescan took" << elapsed << "ms";
	// Send out progress text.
	Q_EMIT progressChanged(0, 0, 0, "Idle");
}

void LibraryRescanner::onDirTravFinished()
{
	qDb() << "Async Dir Trav reports fisished.";

	// Send out progress text.
	Q_EMIT progressChanged(0, 0, 0, "Idle");

	/// @todo Should be a lambda.
	///m_current_libmodel->onIncomingFilenamesComplete();
	m_current_libmodel->startRescan();
}

#endif
