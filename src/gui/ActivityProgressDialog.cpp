#include <src/gui/ActivityProgressDialog.h>
#include "ui_ActivityProgressDialog.h"

#include <KWidgetJobTracker>

#include <utils/DebugHelpers.h>

ActivityProgressDialog::ActivityProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ActivityProgressDialog), m_JobTracker(nullptr)
{
    ui->setupUi(this);
}

ActivityProgressDialog::~ActivityProgressDialog()
{
    delete ui;
}

void ActivityProgressDialog::TrackJob(KJob *job)
{
    if(m_JobTracker == nullptr)
    {
        m_JobTracker = new KWidgetJobTracker(this);
    }
    m_JobTracker->registerJob(job);
    QWidget* progress_widget = m_JobTracker->widget(job);
    ui->m_ProgressLayout->insertWidget(0, progress_widget);
    progress_widget->show();

    connect(job, SIGNAL(entries(KIO::Job*, KIO::UDSEntryList)), SLOT(kjobIncoming(KIO::Job*, KIO::UDSEntryList)));
    connect(job, SIGNAL(result(KJob*)), SLOT(kjobCompleted(KJob*)));

    //    job->start();
}

void ActivityProgressDialog::kjobIncoming(KIO::Job *pJob, const KIO::UDSEntryList &pEntryList)
{
    qInfo() << pEntryList;
}

void ActivityProgressDialog::kjobCompleted(KJob *pJob)
{
    if(pJob->error() != 0)
    {
        qWarning() << "ERROR";
    }
    else
    {
        qInfo() << "COMPLETE";
    }
}
