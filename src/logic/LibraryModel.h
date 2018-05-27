/*
 * Copyright 2017 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

/** @file LibraryModel. */

#ifndef LIBRARYMODEL_H
#define LIBRARYMODEL_H

#include <vector>

#include <QAbstractItemModel>
#include <QFuture>
#include <QSaveFile>
#include <QUrl>
#include <QVector>

#include "ColumnSpec.h"
#include "Library.h"
#include "LibraryEntry.h"
#include "LibraryRescannerMapItem.h"

class QFileDevice;

class LibraryPopulatorWorker;
class LibraryRescanner;
class ActivityProgressWidget;

using VecOfUrls = QVector<QUrl>;
using VecOfLEs = QVector<std::shared_ptr<LibraryEntry> >;
using VecOfPMIs = QVector<QPersistentModelIndex>;
class LibraryRescannerMapItem;

Q_DECLARE_METATYPE(VecOfUrls);
Q_DECLARE_METATYPE(VecOfLEs);
Q_DECLARE_METATYPE(VecOfPMIs);


enum LibState
{
	Idle,
	ScanningForFiles,
	PopulatingMetadata
};

/**
 * The LibraryModel class.
 */
class LibraryModel : public QAbstractItemModel
{
    Q_OBJECT

	using BASE_CLASS = QAbstractItemModel;

Q_SIGNALS:
	/// Signal to ourself to start an asynchronous directory traversal.
    void startFileScanSignal(QUrl url);

    /// Status/Progress signal.
    void statusSignal(LibState, qint64, qint64);

public:
	explicit LibraryModel(QObject *parent = nullptr);
	virtual ~LibraryModel() override;

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

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

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

    virtual void setLibraryRootUrl(const QUrl& url);

//	virtual void connectProgressToActivityProgressWidget(ActivityProgressWidget* apw);
	virtual void stopAllBackgroundThreads();
	virtual void close(bool delete_cache = false);

	///
	/// Serialization
	///

	virtual void writeToJson(QJsonObject & json) const;
	virtual void readFromJson(const QJsonObject& jo);

	/// Static constructor for deserializing from JSON.
	static QPointer<LibraryModel> constructFromJson(const QJsonObject & json, QObject* parent = Q_NULLPTR);

	virtual void serializeToFile(QFileDevice& file) const;
	virtual void deserializeFromFile(QFileDevice& file);

	///
	/// Drag and drop support.
	///

	virtual Qt::DropActions supportedDragActions() const override;
	virtual Qt::DropActions supportedDropActions() const override;
	virtual QStringList mimeTypes() const override;
	virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;

public Q_SLOTS:
	/// All this is for reading the metadata from a non-GUI thread.
	void onIncomingPopulateRowWithItems_Single(QPersistentModelIndex pindex, LibraryEntry* item);
	void onIncomingPopulateRowWithItems_Multiple(QPersistentModelIndex pindex, VecOfLEs items);

    virtual QVector<VecLibRescannerMapItems> getLibRescanItems();

	/// Let's try something different.
	virtual void startRescan();

	virtual void cancelRescan();

	void onIncomingFilename(QString filename);

protected:
	/// @name Subclass model change handlers.
	/// Override these in subclasses if you need to do some extra work when the associated base class
	/// function is called, but you don't need to fully override the function.
	/// @{

	/**
	 * Called from insertRows() and appendRows() just after this base class has done its removal and just before endInsertRows().
	 */
	virtual void subclassesInsertRows(int first_row, int num_rows, const QModelIndex& parent = QModelIndex())
		{ (void)parent; (void)first_row; (void)num_rows; }

	/**
	 * Called from removeRows() just after beginRemoveRows() and before any removal by this base class.
	 */
	virtual void subclassesRemoveRows(int first_row, int num_rows, const QModelIndex& parent = QModelIndex())
		{ (void)parent; (void)first_row; (void)num_rows;}

	/**
	 * Called from setData() after this base class has made its changes and before dataChanged() is emitted.
	 */
	virtual void subclassesSetData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole)
		{ (void)index; (void)value; (void)role;}
	/// @}

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
	Q_DISABLE_COPY(LibraryModel)

	/// The directory where we'll put the LibraryModel's cache file.
	QUrl m_cachedir;

	/// The actual QSaveFile which will serve as the write half of the cache file.
	mutable QSaveFile m_lib_cache_file;

	/// Icons for various entry states.
	QVariant m_IconError, m_IconOk, m_IconUnknown;
};

Q_DECLARE_METATYPE(LibraryModel*)

#endif // LIBRARYMODEL_H
