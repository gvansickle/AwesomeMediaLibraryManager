#ifndef EXPTREEVIEW_H
#define EXPTREEVIEW_H

#include <QWidget>

namespace Ui {
class ExpTreeView;
}

class ExpTreeView : public QWidget
{
	Q_OBJECT

public:
	explicit ExpTreeView(QWidget *parent = nullptr);
	~ExpTreeView();

private:
	Ui::ExpTreeView *ui;
};

#endif // EXPTREEVIEW_H
