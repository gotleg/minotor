#ifndef UIPROGRAM_H
#define UIPROGRAM_H

#include <QWidget>
#include <QFrame>
#include "minoprogram.h"

class UiProgram : public QWidget
{
    Q_OBJECT
public:
    explicit UiProgram(MinoProgram *program, QWidget *parent);
    void setOnAir(bool onAir);
private:
    MinoProgram *_program;
    QFrame *_fPreview;
    QWidget *_wBackground;
    QWidget *_wBorder;

signals:

public slots:
    void setExpanded(bool expanded);
private slots:
    void updateOnAirStatus(bool onAir);
    void requestMasterProgramChange(bool on);
    void exportProgram();
};

#endif // UIPROGRAM_H