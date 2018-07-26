#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>

namespace Ui {
class CollectionView;
}

class QTableView;
class TreeModel;

class CollectionView : public QWidget
{
    Q_OBJECT

public:
    explicit CollectionView(QWidget *parent = nullptr);
    ~CollectionView() override;

    void setMainModel(QSqlRelationalTableModel* model);

	void setPane2Model(TreeModel* tmp);


private:
    Ui::CollectionView *ui;
};

#endif // COLLECTIONVIEW_H
