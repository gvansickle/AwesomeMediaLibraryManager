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

#include "Library.h"
#include "LibraryEntry.h"
#include "LibraryRescanner.h"

class QFileDevice;

class LibraryPopulatorWorker;
class LibraryRescanner;
class ActivityProgressWidget;

typedef QVector<QUrl> VecOfUrls;
typedef QVector<std::shared_ptr<LibraryEntry>> VecOfLEs;
typedef QVector<QPersistentModelIndex>  VecOfPMIs;

Q_DECLARE_METATYPE(VecOfUrls);
Q_DECLARE_METATYPE(VecOfLEs);
Q_DECLARE_METATYPE(VecOfPMIs);

enum LibState
{
	Idle,
	ScanningForFiles,
	PopulatingMetadata
};

struct SectionID
{
	enum Enumerator
	{
		Status,
		Title,
		Artist,
		Album,
		Length,
		FileType,
		Filename,
		PLAYLIST_1
	};

	SectionID() = default;
	explicit SectionID(int val) : m_val(val) {}
	SectionID(SectionID::Enumerator e) { m_val = e; }

	virtual operator int() const { return m_val; }

	template<typename DerivedType>
	operator SectionID() { return DerivedType(); }

protected:
	int m_val;
};


struct ColumnSpec
{
	ColumnSpec(SectionID s, QString dn, QStringList ml, bool fit_col_width = false)
    {
        section_id = s;
        display_name = dn;
        metadata_list = ml;
		m_should_fit_column_width_to_contents = fit_col_width;
	}

    SectionID section_id;
    QString display_name;
    QStringList metadata_list;
	bool m_should_fit_column_width_to_contents {false};
	bool m_default_to_hidden {false};
};

class LibraryModel : public QAbstractItemModel
{
    Q_OBJECT

signals:
	/// Signal to ourself to start an asynchronous directory traversal.
    void startFileScanSignal(QUrl url);

    /// Status/Progress signal.
    void statusSignal(LibState, qint64, qint64);

public:
    explicit LibraryModel(QObject *parent = 0);
	virtual ~LibraryModel() override;

	/// @name Basic functionality.
	/// @{
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
	QModelIndex sibling(int row, int column, const QModelIndex &idx) const override;

	virtual QSize span(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

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

	virtual void connectProgressToActivityProgressWidget(ActivityProgressWidget* apw);
	virtual void stopAllBackgroundThreads();
	virtual void close(bool delete_cache = false);

	///
	/// Serialization
	///

	virtual void writeToJson(QJsonObject & json) const;
	virtual void readFromJson(const QJsonObject& jo);

	/// Static constructor for deserializing from JSON.
	static QSharedPointer<LibraryModel> constructFromJson(const QJsonObject & json, QObject* parent = Q_NULLPTR);

	virtual void serializeToFile(QFileDevice& file) const;
	virtual void deserializeFromFile(QFileDevice& file);

	///
	/// Drag and drop support.
	///

	virtual Qt::DropActions supportedDragActions() const override;
	virtual Qt::DropActions supportedDropActions() const override;
	virtual QStringList mimeTypes() const override;
	virtual QMimeData* mimeData(const QModelIndexList &indexes) const override;

public slots:
	/// All this is for reading the metadata from a non-GUI thread.
	void onIncomingPopulateRowWithItems_Single(QPersistentModelIndex pindex, LibraryEntry* item);
	void onIncomingPopulateRowWithItems_Multiple(QPersistentModelIndex pindex, VecOfLEs items);

	/// Let's try something different.
	virtual void startRescan();

	void onIncomingFilename(QString filename);

protected:
	virtual void onRowsInserted(QModelIndex parent, int first, int last) { (void)parent; (void)first; (void)last; }
	virtual void onRowsRemoved(QModelIndex parent, int first, int last) { (void)parent; (void)first; (void)last;}
	virtual void onSetData(QModelIndex index, QVariant value, int role = Qt::EditRole) { (void)index; (void)value; (void)role;}

	virtual void createCacheFile(QUrl root_url);

	virtual void deleteCache();

	virtual void connectSignals();
	virtual void disconnectIncomingSignals();

	void finishIncoming();

	virtual QString getEntryStatusToolTip(LibraryEntry* item) const;

	std::vector<ColumnSpec> m_columnSpecs;

    Library* m_library = nullptr;

	LibraryRescanner* m_rescanner = nullptr;

private:
	Q_DISABLE_COPY(LibraryModel)

	/// The directory where we'll put the LibraryModel's cache file.
	QUrl m_cachedir;

	/// The actual QSaveFile which will serve as the write half of the cache file.
	mutable QSaveFile m_lib_cache_file;

	qint64 m_first_possible_unpop_row;

	QVariant m_IconError, m_IconOk, m_IconUnknown;
};

Q_DECLARE_METATYPE(LibraryModel*)

#endif // LIBRARYMODEL_H
