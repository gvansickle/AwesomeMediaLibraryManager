/*
 * Copyright 2017, 2018, 2019, 2025 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

#ifndef LIBRARYMODEL_H
#define LIBRARYMODEL_H

/** @file LibraryModel.h */

// Std C++
#include <vector>
#include <memory>

// Qt
#include <QAbstractItemModel>
#include <QFuture>
#include <QSaveFile>
#include <QUrl>
#include <QVector>
class QFileDevice;

// Ours
#include <logic/serialization/ISerializable.h>
#include <logic/dbmodels/CollectionDatabaseModel.h>
#include <concurrency/ThreadsafeMap.h>
#include <ColumnSpec.h>
#include "logic/Library.h"
#include "logic/LibraryRescanner.h" ///< For MetadataReturnVal
#include "logic/LibraryEntry.h"
#include <logic/jobs/LibraryEntryLoaderJob.h>
#include "logic/LibraryRescannerMapItem.h"
#include <utils/RegisterQtMetatypes.h> ///< For common metatype declarations of C++ std types.


class LibraryRescanner;

using VecOfUrls = QVector<QUrl>;
//using VecOfLEs = std::vector<std::shared_ptr<LibraryEntry> >;
using VecOfPMIs = QVector<QPersistentModelIndex>;
struct LibraryRescannerMapItem;

Q_DECLARE_METATYPE(VecOfUrls);
//Q_DECLARE_METATYPE(std::vector<std::shared_ptr<LibraryEntry>>);
//Q_DECLARE_METATYPE(VecOfLEs);
Q_DECLARE_METATYPE(VecOfPMIs);


/**
 * The LibraryModel class.
 */
class LibraryModel : public QAbstractItemModel, public virtual ISerializable
{
    Q_OBJECT
	Q_INTERFACES(ISerializable);

	using BASE_CLASS = QAbstractItemModel;

Q_SIGNALS:
	/// Signal to ourself to start an asynchronous directory traversal.
    void startFileScanSignal(QUrl url);

    /// Signal-to-self for async loading of metadata for a single LibraryEntry.
//    void SIGNAL_selfSendReadyResults(MetadataReturnVal results) const;
	void SIGNAL_selfSendReadyResults(LibraryEntryLoaderJobResult results);

public:
	explicit LibraryModel(QObject *parent = nullptr);
    ~LibraryModel() override;
	M_GH_POLYMORPHIC_SUPPRESS_COPYING_C67(LibraryModel)

//    CollectionDatabaseModel *m_cdb_model;

	/**
	 * Open a new LibraryModel on the specified QUrl.
	 */
	static QPointer<LibraryModel> openFile(QUrl open_url, QObject* parent);

	/// @name Basic functionality.
	/// @{
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
	QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

	QSize span(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    /// Returns the data stored under the given role for the item referred to by the index.
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	/// Fills the @a roleDataSpan with the requested data for the given @a index.
	void multiData(const QModelIndex& index, QModelRoleDataSpan roleDataSpan) const override;

    // Returns a map with values for all predefined roles in the model for the item at the given index.
	QMap<int, QVariant> itemData(const QModelIndex &index) const override;

	QHash<int, QByteArray> roleNames() const override;

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	/// @}

    /// Everything is a top-level item.
	bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

	///
    /// Overrides to enable model editing.
	///

	/// Override this in derived classes to return a newly-allocated, default-constructed instance
	/// of an entry derived from LibraryEntry.  Used by insertRows().
	virtual std::shared_ptr<LibraryEntry> createDefaultConstructedEntry() const;

	virtual std::shared_ptr<LibraryEntry> getItem(const QModelIndex& index) const;

	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;


	bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    /// Additional interfaces beyond the QAbstractItemModel interface.

	void appendRow(std::shared_ptr<LibraryEntry> libentry);
	void appendRows(std::vector<std::shared_ptr<LibraryEntry> > libentries);

    int getColFromSection(SectionID section_id) const;
    SectionID getSectionFromCol(int col) const;

	///
	/// Functions for accessing underlying library.
	///

    QUrl getLibRootDir() const;
    QString getLibraryName() const;
	qint64 getLibraryNumEntries() const;

	struct NumDirsFiles
	{
		qint64 dirs;
		qint64 files;
	};
	NumDirsFiles getLibraryNumDirsFiles() const;

	virtual void setLibraryRootUrl(const QUrl& url);

	virtual void stopAllBackgroundThreads();
	virtual void close(bool delete_cache = false);

	///
	/// Serialization
	///

	QVariant toVariant() const override;
	void fromVariant(const QVariant& variant) override;

	///
	/// Drag and drop support.
	///

    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList &indexes) const override;

	// This is FBO clazy.  Without this, it'll flag the slot using this below with:
	// "warning: slot arguments need to be fully-qualified [...] [-Wclazy-fully-qualified-moc-types]"
	using StdVecOfSharedPtrToLibEntry = std::vector<std::shared_ptr<LibraryEntry>>;

public Q_SLOTS:
	/// All this is for reading the metadata from a non-GUI thread.
    void SLOT_processReadyResults(MetadataReturnVal lritem_vec);
    void SLOT_processReadyResults(LibraryEntryLoaderJobResult loader_results);
    void SLOT_onIncomingPopulateRowWithItems_Single(QPersistentModelIndex pindex, std::shared_ptr<LibraryEntry> item);
	void SLOT_onIncomingPopulateRowWithItems_Multiple(QPersistentModelIndex pindex, LibraryModel::StdVecOfSharedPtrToLibEntry items);

	/**
	 * 
	 * @return  A QList of all the items in the LibraryModel which need to be populated with metadata.
	 */
	virtual QList<VecLibRescannerMapItems> getLibRescanItems();

	/// Let's try something different.
	virtual void startRescan();

	virtual void cancelRescan();

    void SLOT_onIncomingFilename(QString filename);

protected:

	virtual void createCacheFile(QUrl root_url);

	virtual void deleteCache();

	virtual void connectSignals();
	virtual void disconnectIncomingSignals();

	void finishIncoming();

	virtual QString getEntryStatusToolTip(LibraryEntry* item) const;

	std::vector<ColumnSpec> m_columnSpecs;

	/// The underlying data store.
    Library m_library;

	LibraryRescanner* m_rescanner {nullptr};

private:

	/// The directory where we'll put the LibraryModel's cache file.
	QUrl m_cachedir;

	/// The actual QSaveFile which will serve as the write half of the cache file.
	mutable QSaveFile m_lib_cache_file;

	/// Icons for various entry states.
	QVariant m_IconError, m_IconOk, m_IconUnknown;

	/// @name Data structures for managing the data loading process.
//    mutable std::map<std::shared_ptr<LibraryEntry>, LibraryEntryLoaderJobPtr> m_pending_async_item_loads;
	mutable ThreadsafeMap<QPersistentModelIndex, bool> m_pending_async_item_loads;
};

Q_DECLARE_METATYPE(LibraryModel);
Q_DECLARE_METATYPE(LibraryModel*);

#endif // LIBRARYMODEL_H
