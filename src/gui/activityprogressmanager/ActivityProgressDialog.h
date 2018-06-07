#ifndef EXPDIALOG_H
#define EXPDIALOG_H

#if 0

#include <QDialog>
#include <QAbstractItemView>
#include <QStyledItemDelegate>

#include <KIO/Job>
#include <KWidgetJobTracker>
#include <KWidgetItemDelegate>


#include "utils/DebugHelpers.h"

class KJob;

namespace Ui {
class ActivityProgressDialog;
}

class WidgetItemDelegate;

class ActivityProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ActivityProgressDialog(QWidget *parent = 0);
    ~ActivityProgressDialog() override;

    void TrackJob(KJob *job);

protected Q_SLOTS:
    void kjobIncoming(KIO::Job *pJob, const KIO::UDSEntryList &pEntryList);
    void kjobCompleted(KJob *pJob);

private:
    Ui::ActivityProgressDialog *ui;

    KWidgetJobTracker *m_JobTracker;
};

#endif

#endif // EXPDIALOG_H
