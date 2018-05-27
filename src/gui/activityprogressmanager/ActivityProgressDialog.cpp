#include "ActivityProgressDialog.h"
#include "ui_ActivityProgressDialog.h"

#include <KWidgetJobTracker>
//#include <QStackedLayout>

#include <utils/DebugHelpers.h>





ActivityProgressDialog::ActivityProgressDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ActivityProgressDialog), m_JobTracker(nullptr)
{
    ui->setupUi(this);

    // QListWidget is-a QListView.
//    auto list_widget = ui->m_listWidget;
//    auto list_widget = ui->m_blank_widget;
//    auto new_layout = new QStackedLayout(list_widget);
//    new_layout->setStackingMode(QStackedLayout::StackAll);
//    list_widget->setLayout(new_layout);
//    qobject_cast<QStackedLayout*>(list_widget->layout())->setStackingMode(QStackedLayout::StackAll);

    // views don't take ownership of delegates.
//    m_delegate = new WidgetItemDelegate(list_widget, list_widget);

//    auto old_delegate = list_widget->itemDelegate();
//    list_widget->setItemDelegate(m_delegate);
//    old_delegate->deleteLater();
}

ActivityProgressDialog::~ActivityProgressDialog()
{
//    delete m_delegate;
    delete ui;
}

void ActivityProgressDialog::TrackJob(KJob *job)
{
    if(m_JobTracker == nullptr)
    {
        m_JobTracker = new KWidgetJobTracker(ui->m_blank_widget);
    }

    // Register the KJob.
    m_JobTracker->registerJob(job);


    // Get/Create the widget associated with the KJob.
    QWidget* progress_widget = m_JobTracker->widget(job);
    ui->m_blank_widget->layout()->addWidget(progress_widget);

//    ui->m_ProgressLayout->insertWidget(0, progress_widget);
//    auto last_item_idx = ui->m_listWidget->count();
//    auto list_widget_item = new QListWidgetItem();
//    list_widget_item->setData(Qt::UserRole, QVariant::fromValue(m_JobTracker));
//    list_widget_item->setData(Qt::UserRole+1, QVariant::fromValue(job));

//    ui->m_listWidget->addItem(list_widget_item);
//    ui->listWidget->setItemWidget(list_widget_item, progress_widget);

//    progress_widget->show();

//    connect(job, SIGNAL(entries(KIO::Job*, KIO::UDSEntryList)), SLOT(kjobIncoming(KIO::Job*, KIO::UDSEntryList)));
    connect(job, SIGNAL(result(KJob*)), SLOT(kjobCompleted(KJob*)));
//    connect(job, &KJob::finished, [=](KJob* job) mutable {
//        // Our widget is getting deleted out from under us.  Remove this list item.
//        delete list_widget_item;
//    });

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
