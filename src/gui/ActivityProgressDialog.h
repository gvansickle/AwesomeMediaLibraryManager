#ifndef EXPDIALOG_H
#define EXPDIALOG_H

#include <QDialog>
#include <KIO/Job>

class KWidgetJobTracker;

class KJob;

namespace Ui {
class ActivityProgressDialog;
}

class ActivityProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActivityProgressDialog(QWidget *parent = 0);
    ~ActivityProgressDialog();

    void TrackJob(KJob *job);

protected Q_SLOTS:
    void kjobIncoming(KIO::Job *pJob, const KIO::UDSEntryList &pEntryList);
    void kjobCompleted(KJob *pJob);

private:
    Ui::ActivityProgressDialog *ui;

    KWidgetJobTracker *m_JobTracker;
};

#endif // EXPDIALOG_H
