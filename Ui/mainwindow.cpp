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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sstream>

#include <QtCore/QList>
#include <QtCore/QDebug>

#include <QToolBar>
#include <QToolButton>
#include <QLCDNumber>
#include <QSplitter>
#include <QDoubleSpinBox>
#include <QDesktopServices>
#include <QUrl>
#include <QMetaProperty>
#include <QFileDialog>
#include <QMessageBox>

#include "uianimationdescription.h"
#include "uianimationpicker.h"
#include "uianimationproperty.h"
#include "uiprogrambank.h"
#include "uiknob.h"
#include "uimidicontrollableparameter.h"

#include "minoanimation.h"
#include "minoproperty.h"
#include "minoprogram.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _minotor = Minotor::minotor();

    // Debug
    _minotor->initWithDebugSetup();
    ui->gvGlobal->setScene(_minotor->scene());
    ui->dwDebug->hide();

    // Configuration dialog box
    _configDialog = new ConfigDialog(this);

    // External master view
    _externalMasterView = new ExternalMasterView(this);
    _externalMasterView->setVisible(false);

    QVBoxLayout *lCentralWidget = new QVBoxLayout(ui->centralWidget);
    lCentralWidget->setSpacing(5);
    lCentralWidget->setMargin(0);
    lCentralWidget->setContentsMargins(10,0,5,0);

    //Toolbar
    QToolBar *_tToolBar = new QToolBar("Transport",this);
    this->addToolBar(Qt::TopToolBarArea,_tToolBar);

    QWidget *_wToolBar = new QWidget(_tToolBar);
    _tToolBar->addWidget(_wToolBar);
    QHBoxLayout *lToolBar = new QHBoxLayout(_wToolBar);

    QWidget *wToolContainer = new QWidget(this);
    lToolBar->addWidget(wToolContainer);
    wToolContainer->setObjectName("panel");
    QHBoxLayout *lToolContainer = new QHBoxLayout(wToolContainer);
    lToolContainer->setSpacing(0);
    lToolContainer->setMargin(0);
    lToolContainer->setContentsMargins(2,2,2,2);

    QWidget *wBackground = new QWidget(wToolContainer);
    lToolContainer->addWidget(wBackground);
    wBackground->setObjectName("panelcontent");
    QHBoxLayout *lBackground = new QHBoxLayout(wBackground);
    lBackground->setSpacing(10);
    lBackground->setMargin(0);
    lBackground->setContentsMargins(5,2,5,2);
    //Transport
    QWidget *wTransportButtons = new QWidget(wBackground);
    lBackground->addWidget(wTransportButtons);
    QHBoxLayout *lTransportButtons = new QHBoxLayout(wTransportButtons);
    lTransportButtons->setSpacing(5);
    lTransportButtons->setMargin(0);
    lTransportButtons->setContentsMargins(0,0,0,0);
    QPushButton *pbStart = new QPushButton(wTransportButtons);
    lTransportButtons->addWidget(pbStart);
    pbStart->setCheckable(true);
    pbStart->setToolTip("Start");
    pbStart->setIcon(QIcon(":/pictos/play.png"));
    pbStart->setIconSize(QSize(16,16));
    pbStart->setMinimumSize(28,28);
    pbStart->setMaximumSize(28,28);
    connect(pbStart,SIGNAL(clicked(bool)),_minotor->clockSource(),SLOT(uiStart()));
    connect(_minotor->clockSource(),SIGNAL(enabledChanged(bool)),pbStart, SLOT(setChecked(bool)));

    QPushButton *pbStop = new QPushButton(wTransportButtons);
    lTransportButtons->addWidget(pbStop);
    pbStop->setCheckable(true);
    pbStop->setChecked(true);
    pbStop->setToolTip("Stop");
    pbStop->setIcon(QIcon(":/pictos/stop.png"));
    pbStop->setIconSize(QSize(16,16));
    pbStop->setMinimumSize(28,28);
    pbStop->setMaximumSize(28,28);
    connect(pbStop,SIGNAL(clicked(bool)),_minotor->clockSource(),SLOT(uiStop()));
    connect(_minotor->clockSource(),SIGNAL(disabledChanged(bool)),pbStop, SLOT(setChecked(bool)));

    QPushButton *pbSync = new QPushButton(wTransportButtons);
    lTransportButtons->addWidget(pbSync);
    pbSync->setToolTip("Sync: click to define the first beat of the loop");
    pbSync->setIcon(QIcon(":/pictos/sync.png"));
    pbSync->setIconSize(QSize(16,16));
    pbSync->setMinimumSize(28,28);
    pbSync->setMaximumSize(28,28);
    connect(pbSync,SIGNAL(clicked(bool)),_minotor->clockSource(),SLOT(uiSync()));

    QWidget *wTempoButtons = new QWidget(wBackground);
    lBackground->addWidget(wTempoButtons);
    QHBoxLayout *lTempoButtons = new QHBoxLayout(wTempoButtons);
    lTempoButtons->setSpacing(5);
    lTempoButtons->setMargin(0);
    lTempoButtons->setContentsMargins(0,0,0,0);

    //Clock source
    _pbClockSource = new QPushButton(wTempoButtons);
    lTempoButtons->addWidget(_pbClockSource);
    _pbClockSource->setToolTip("Clock source: Choose internal clock or external midi clock");
    _pbClockSource->setIcon(QIcon(":/pictos/clock.png"));
    _pbClockSource->setText("Internal");
    _pbClockSource->setIconSize(QSize(20,20));
    _pbClockSource->setMinimumWidth(95);
    _pbClockSource->setCheckable(true);
    connect(_pbClockSource,SIGNAL(clicked(bool)),_minotor->clockSource(),SLOT(setExternalClockSource(bool)));
    connect(_minotor->clockSource(), SIGNAL(useExternalClockSourceChanged(bool)),this, SLOT(pbClockSourceChecked(bool)));
    
    //BPM
    QDoubleSpinBox *_sbBPM = new QDoubleSpinBox(wTempoButtons);
    _sbBPM->setToolTip("BPM: current tempo of the animation");
    _sbBPM->setMinimum(20);
    _sbBPM->setMaximum(300);
    _sbBPM->setSingleStep(0.1);
    _sbBPM->setDecimals(1);
    _sbBPM->setFocusPolicy(Qt::NoFocus);

    lTempoButtons->addWidget(_sbBPM);
    _sbBPM->setValue(_minotor->clockSource()->bpm());
    connect(_minotor->clockSource(), SIGNAL(bpmChanged(double)), _sbBPM, SLOT(setValue(double)));
    connect(_sbBPM,SIGNAL(valueChanged(double)),_minotor->clockSource(),SLOT(setBPM(double)));

    //Tap
    QPushButton *pbTap = new QPushButton(wTempoButtons);
    lTempoButtons->addWidget(pbTap);
    pbTap->setToolTip("Tap tempo: ajust tempo by clicking to the rythm");
    pbTap->setIcon(QIcon(":/pictos/tap.png"));
    pbTap->setIconSize(QSize(20,20));
    pbTap->setMinimumSize(28,28);
    pbTap->setMaximumSize(28,28);
    connect(pbTap,SIGNAL(clicked(bool)),_minotor->clockSource(),SLOT(uiTapOn()));

    QWidget *wMidiButtons = new QWidget(wBackground);
    lBackground->addWidget(wMidiButtons);
    QHBoxLayout *lMidiButtons = new QHBoxLayout(wMidiButtons);
    lMidiButtons->setSpacing(5);
    lMidiButtons->setMargin(0);
    lMidiButtons->setContentsMargins(0,0,0,0);

