#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

// Qt5
#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>

// Ours
#include <logic/dbmodels/ScanResultsTableModel.h>

namespace Ui {
class CollectionView;
}

class QTableView;
class AbstractTreeModel;

class CollectionView : public QWidget
{
    Q_OBJECT

public:
    explicit CollectionView(QWidget *parent = nullptr);
    ~CollectionView() override;

    void setMainModel(QSqlRelationalTableModel* model);

	void setMainModel2(ScanResultsTableModel* model);

	void setPane2Model(AbstractTreeModel* tmp);


private:
    Ui::CollectionView *ui;
};

#endif // COLLECTIONVIEW_H
