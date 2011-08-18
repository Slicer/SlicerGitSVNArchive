/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QCompleter>
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QToolButton>

// CTK includes
#include "qSlicerAbstractModule.h"
#include "qSlicerApplication.h"
#include "qSlicerModuleManager.h"
#include "ctkMenuComboBox.h"

// SlicerQt includes
#include "qSlicerModuleSelectorToolBar.h"
#include "qSlicerModulesMenu.h"

class qSlicerModuleSelectorToolBarPrivate
{
  Q_DECLARE_PUBLIC(qSlicerModuleSelectorToolBar);
protected:
  qSlicerModuleSelectorToolBar* const q_ptr;
public:
  qSlicerModuleSelectorToolBarPrivate(qSlicerModuleSelectorToolBar& object);
  void init();

  void insertActionOnTop(QAction* action, QMenu* menu);
  QAction* lastSelectedAction()const;

  qSlicerModulesMenu* ModulesMenu;

  ctkMenuComboBox*  ModulesComboBox;

  QMenu*            HistoryMenu;
  QToolButton*      HistoryButton;
  QToolButton*      PreviousButton;
  QMenu*            PreviousHistoryMenu;
  QToolButton*      NextButton;
  QMenu*            NextHistoryMenu;
};


//---------------------------------------------------------------------------
qSlicerModuleSelectorToolBarPrivate::qSlicerModuleSelectorToolBarPrivate(qSlicerModuleSelectorToolBar& object)
  : q_ptr(&object)
{
  this->ModulesMenu = 0;
  this->ModulesComboBox = 0;
  this->HistoryMenu = 0;
  this->HistoryButton = 0;
  this->PreviousButton = 0;
  this->PreviousHistoryMenu = 0;
  this->NextButton = 0;
  this->NextHistoryMenu = 0;
}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBarPrivate::init()
{
  Q_Q(qSlicerModuleSelectorToolBar);
  QIcon previousIcon = q->style()->standardIcon(QStyle::SP_ArrowLeft);
  QIcon nextIcon = q->style()->standardIcon(QStyle::SP_ArrowRight);
  QIcon historyIcon(":Icons/ModuleHistory.png");

  // Modules menu
  this->ModulesMenu = new qSlicerModulesMenu(QObject::tr("Modules"),q);
  QObject::connect(this->ModulesMenu, SIGNAL(currentModuleChanged(const QString&)),
                   q, SLOT(onModuleSelected(const QString&)));

  // Modules Label
  q->addWidget(new QLabel(QObject::tr("Modules:"), q));

  // Modules comboBox
  this->ModulesComboBox = new ctkMenuComboBox(q);
  this->ModulesComboBox->setToolTip(QObject::tr("Select a module from the module list"));
  this->ModulesComboBox->setMenu(this->ModulesMenu);
  this->ModulesComboBox->setEditableBehavior(ctkMenuComboBox::EditableOnDoubleClick);
  this->ModulesComboBox->setMinimumContentsLength(20);
  q->addWidget(this->ModulesComboBox);

  // History
  this->HistoryMenu = new QMenu(QObject::tr("Modules history"), q);
  this->HistoryButton = new QToolButton;
  this->HistoryButton->setIcon(historyIcon);
  this->HistoryButton->setToolTip(QObject::tr("Modules history"));
  this->HistoryButton->setMenu(this->HistoryMenu);
  this->HistoryButton->setPopupMode(QToolButton::InstantPopup);
  q->addWidget(this->HistoryButton);

  // Previous button
  this->PreviousHistoryMenu = new QMenu("Modules Previous History", q);
  this->PreviousButton = new QToolButton(q);
  this->PreviousButton->setIcon(previousIcon);
  this->PreviousButton->setText(QObject::tr("Previous"));
  this->PreviousButton->setToolTip(QObject::tr("Previous modules"));
  this->PreviousButton->setMenu(this->PreviousHistoryMenu);
  // selectPreviousModule is called only if the toolbutton is clicked not if an
  // action in the history is triggered
  QObject::connect(this->PreviousButton, SIGNAL(clicked(bool)),
                   q, SLOT(selectPreviousModule()));
  q->addWidget(this->PreviousButton);
  this->PreviousButton->setEnabled(this->PreviousHistoryMenu->actions().size() > 0);

  // Next button
  this->NextHistoryMenu = new QMenu("Modules Next History", q);
  this->NextButton = new QToolButton(q);
  this->NextButton->setIcon(nextIcon);
  this->NextButton->setText(QObject::tr("Next"));
  this->NextButton->setToolTip(QObject::tr("Next modules"));
  this->NextButton->setMenu(this->NextHistoryMenu);
  // selectNextModule is called only if the toolbutton is clicked not if an
  // action in the history is triggered
  QObject::connect(this->NextButton, SIGNAL(clicked(bool)),
                   q, SLOT(selectNextModule()));
  q->addWidget(this->NextButton);
  this->NextButton->setEnabled(this->NextHistoryMenu->actions().size() > 0);

}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBarPrivate::insertActionOnTop(QAction* action, QMenu* menu)
{
  menu->removeAction(action);
  QAction* before = menu->actions().isEmpty() ? 0 : menu->actions().first();
  menu->insertAction(before, action);
  QList<QAction*> actions = menu->actions();
  for (int i = 8; i < actions.size(); ++i)
    {
    menu->removeAction(actions[i]);
    }
}