//    // MIDI learn
//    QPushButton *pbMidiLearn = new QPushButton(_tToolBar);
//    connect(pbMidiLearn,SIGNAL(toggled(bool)),this,SLOT(tbMidiLearnToggled(bool)));
//    pbMidiLearn->setToolTip("Active/Deactivate midi learn to map your controler");
//    pbMidiLearn->setText("MIDI learn");
//    pbMidiLearn->setCheckable(true);
//    lMidiButtons->addWidget(pbMidiLearn);

    lBackground->addStretch();
    //Midi monitors
    QWidget *wMidiMonitor = new QWidget(wBackground);
    lBackground->addWidget(wMidiMonitor);
    QVBoxLayout *lMidiMonitor = new QVBoxLayout(wMidiMonitor);
    lMidiMonitor->setSpacing(0);
    lMidiMonitor->setMargin(0);
    lMidiMonitor->setContentsMargins(0,0,0,0);
    //Midi clock
    QWidget *wMidiClock = new QWidget(wMidiMonitor);
    lMidiMonitor->addWidget(wMidiClock);
    QHBoxLayout *lMidiClock = new QHBoxLayout(wMidiClock);
    lMidiClock->setSpacing(5);
    lMidiClock->setMargin(0);
    lMidiClock->setContentsMargins(0,0,0,0);

    _wClockLed = new QWidget(wMidiClock);
    _wClockLed->setObjectName("led");
    lMidiClock->addWidget(_wClockLed);
    _wClockLed->setMinimumSize(10,10);
    _wClockLed->setMaximumSize(10,10);
    connect(_minotor->clockSource(),SIGNAL(beatToggled(bool)),this,SLOT(beatToggledReceived(bool)));

    QLabel *tClockLed = new QLabel(wMidiClock);
    lMidiClock->addWidget(tClockLed);
    tClockLed->setText("clock");

    //Midi note
    QWidget *wMidi = new QWidget(wMidiMonitor);
    lMidiMonitor->addWidget(wMidi);
    QHBoxLayout *lMidiNote = new QHBoxLayout(wMidi);
    lMidiNote->setSpacing(5);
    lMidiNote->setMargin(0);
    lMidiNote->setContentsMargins(0,0,0,0);

    _wMidiLed = new QWidget(wMidi);
    _wMidiLed->setObjectName("led");
    lMidiNote->addWidget(_wMidiLed);
    _wMidiLed->setMinimumSize(10,10);
    _wMidiLed->setMaximumSize(10,10);
    connect(Minotor::minotor()->midi(), SIGNAL(dataReceived()), this, SLOT(midiDataReceived()));

    QLabel *tNoteLed = new QLabel(wMidi);
    lMidiNote->addWidget(tNoteLed);
    tNoteLed->setText("midi");

    // Viewmode
    QPushButton *pbViewmode = new QPushButton(wBackground);
    lBackground->addWidget(pbViewmode);
    pbViewmode->setToolTip("Switch between editor mode and live mode");
    pbViewmode->setText(QString::fromUtf8("η"));
    pbViewmode->setIconSize(QSize(20,20));
    pbViewmode->setMinimumSize(28,28);
    pbViewmode->setMaximumSize(28,28);
    pbViewmode->setCheckable(true);
    connect(pbViewmode,SIGNAL(toggled(bool)),this,SLOT(tbViewmodeToggled(bool)));

    // Fullscreen
    QPushButton *pbFullscreen = new QPushButton(wBackground);
    pbFullscreen->setShortcut(Qt::Key_Escape);
    lBackground->addWidget(pbFullscreen);
    pbFullscreen->setToolTip("Fullscreen mode");
    pbFullscreen->setIcon(QIcon(":/pictos/fullscreen.png"));
    pbFullscreen->setIconSize(QSize(20,20));
    pbFullscreen->setMinimumSize(28,28);
    pbFullscreen->setMaximumSize(28,28);
    pbFullscreen->setCheckable(true);
    connect(pbFullscreen,SIGNAL(toggled(bool)),this,SLOT(tbFullScreenToggled(bool)));

    //UiMaster
    _uiMaster = new UiMaster(_minotor->master(), ui->centralWidget);
    lCentralWidget->addWidget(_uiMaster);

    // Programs bank
    createUiProgramBank(_minotor->programBank());
    connect(_minotor, SIGNAL(programBankChanged(QObject*)), this, SLOT(createUiProgramBank(QObject*)));

    connect(_uiMaster, SIGNAL(midiLearnToggled(bool)), this, SLOT(tbMidiLearnToggled(bool)));
    _tAnimationToolBar = new QToolBar("Animations",this);
    this->addToolBar(Qt::BottomToolBarArea,_tAnimationToolBar);
    QWidget *_wAnimations = new QWidget(_tAnimationToolBar);
    _tAnimationToolBar->addWidget(_wAnimations);
    QHBoxLayout *lAnimations = new QHBoxLayout(_wAnimations);

    QWidget *wContainer = new QWidget(this);
    lAnimations->addWidget(wContainer);
    wContainer->setObjectName("panel");
    QVBoxLayout *lContainer = new QVBoxLayout(wContainer);
    lContainer->setSpacing(0);
    lContainer->setMargin(0);
    lContainer->setContentsMargins(2,2,2,2);

    QWidget *wContent = new QWidget(wContainer);
    lContainer->addWidget(wContent);
    wContent->setObjectName("panelcontent");
    QVBoxLayout *lContent = new QVBoxLayout(wContent);
    lContent->setSpacing(0);
    lContent->setMargin(0);
    lContent->setContentsMargins(5,5,5,5);

    // Populate animation scrollarea
    QScrollArea *sa =  new QScrollArea();
    sa->setFrameShadow(QFrame::Plain);
    sa->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    sa->setFocusPolicy(Qt::NoFocus);
    sa->setWidgetResizable(true);
    UiAnimationPicker *ap = new UiAnimationPicker(sa);

    sa->setWidget(ap);
    lContent->addWidget(sa);

    // Set default viewmode to live
    pbViewmode->setChecked(false);
    tbViewmodeToggled(false);
}

