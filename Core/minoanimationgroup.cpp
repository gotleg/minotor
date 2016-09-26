/*
 * Copyright 2012, 2013 Gauthier Legrand
 * Copyright 2012, 2013 Romuald Conty
 * 
 * This file is part of Minotor.
 * 
 * Minotor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Minotor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Minotor.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "minoanimationgroup.h"

#include <QDebug>

#include "minoprogram.h"
#include "minoinstrumentedanimation.h"
#include "minopersistentobjectfactory.h"
#include "midicontrollableparameter.h"

#include "minotor.h"

MinoAnimationGroup::MinoAnimationGroup(QObject *parent) :
    MinoPersistentObject(parent),
    _enabled(false),
    _alive(false),
    _program(NULL)
{
    Q_ASSERT(parent);
    Q_ASSERT(qobject_cast<MinoProgram*>(parent));
}

int MinoAnimationGroup::id ()
{
    return _program->animationGroups().indexOf(this);
}

void MinoAnimationGroup::addAnimation(MinoAnimation *animation)
{
    insertAnimation(animation);
}

void MinoAnimationGroup::setProgram(MinoProgram *program)
{
    if(_program != program)
    {
        setParent(program);
        if(program) {
            _itemGroup.setParentItem(program->itemGroup());
            _itemGroup.setGroup(program->itemGroup());
            _itemGroup.setPos(0,0);
            _itemGroup.setVisible(true);
        } else {
            _itemGroup.setVisible(false);
        }
        _program = program;
    }
}

void MinoAnimationGroup::setScreenshot(const QPixmap &screenshot)
{
    _screenshot = screenshot;
    emit screenshotUpdated();
}

MinoAnimationGroup::~MinoAnimationGroup()
{
    foreach (MinoAnimation *animation, _animations)
    {
        delete animation;
    }
}

MinoAnimation* MinoAnimationGroup::addAnimation(const QString &animationClassName, int index)
{
    MinoAnimation *animation = qobject_cast<MinoAnimation*>(MinoPersistentObjectFactory::createObject(animationClassName.toLatin1(), this));
    if(animation)
    {
        MidiControllableParameter *mcpHue = animation->findChild<MidiControllableParameter*>("hue");
        if (mcpHue)
        {
            mcpHue->setPreferred();
        }
        MidiControllableParameter *mcpBeatFactor = animation->findChild<MidiControllableParameter*>("loop-size");
        if (mcpBeatFactor)
        {
            mcpBeatFactor->setPreferred();
        }
        insertAnimation(animation, index);
    }
    return animation;
}

void MinoAnimationGroup::insertAnimation(MinoAnimation *animation, int index)
{
//    qDebug() << Q_FUNC_INFO
//             << "animation:" << animation
//             << "index:" << index;
    // Add animation to program's list
    if(index<0)
        index = _animations.size();
    _animations.insert(index, animation);

    // Will remove animation from list when destroyed
    connect(animation, SIGNAL(destroyed(QObject*)), this, SLOT(destroyAnimation(QObject*)));

    MinoInstrumentedAnimation *mia = qobject_cast<MinoInstrumentedAnimation*>(animation);
    if(mia)
    {
        connect(Minotor::minotor()->midi(), SIGNAL(noteChanged(int,quint8,quint8,bool,quint8)), mia, SLOT(handleNoteChange(int,quint8,quint8,bool,quint8)));
    }

    // Add to program QGraphicsItemGroup to ease group manipulation (ie. change position, brightness, etc.)
    _itemGroup.addToGroup(animation->graphicItem());

    // Re-parent animation to our itemGroup
    animation->graphicItem()->setVisible(_enabled);
    animation->graphicItem()->setParentItem(&_itemGroup);
    animation->graphicItem()->setGroup(&_itemGroup);
    animation->graphicItem()->setPos(0,0);

    reorderAnimations();
    animation->setGroup(this);
    animation->setEnabled(_enabled);
    emit animationAdded(animation);
}
void MinoAnimationGroup::reorderAnimations()
{
    // Reorder Z values
    for(int z=0; z<_animations.count(); z++)
    {
        MinoAnimation *ma = _animations.at(z);
        ma->setParent(NULL);
        ma->setParent(this);
        ma->graphicItem()->setZValue(z);
    }
}

void MinoAnimationGroup::moveAnimation(int srcIndex, int destIndex, MinoAnimationGroup *destGroup)
{
    qDebug() << Q_FUNC_INFO
             << "this:" << this
             << "destIndex:" << destIndex
             << "destGroup:" << destGroup;

    if(destGroup == NULL)
        destGroup = this;

    //Position -1 is used to place the item at the end of the list
    if (destIndex == -1)
    {
        destIndex = destGroup->animations().count()-1;
    }

    if((this==destGroup) && (srcIndex==destIndex))
    {
        // Nothing to do: destination is the same as source
    }
    else if(this==destGroup)
    {
        // Destination group is our group
        _animations.move(srcIndex, destIndex);
        reorderAnimations();
        emit animationMoved(_animations.at(destIndex));
    }
    else
    {
        // Destination group is not this group
        // Let's take animation from this group
        MinoAnimation *animation = takeAnimationAt(srcIndex);
        // Prevent group from emitting animationAdded signal
        destGroup->blockSignals(true);
        destGroup->insertAnimation(animation, destIndex);
        destGroup->blockSignals(false);
        animation->setEnabled(destGroup->enabled());
        emit animationMoved(animation);
    }
}

MinoAnimation* MinoAnimationGroup::takeAnimationAt(int index)
{
    MinoAnimation *animation = _animations.takeAt(index);
    disconnect(animation);
    animation->setGroup(NULL);
    if (_animations.count() == 0)
    {
        this->deleteLater();
    }
    return animation;
}

void MinoAnimationGroup::setDelayedEnabled(const bool on)
{
    _program->registerAnimationGroupEnableChange(this, on);
}

void MinoAnimationGroup::setEnabled(const bool on)
{
    if(on != _enabled)
    {
        _setEnabled(on);
    }
}

void MinoAnimationGroup::_setEnabled(const bool on)
{
    _enabled = on;
    foreach(MinoAnimation *animation, _animations)
    {
        animation->setEnabled(on);
    }

    // Show _itemGroup (on=true) but do not hide it (on=false):
    //     it will be shuted-down when all animations stopped running..
    if(on)
    {
        setAlive(true);
    }
    emit enabledChanged(on);
}

void MinoAnimationGroup::destroyAnimation(QObject *animation)
{
    _animations.removeAt(_animations.indexOf(static_cast<MinoAnimation*>(animation)));
    if (_animations.count() == 0)
    {
        this->deleteLater();
    }
}

void MinoAnimationGroup::animate(const unsigned int uppqn, const unsigned int gppqn, const unsigned int ppqn, const unsigned int qn)
{
    bool alive = false;
    foreach(MinoAnimation *ma, _animations)
    {
        if(ma->isAlive())
        {
            ma->animate(uppqn, gppqn, ppqn, qn);
            alive = true;
        }
    }

    setAlive(alive);
}

void MinoAnimationGroup::createItem()
{
    bool alive = _alive;
    foreach(MinoAnimation *ma, _animations)
    {
        MinoInstrumentedAnimation *mia = qobject_cast<MinoInstrumentedAnimation*>(ma);
        if(mia)
        {
            mia->createItem();
            if(mia->isAlive())
            {
                alive = true;
            }
        }
    }
    setAlive(alive);
}

void MinoAnimationGroup::setAlive(const bool on)
{
    if(_alive != on)
    {
        _alive = on;
        _itemGroup.setVisible(on);
    }
}