//---------------------------------------------------------------------------
QAction* qSlicerModuleSelectorToolBarPrivate::lastSelectedAction()const
{
  QList<QAction*> actions = this->HistoryMenu->actions();
  return actions.size() ? actions[0] : 0;
}

//---------------------------------------------------------------------------
qSlicerModuleSelectorToolBar::qSlicerModuleSelectorToolBar(const QString& title,
                                                           QWidget* parentWidget)
  : Superclass(title, parentWidget)
  , d_ptr(new qSlicerModuleSelectorToolBarPrivate(*this))
{
  Q_D(qSlicerModuleSelectorToolBar);
  d->init();
}

//---------------------------------------------------------------------------
qSlicerModuleSelectorToolBar::qSlicerModuleSelectorToolBar(QWidget* parentWidget)
  : Superclass(parentWidget)
  , d_ptr(new qSlicerModuleSelectorToolBarPrivate(*this))
{
  Q_D(qSlicerModuleSelectorToolBar);
  d->init();
}

//---------------------------------------------------------------------------
qSlicerModuleSelectorToolBar::~qSlicerModuleSelectorToolBar()
{
}

//---------------------------------------------------------------------------
qSlicerModulesMenu* qSlicerModuleSelectorToolBar::modulesMenu()const
{
  Q_D(const qSlicerModuleSelectorToolBar);
  return d->ModulesMenu;
}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBar::setModuleManager(qSlicerModuleManager* moduleManager)
{
  Q_D(qSlicerModuleSelectorToolBar);

  if (d->ModulesMenu->moduleManager())
    {
    QObject::disconnect(d->ModulesMenu->moduleManager(),
                        SIGNAL(moduleAboutToBeUnloaded(qSlicerAbstractCoreModule*)),
                        this, SLOT(moduleRemoved(qSlicerAbstractCoreModule*)));
    }
  d->ModulesMenu->setModuleManager(moduleManager);

  if (moduleManager)
    {
    QObject::connect(moduleManager,
                     SIGNAL(moduleAboutToBeUnloaded(qSlicerAbstractCoreModule*)),
                     this, SLOT(moduleRemoved(qSlicerAbstractCoreModule*)));
    }
}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBar::moduleRemoved(qSlicerAbstractCoreModule* moduleRemoved)
{
  Q_D(qSlicerModuleSelectorToolBar);
  qSlicerAbstractModule* module = qobject_cast<qSlicerAbstractModule*>(moduleRemoved);
  if (!module)
    {
    return;
    }
  QAction* moduleAction = module->action();
  // removing a module consists in retrieving the unique action of the module
  // and removing it from all the possible menus
  d->HistoryMenu->removeAction(moduleAction);
  d->PreviousHistoryMenu->removeAction(moduleAction);
  d->NextHistoryMenu->removeAction(moduleAction);
  // TBD: what if the module is the current module ?
}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBar::selectModule(const QString& moduleName)
{
  Q_D(qSlicerModuleSelectorToolBar);
  d->ModulesMenu->setCurrentModule(moduleName);
}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBar::onModuleSelected(const QString& name)
{
  Q_D(qSlicerModuleSelectorToolBar);
  this->actionSelected(d->ModulesMenu->moduleAction(name));
}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBar::actionSelected(QAction* action)
{
  Q_D(qSlicerModuleSelectorToolBar);
  QAction* lastAction = d->lastSelectedAction();
  if (action == lastAction)
    {
    return;
    }
  QList<QAction*> previousActions = d->PreviousHistoryMenu->actions();
  QList<QAction*> nextActions = d->NextHistoryMenu->actions();
  int actionIndexInPreviousMenu = previousActions.indexOf(action);
  int actionIndexInNextMenu = nextActions.indexOf(action);
  if ( actionIndexInNextMenu >= 0)
    {
    if (lastAction)
      {
      previousActions.push_front(lastAction);
      }
    for (int i = 0; i < actionIndexInNextMenu ; ++i)
      {
      previousActions.push_front(nextActions.takeFirst());
      }
    Q_ASSERT(nextActions[0] == action);
    nextActions.removeFirst();
    }
  else if ( actionIndexInPreviousMenu >= 0)
    {
    if (lastAction)
      {
      nextActions.push_front(lastAction);
      }
    for (int i = 0; i < actionIndexInPreviousMenu  ; ++i)
      {
      nextActions.push_front(previousActions.takeFirst());
      }
    Q_ASSERT(previousActions[0] == action);
    previousActions.removeFirst();
    }
  else
    {
    if (lastAction)
      {
      previousActions.push_front(lastAction);
      }
    nextActions.clear();
    }
  // don't keep more than X history
  previousActions = previousActions.mid(0, 8);
  nextActions = nextActions.mid(0, 8);

  d->PreviousHistoryMenu->clear();
  d->PreviousHistoryMenu->addActions(previousActions);
  d->NextHistoryMenu->clear();
  d->NextHistoryMenu->addActions(nextActions);

  d->PreviousButton->setEnabled(d->PreviousHistoryMenu->actions().size());
  d->NextButton->setEnabled(d->NextHistoryMenu->actions().size());

  if (action)
    {
    d->insertActionOnTop(action, d->HistoryMenu);
    }
  emit moduleSelected(action->data().toString());
}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBar::selectNextModule()
{
  Q_D(qSlicerModuleSelectorToolBar);
  // selectNextModule() is not called when an action from the next history menu
  // is triggered. selectNextModule() is called only if the next toolbutton is
  // clicked.
  QList<QAction*> actions = d->NextHistoryMenu->actions();
  QAction* nextAction = actions.size() ? actions.first() : 0;
  if (nextAction)
    {
    // triggering the action will eventually call actionSelected()
    nextAction->trigger();
    }
}

//---------------------------------------------------------------------------
void qSlicerModuleSelectorToolBar::selectPreviousModule()
{
  Q_D(qSlicerModuleSelectorToolBar);
  // selectPreviousModule() is not called when an action from the Previous
  // history menu is triggered. selectPreviousModule() is called only if the
  // previous toolbutton is clicked.
  QList<QAction*> actions = d->PreviousHistoryMenu->actions();
  QAction* previousAction = actions.size() ? actions.first() : 0;
  if (previousAction)
    {
    // triggering the action will eventually call actionSelected()
    previousAction->trigger();
    }
}
