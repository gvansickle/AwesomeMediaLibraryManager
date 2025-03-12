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
 * @file Core.h
 */
#ifndef SRC_CORE_H_
#define SRC_CORE_H_

// Std C++
#include <memory>
#include <deque>

// Qt5
#include <QObject>

// Ours.
#include <future/guideline_helpers.h>
#include <logic/models/ScanResultsTreeModel.h>
#include <models/ScanResultsTreeModel.h>
#include <logic/models/treemodel.h>

namespace AMLM
{

#if 0
class Instance;

  class Instance_Manager : public QObject {
	Q_OBJECT

	public:
	  Instance_Manager(QObject *parent = nullptr);

	  Instance *create_instance();
	public slots:
	  void do_create_instance();
	  void quit();

	  void open_urls_in_new_instances(const QList<QUrl> &urls);

	signals:
	  void first_open_requested(const QString &filename);

	private:
	  std::deque<Instance*> instances_;



  };
#endif

class Instance;

/**
 *
 */
class Core : public QObject
{
	Q_OBJECT

public:
    M_GH_DELETE_COPY_AND_MOVE(Core)
	~Core() override;

	/**
	 * Named constructor for the Core singleton.
	 * This can be expanded to take any necessary parameters.
	 */
	static void build();

	/**
     * Init the GUI part of the app and show the main window
     */
	void initGUI(/*const QUrl &Url*/);

	/** Returns a pointer to the singleton object. */
	static std::unique_ptr<Core>& self();

	/** Delete the global core instance */
	static void clean();

	/// @name Accessors for the singletons.
	/// @{

//	std::shared_ptr<AbstractTreeModel> getScanResultsTreeModel();
	std::shared_ptr<ScanResultsTreeModel> getScanResultsTreeModel();
	std::shared_ptr<ScanResultsTreeModel> swapScanResultsTreeModel(const std::shared_ptr<ScanResultsTreeModel>& new_model);
	/// Really don't like this here.
	std::initializer_list<ColumnSpec> getDefaultColumnSpecs();

	std::shared_ptr<TreeModel> getEditableTreeModel();

	/// @}

private:
	explicit Core();
	// Singleton ptr to us.
	static std::unique_ptr<Core> m_self;

	// Shared ptr to the scan results tree model.  Will be deleted in the destructor.
	std::shared_ptr<ScanResultsTreeModel/*AbstractTreeModel*/> m_srtm_instance;
//	std::shared_ptr</*ScanResultsTreeModel*/AbstractTreeModel> m_srtm_instance;

	std::shared_ptr<TreeModel> m_etm_instance;
	std::shared_ptr<AbstractTreeModel> m_atm_instance;

};

} /* namespace AMLM */

#endif /* SRC_CORE_H_ */
