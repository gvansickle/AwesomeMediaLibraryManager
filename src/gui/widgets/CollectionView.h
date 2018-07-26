#ifndef COLLECTIONVIEW_H
#define COLLECTIONVIEW_H

#include <QWidget>
#include <QSqlRelationalTableModel>
#include <QSqlRelationalDelegate>

namespace Ui {
class CollectionView;
}

class QTableView;

class CollectionView : public QWidget
{
    Q_OBJECT

public:
    explicit CollectionView(QWidget *parent = nullptr);
    ~CollectionView() override;

    void setMainModel(QSqlRelationalTableModel* model);

    QTableView* getTableView();

private:
    Ui::CollectionView *ui;
};

#endif // COLLECTIONVIEW_H