void MainWindow::midiDataReceived()
{
    if(!_wMidiLed->property("active").toBool())
    {
        setMidiDataLedStatus(true);
    }
    _midiDataLedTimer.start(100,this);
}

void MainWindow::timerEvent(QTimerEvent *)
{
    setMidiDataLedStatus(false);
}

void MainWindow::setMidiDataLedStatus(bool active)
{
    _wMidiLed->setProperty("active", active);
    _wMidiLed->style()->unpolish(_wMidiLed);
    _wMidiLed->style()->polish(_wMidiLed);
}

MainWindow::~MainWindow()
{
    delete _configDialog;
    delete ui;
}

void MainWindow::on_action_Configuration_triggered()
{
    _configDialog->exec();
}

void MainWindow::on_action_MinotorWiki_triggered()
{
    QDesktopServices::openUrl(QUrl("http://wiki.minotor.org"));
}

void MainWindow::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionLoad_triggered()
{
    QString dataPath = Minotor::dataPath();
    _programBankFileName = QFileDialog::getOpenFileName(this, tr("Load File"), dataPath,tr("Program (*.mpb)"));
    QSettings parser(_programBankFileName, QSettings::IniFormat);
    _minotor->load(&parser);
}

void MainWindow::on_actionSave_triggered()
{
    if (!_programBankFileName.isEmpty())
    {
        QFile::remove(_programBankFileName);
        QSettings parser(_programBankFileName, QSettings::IniFormat);
        _minotor->save(_minotor->programBank(), &parser);
    }
    else
    {
        on_actionSaveAs_triggered();
    }
}

