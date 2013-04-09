#ifndef MINOANIMATIONPLASMA_H
#define MINOANIMATIONPLASMA_H

#include "minoanimation.h"

#include <QGraphicsRectItem>

#include "minopropertyeasingcurve.h"

class MinaPlasma : public MinoAnimation
{
    Q_OBJECT
public:
    explicit MinaPlasma(MinoAnimationGroup *group);
    ~MinaPlasma();
    void animate(const unsigned int uppqn, const unsigned int gppqn, const unsigned int ppqn, const unsigned int qn);

    static const MinoAnimationDescription getDescription() {
        return MinoAnimationDescription("plasma", "Plasma", QPixmap(":/images/plasma.png"), MinaPlasma::staticMetaObject.className());
    }
    const MinoAnimationDescription description() const { return getDescription(); }

    QGraphicsItem *graphicItem() { return &_itemGroup; }

private:
    QGraphicsItemGroup _itemGroup;
    QGraphicsRectItem *_rectBackground;
    QGraphicsRectItem *_rectTopLeft;
    QGraphicsRectItem *_rectTopRight;
    QGraphicsRectItem *_rectBottomLeft;
    QGraphicsRectItem *_rectBottomRight;
    MinoPropertyEasingCurve *_generatorCurve;
    MinoItemizedProperty *_generatorSteps;

};

#endif // MINOANIMATIONPLASMA_H
