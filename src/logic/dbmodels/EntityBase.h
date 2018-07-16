#ifndef ENTITYBASE_H
#define ENTITYBASE_H

#include <QObject>

class EntityBase : public QObject
{
    Q_OBJECT
public:
    explicit EntityBase(QObject *parent = nullptr);

Q_SIGNALS:

public Q_SLOTS:
};

namespace AMLM
{

class Artist : public EntityBase {};
class Release : public EntityBase {};

}; // END namespace AMLM

#endif // ENTITYBASE_H
