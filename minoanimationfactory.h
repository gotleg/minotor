#ifndef MINOANIMATIONFACTORY_H
#define MINOANIMATIONFACTORY_H

#include <QObject>
#include <QHash>
#include <QString>

#include "minoanimation.h"
#include "minochannel.h"

class MinoAnimationFactory : public QObject
{
    Q_OBJECT
public:
    explicit MinoAnimationFactory(QObject *parent = 0);

    QList<MinoAnimationDescription> availableAnimations();
    MinoAnimation *instantiate(const QString name, MinoChannel *channel);

signals:
    
public slots:
    
private:
    QHash<QString, MinoAnimationDescription> _animations;
};

#endif // MINOANIMATIONFACTORY_H