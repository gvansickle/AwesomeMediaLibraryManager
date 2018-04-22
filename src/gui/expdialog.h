#ifndef EXPDIALOG_H
#define EXPDIALOG_H

#include <QDialog>
#include <KIO/Job>

class KWidgetJobTracker;

class KJob;

namespace Ui {
class ExpDialog;
}

class ExpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExpDialog(QWidget *parent = 0);
    ~ExpDialog();

    void TrackJob(KJob *job);

protected Q_SLOTS:
    void kjobIncoming(KIO::Job *pJob, const KIO::UDSEntryList &pEntryList);
    void kjobCompleted(KJob *pJob);

private:
    Ui::ExpDialog *ui;

    KWidgetJobTracker *m_JobTracker;
};

#endif // EXPDIALOG_H