void MainWindow::on_actionSaveAs_triggered()
{
    QString dataPath = Minotor::dataPath();
    _programBankFileName = QFileDialog::getSaveFileName(this, tr("Save File"), dataPath,tr(" (*.mpb)"));
    if(QFile::exists(_programBankFileName))
    {
        QFile::remove(_programBankFileName);
    }
    QSettings parser(_programBankFileName, QSettings::IniFormat);
    _minotor->save(_minotor->programBank(), &parser);
}

void MainWindow::on_actionNewProgram_triggered()
{
    new MinoProgram(_minotor->programBank());
}

void MainWindow::on_actionImport_triggered()
{
    QString dataPath = Minotor::dataPath();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), dataPath,tr(" (*.mpr)"));
    QSettings parser(fileName, QSettings::IniFormat);
    _minotor->load(&parser);
}

void MainWindow::on_sPpqn_valueChanged(int value)
{
    static unsigned int uppqn = 0;
    const unsigned int ppqn = value%24;
    const unsigned int qn = value/24;
    _minotor->dispatchClock(uppqn, value, ppqn, qn);
    uppqn++;
}

void MainWindow::tbMidiLearnToggled(bool checked)
{
    QList<UiMidiControllableParameter*> midiProperties = findChildren<UiMidiControllableParameter*>();
    foreach(UiMidiControllableParameter *midiProperty, midiProperties)
    {
        midiProperty->setMidiLearnMode(checked);
    }
}

