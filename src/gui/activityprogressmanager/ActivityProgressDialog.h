#ifndef EXPDIALOG_H
#define EXPDIALOG_H

#include <QDialog>
#include <QAbstractItemView>
#include <QStyledItemDelegate>

#include <KIO/Job>
#include <KWidgetJobTracker>
#include <KWidgetItemDelegate>


#include "utils/DebugHelpers.h"

//class KWidgetJobTracker;
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
    ~ActivityProgressDialog();

    void TrackJob(KJob *job);

protected Q_SLOTS:
    void kjobIncoming(KIO::Job *pJob, const KIO::UDSEntryList &pEntryList);
    void kjobCompleted(KJob *pJob);

private:
    Ui::ActivityProgressDialog *ui;

//    WidgetItemDelegate* m_delegate;

    KWidgetJobTracker *m_JobTracker;
};

Q_DECLARE_METATYPE(QModelIndex)

/**
 * @brief The WidgetItemDelegate class
 * https://api.kde.org/frameworks/kitemviews/html/classKWidgetItemDelegate.html
 */
class WidgetItemDelegate : public KWidgetItemDelegate
{
    Q_OBJECT

public:
    WidgetItemDelegate(QAbstractItemView *item_view, QObject* parent = nullptr) : KWidgetItemDelegate(item_view, parent)
    {
        m_styledDelegate = new QStyledItemDelegate(parent);
        connect(m_styledDelegate, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)),
                    this, SIGNAL(closeEditor(QWidget*,QAbstractItemDelegate::EndEditHint)));
        connect(m_styledDelegate, SIGNAL(commitData(QWidget*)),
                    this, SIGNAL(commitData(QWidget*)));
        connect(m_styledDelegate, SIGNAL(sizeHintChanged(QModelIndex)),
                this, SIGNAL(sizeHintChanged(QModelIndex)));
    }

    ~WidgetItemDelegate() override { qDb() << "DESTRUCTED"; }

protected:

//    inline static QWidget* toWidget(const QModelIndex &index) { return index.data(Qt::UserRole).value<QWidget*>(); }
    inline static KWidgetJobTracker* toKJobTracker(const QModelIndex &index) { return index.data(Qt::UserRole).value<KWidgetJobTracker*>(); }
    inline static KJob* toKJob(const QModelIndex &index) { return index.data(Qt::UserRole+1).value<KJob*>(); }
    inline static QWidget* toWidget(const QModelIndex &index)
    {
        QWidget* widget = nullptr;

        auto tracker = toKJobTracker(index);
        auto job = toKJob(index);
        if(tracker && job)
        {
            widget = tracker->widget(job);
            if(!widget)
            {
                qWr() << "BAD WIDGET:" << widget;
            }
        }
        else
        {
            qWr() << "BAD TRACKER/JOB:" << tracker << job;
        }

        return widget;
    }

    QList<QWidget*> createItemWidgets(const QModelIndex &index) const override
    {
        // Per https://api.kde.org/4.x-api/kdelibs-apidocs/kdeui/html/classKWidgetItemDelegate.html#ae4adcebdc0c6e94d280565d10822f298:
        // "If you want to know the index for which you are creating widgets, it is available as a QModelIndex Q_PROPERTY called "goya:creatingWidgetForIndex".
        // Ensure to add Q_DECLARE_METATYPE(QModelIndex) before your method definition to tell QVariant about QModelIndex."

//        dump_properties(&index);

        QList<QWidget*> retval;

        if(!index.isValid())
        {
            qWr() << "INVALID INDEX";
        }
        else
        {
            qDb() << "INDEX:" << index;
            qDb() << "INDEX R/C/tracker/kjob:" << index.row() << index.column() << toKJobTracker(index) << toKJob(index);

            auto tracker = toKJobTracker(index);
            auto job = toKJob(index);

            if(!tracker || !job)
            {
                qDb() << "NO tracker or JOB";
            }
            else
            {
                qDb() << "tracker/job:" << tracker << job;
                // Create the new progress widget.
                QWidget* progress_widget = tracker->widget(job);
                progress_widget->setParent(this->itemView());

                retval << progress_widget;
            }
        }

        return retval;
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        m_styledDelegate->paint(painter, option, index);
    }

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        // get default size hint
        QSize hint = m_styledDelegate->sizeHint(option, index);
        qDb() << "DEFAULT SIZE HINT:" << hint;

        if(!index.isValid())
        {
            qWr() << "INVALID";
            return hint;
        }

        auto widget = toWidget(index); //index.data(Qt::UserRole).value<QWidget*>();
        if(!widget)
        {
            qDb() << "NO WIDGET";
        }
        else
        {
            hint = widget->sizeHint();
        }

        qDb() << "SIZE HINT:" << hint;

        return hint;
    }

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        m_styledDelegate->updateEditorGeometry(editor, option, index);
    }


    void updateItemWidgets(const QList<QWidget*> widgets, const QStyleOptionViewItem &option,
                           const QPersistentModelIndex &index) const override
    {
        qDb() << "UPDATE";
        qDb() << "PINDEX:" << index;


        QWidget* widget = nullptr;
        if(widgets.size() > 0)
        {
            widget = widgets[0];
        }

        if(!widget)
        {
            qDb() << "NO WIDGET";
            return;
        }

        // Resize
        auto new_size = option.rect; //widget->sizeHint();
        qDb() << "RESIZING TO NEW SIZE:" << new_size;
//        widget->resize(new_size);
        widget->setGeometry(option.rect);
        widget->show();

    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        return m_styledDelegate->createEditor(parent, option, index);
    }

    bool editorEvent(QEvent* event, QAbstractItemModel* model,
                                              const QStyleOptionViewItem& option, const QModelIndex& index)
    {
        return static_cast<QAbstractItemDelegate*>(m_styledDelegate)->editorEvent(event, model, option, index);
    }

private:
    QStyledItemDelegate* m_styledDelegate { nullptr };

};

//Q_DECLARE_METATYPE(WidgetItemDelegate)

#endif // EXPDIALOG_H
