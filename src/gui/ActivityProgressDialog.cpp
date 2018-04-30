#include <src/gui/ActivityProgressDialog.h>
#include "ui_ActivityProgressDialog.h"

#include <KWidgetJobTracker>

#include <utils/DebugHelpers.h>





ActivityProgressDialog::ActivityProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ActivityProgressDialog), m_JobTracker(nullptr)
{
    ui->setupUi(this);

    // QListWidget is-a QListView.
    auto list_widget = ui->m_listWidget;

    // views don't take ownership of delegates.
    m_delegate = new WidgetItemDelegate(ui->m_listWidget, ui->m_listWidget);

    auto old_delegate = ui->m_listWidget->itemDelegate();
    ui->m_listWidget->setItemDelegate(m_delegate);
    old_delegate->deleteLater();
}

ActivityProgressDialog::~ActivityProgressDialog()
{
    delete m_delegate;
    delete ui;
}

void ActivityProgressDialog::TrackJob(KJob *job)
{
    if(m_JobTracker == nullptr)
    {
        m_JobTracker = new KWidgetJobTracker(this);
    }

    // Register the KJob.
    m_JobTracker->registerJob(job);
    // Get the widget associated with the KJob.
    QWidget* progress_widget = m_JobTracker->widget(job);

//    ui->m_ProgressLayout->insertWidget(0, progress_widget);
//    auto last_item_idx = ui->m_listWidget->count();
    auto list_widget_item = new QListWidgetItem();
    list_widget_item->setData(Qt::UserRole, QVariant::fromValue(progress_widget));
    ui->m_listWidget->addItem(list_widget_item);
//    ui->listWidget->setItemWidget(list_widget_item, progress_widget);

    progress_widget->show();

    connect(job, SIGNAL(entries(KIO::Job*, KIO::UDSEntryList)), SLOT(kjobIncoming(KIO::Job*, KIO::UDSEntryList)));
    connect(job, SIGNAL(result(KJob*)), SLOT(kjobCompleted(KJob*)));
    connect(job, &KJob::finished, [=](KJob* job) mutable {
        // Our widget is getting deleted out from under us.  Remove this list item.
        delete list_widget_item;
    });

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