void MainWindow::pbClockSourceChecked(bool checked)
{
    if (checked)
    {
        _pbClockSource->setText("External");
    }
    else
    {
        _pbClockSource->setText("Internal");
    }
	_pbClockSource->setChecked(checked);
}

void MainWindow::tbViewmodeToggled(bool on)
{
    _uiMaster->setVisible(on);
    _tAnimationToolBar->setVisible(!on);
}

void MainWindow::tbFullScreenToggled(bool on)
{
    (void)on;
    if(isFullScreen()) {
         this->setWindowState(Qt::WindowMaximized);
      } else {
         this->setWindowState(Qt::WindowFullScreen);
      }
}

void MainWindow::on_pbShot_clicked()
{
    static int id = 0;
    QString shotname = "master_";
    shotname.append(QString::number(id++));
    shotname.append(QString(".png"));

    qDebug() << Q_FUNC_INFO
             << "shotname:" << shotname;
    UiProgramView *masterMonitor = findChild<UiProgramView*>("masterMonitor");
    if(masterMonitor)
    {
        QPixmap p = QPixmap::grabWidget(masterMonitor);
        qDebug() << Q_FUNC_INFO
                 << "saved:" << p.save(shotname);
    }
}

void MainWindow::on_pbScene_clicked()
{
    QList<QGraphicsItem*> items = Minotor::minotor()->scene()->items();
    foreach(QGraphicsItem *item, items)
    {
        qDebug() << item;
    }
}

void MainWindow::beatToggledReceived(bool active)
{
    _wClockLed->setProperty("active", active);
    _wClockLed->style()->unpolish(_wClockLed);
    _wClockLed->style()->polish(_wClockLed);
}

void MainWindow::on_actionExternal_master_view_toggled(bool on)
{
    _externalMasterView->setVisible(on);
}

void MainWindow::on_actionNew_triggered()
{
    QMessageBox::StandardButton ret = QMessageBox::question(this,"Create a new program bank","This will erase you current bank.",QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Ok);

    if (ret == QMessageBox::Ok) {
            _minotor->clearPrograms();
     }
}

void MainWindow::createUiProgramBank(QObject *bank)
{
    MinoProgramBank *programBank = qobject_cast<MinoProgramBank*>(bank);
    Q_ASSERT(programBank);
    UiProgramBank *uiProgramBank = new UiProgramBank(programBank, ui->centralWidget);
    QLayout *layout = ui->centralWidget->layout();
    layout->addWidget(uiProgramBank);

}
