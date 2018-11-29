#ifndef EXPERIMENTALKDEVIEW1_H
#define EXPERIMENTALKDEVIEW1_H

#include <QWidget>

namespace Ui {
class ExperimentalKDEView1;
}

class ExperimentalKDEView1 : public QWidget
{
	Q_OBJECT

public:
	explicit ExperimentalKDEView1(QWidget *parent = nullptr);
	~ExperimentalKDEView1();

private:
	Ui::ExperimentalKDEView1 *ui;
};

#endif // EXPERIMENTALKDEVIEW1_H
