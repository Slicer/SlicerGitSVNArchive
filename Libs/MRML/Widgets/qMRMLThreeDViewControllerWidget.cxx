/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

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
#include <QActionGroup>
#include <QDebug>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>

// CTK includes
#include <ctkButtonGroup.h>
#include <ctkPopupWidget.h>
#include <ctkSignalMapper.h>

// qMRML includes
#include "qMRMLColors.h"
#include "qMRMLNodeFactory.h"
#include "qMRMLSceneViewMenu.h"
#include "qMRMLThreeDView.h"
#include "qMRMLThreeDViewControllerWidget_p.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLViewNode.h>
#include <vtkMRMLSceneViewNode.h>

// VTK includes
#include <vtkRenderWindow.h>

//--------------------------------------------------------------------------
// qMRMLThreeDViewControllerWidgetPrivate methods

//---------------------------------------------------------------------------
qMRMLThreeDViewControllerWidgetPrivate::qMRMLThreeDViewControllerWidgetPrivate(
  qMRMLThreeDViewControllerWidget& object)
  : Superclass(object)
{
  this->ViewNode = 0;
  this->ThreeDView = 0;
  this->CenterToolButton = 0;
}

//---------------------------------------------------------------------------
qMRMLThreeDViewControllerWidgetPrivate::~qMRMLThreeDViewControllerWidgetPrivate()
{
}

//---------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidgetPrivate::setupPopupUi()
{
  Q_Q(qMRMLThreeDViewControllerWidget);

  this->Superclass::setupPopupUi();
  this->PopupWidget->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
  this->Ui_qMRMLThreeDViewControllerWidget::setupUi(this->PopupWidget);

  // Look from axes
  QObject::connect(this->AxesWidget,
                   SIGNAL(currentAxisChanged(ctkAxesWidget::Axis)),
                   q, SLOT(lookFromAxis(ctkAxesWidget::Axis)));

  // Orthographic/perspective button
  QObject::connect(this->OrthoButton, SIGNAL(toggled(bool)),
                   q, SLOT(setOrthographicModeEnabled(bool)));

  // ZoomIn, ZoomOut button
  QObject::connect(this->ZoomInButton, SIGNAL(clicked()),
                   q, SLOT(zoomIn()));
  QObject::connect(this->ZoomOutButton, SIGNAL(clicked()),
                   q, SLOT(zoomOut()));

  // ResetFocalPoint button
  this->CenterButton->setDefaultAction(this->actionCenter);
  QObject::connect(this->actionCenter, SIGNAL(triggered()),
                   q, SLOT(resetFocalPoint()));

  // StereoType actions
  this->StereoTypesMapper = new ctkSignalMapper(this->PopupWidget);
  this->StereoTypesMapper->setMapping(this->actionNoStereo,
                                      vtkMRMLViewNode::NoStereo);
  this->StereoTypesMapper->setMapping(this->actionSwitchToAnaglyphStereo,
                                      vtkMRMLViewNode::Anaglyph);
  this->StereoTypesMapper->setMapping(this->actionSwitchToQuadBufferStereo,
                                      vtkMRMLViewNode::QuadBuffer);
  this->StereoTypesMapper->setMapping(this->actionSwitchToInterlacedStereo,
                                      vtkMRMLViewNode::Interlaced);
  this->StereoTypesMapper->setMapping(this->actionSwitchToRedBlueStereo,
                                      vtkMRMLViewNode::RedBlue);
  this->StereoTypesMapper->setMapping(this->actionSwitchToUserDefinedStereo_1,
                                      vtkMRMLViewNode::UserDefined_1);
  this->StereoTypesMapper->setMapping(this->actionSwitchToUserDefinedStereo_2,
                                      vtkMRMLViewNode::UserDefined_2);
  this->StereoTypesMapper->setMapping(this->actionSwitchToUserDefinedStereo_3,
                                      vtkMRMLViewNode::UserDefined_3);
  QActionGroup* stereoTypesActions = new QActionGroup(this->PopupWidget);
  stereoTypesActions->setExclusive(true);
  stereoTypesActions->addAction(this->actionNoStereo);
  stereoTypesActions->addAction(this->actionSwitchToRedBlueStereo);
  stereoTypesActions->addAction(this->actionSwitchToAnaglyphStereo);
  stereoTypesActions->addAction(this->actionSwitchToInterlacedStereo);
  stereoTypesActions->addAction(this->actionSwitchToQuadBufferStereo);
  stereoTypesActions->addAction(this->actionSwitchToUserDefinedStereo_1);
  stereoTypesActions->addAction(this->actionSwitchToUserDefinedStereo_2);
  stereoTypesActions->addAction(this->actionSwitchToUserDefinedStereo_3);
  QMenu* stereoTypesMenu = new QMenu("Stereo Modes", this->PopupWidget);
  stereoTypesMenu->setObjectName("stereoTypesMenu");
  stereoTypesMenu->addActions(stereoTypesActions->actions());
  this->StereoButton->setMenu(stereoTypesMenu);
  QObject::connect(this->StereoTypesMapper, SIGNAL(mapped(int)),
                   q, SLOT(setStereoType(int)));
  QObject::connect(stereoTypesActions, SIGNAL(triggered(QAction*)),
                   this->StereoTypesMapper, SLOT(map(QAction*)));
  this->actionSwitchToQuadBufferStereo->setEnabled(false); // Disabled by default

  QMenu* visibilityMenu = new QMenu("Visibility", this->PopupWidget);
  visibilityMenu->setObjectName("visibilityMenu");
  this->VisibilityButton->setMenu(visibilityMenu);

  // Show 3D Axis, 3D Axis label
  visibilityMenu->addAction(this->actionSet3DAxisVisible);
  visibilityMenu->addAction(this->actionSet3DAxisLabelVisible);
  QObject::connect(this->actionSet3DAxisVisible, SIGNAL(triggered(bool)),
                   q, SLOT(set3DAxisVisible(bool)));
  QObject::connect(this->actionSet3DAxisLabelVisible, SIGNAL(triggered(bool)),
                   q, SLOT(set3DAxisLabelVisible(bool)));

  // OrientationMarker actions
  // Type
  this->OrientationMarkerTypesMapper = new ctkSignalMapper(this->PopupWidget);
  this->OrientationMarkerTypesMapper->setMapping(this->actionOrientationMarkerTypeNone, vtkMRMLAbstractViewNode::OrientationMarkerTypeNone);
  this->OrientationMarkerTypesMapper->setMapping(this->actionOrientationMarkerTypeCube, vtkMRMLAbstractViewNode::OrientationMarkerTypeCube);
  this->OrientationMarkerTypesMapper->setMapping(this->actionOrientationMarkerTypeHuman, vtkMRMLAbstractViewNode::OrientationMarkerTypeHuman);
  this->OrientationMarkerTypesMapper->setMapping(this->actionOrientationMarkerTypeAxes, vtkMRMLAbstractViewNode::OrientationMarkerTypeAxes);
  QActionGroup* orientationMarkerTypesActions = new QActionGroup(this->PopupWidget);
  orientationMarkerTypesActions->setExclusive(true);
  orientationMarkerTypesActions->addAction(this->actionOrientationMarkerTypeNone);
  orientationMarkerTypesActions->addAction(this->actionOrientationMarkerTypeCube);
  orientationMarkerTypesActions->addAction(this->actionOrientationMarkerTypeHuman);
  orientationMarkerTypesActions->addAction(this->actionOrientationMarkerTypeAxes);
  QObject::connect(this->OrientationMarkerTypesMapper, SIGNAL(mapped(int)),q, SLOT(setOrientationMarkerType(int)));
  QObject::connect(orientationMarkerTypesActions, SIGNAL(triggered(QAction*)),this->OrientationMarkerTypesMapper, SLOT(map(QAction*)));
  // Size
  this->OrientationMarkerSizesMapper = new ctkSignalMapper(this->PopupWidget);
  this->OrientationMarkerSizesMapper->setMapping(this->actionOrientationMarkerSizeSmall, vtkMRMLAbstractViewNode::OrientationMarkerSizeSmall);
  this->OrientationMarkerSizesMapper->setMapping(this->actionOrientationMarkerSizeMedium, vtkMRMLAbstractViewNode::OrientationMarkerSizeMedium);
  this->OrientationMarkerSizesMapper->setMapping(this->actionOrientationMarkerSizeLarge, vtkMRMLAbstractViewNode::OrientationMarkerSizeLarge);
  QActionGroup* orientationMarkerSizesActions = new QActionGroup(this->PopupWidget);
  orientationMarkerSizesActions->setExclusive(true);
  orientationMarkerSizesActions->addAction(this->actionOrientationMarkerSizeSmall);
  orientationMarkerSizesActions->addAction(this->actionOrientationMarkerSizeMedium);
  orientationMarkerSizesActions->addAction(this->actionOrientationMarkerSizeLarge);
  QObject::connect(this->OrientationMarkerSizesMapper, SIGNAL(mapped(int)),q, SLOT(setOrientationMarkerSize(int)));
  QObject::connect(orientationMarkerSizesActions, SIGNAL(triggered(QAction*)),this->OrientationMarkerSizesMapper, SLOT(map(QAction*)));
  // Menu
  QMenu* orientationMarkerMenu = new QMenu(tr("Orientation marker"), this->PopupWidget);
  orientationMarkerMenu->setObjectName("orientationMarkerMenu");
  this->OrientationMarkerButton->setMenu(orientationMarkerMenu);
  orientationMarkerMenu->addActions(orientationMarkerTypesActions->actions());
  orientationMarkerMenu->addSeparator();
  orientationMarkerMenu->addActions(orientationMarkerSizesActions->actions());

  // Ruler actions
  // Type
  this->RulerTypesMapper = new ctkSignalMapper(this->PopupWidget);
  this->RulerTypesMapper->setMapping(this->actionRulerTypeNone, vtkMRMLAbstractViewNode::RulerTypeNone);
  this->RulerTypesMapper->setMapping(this->actionRulerTypeThin, vtkMRMLAbstractViewNode::RulerTypeThin);
  this->RulerTypesMapper->setMapping(this->actionRulerTypeThick, vtkMRMLAbstractViewNode::RulerTypeThick);
  QActionGroup* rulerTypesActions = new QActionGroup(this->PopupWidget);
  rulerTypesActions->setExclusive(true);
  rulerTypesActions->addAction(this->actionRulerTypeNone);
  rulerTypesActions->addAction(this->actionRulerTypeThin);
  rulerTypesActions->addAction(this->actionRulerTypeThick);
  QObject::connect(this->RulerTypesMapper, SIGNAL(mapped(int)),q, SLOT(setRulerType(int)));
  QObject::connect(rulerTypesActions, SIGNAL(triggered(QAction*)),this->RulerTypesMapper, SLOT(map(QAction*)));
  // Menu
  QMenu* rulerMenu = new QMenu(tr("Ruler"), this->PopupWidget);
  rulerMenu->setObjectName("rulerMenu");
  this->RulerButton->setMenu(rulerMenu);
  rulerMenu->addActions(rulerTypesActions->actions());

  // More controls
  QMenu* moreMenu = new QMenu("More", this->PopupWidget);
  moreMenu->addAction(this->actionUseDepthPeeling);
  moreMenu->addAction(this->actionSetFPSVisible);
  this->MoreToolButton->setMenu(moreMenu);

  // Depth peeling
  QObject::connect(this->actionUseDepthPeeling, SIGNAL(toggled(bool)),
                   q, SLOT(setUseDepthPeeling(bool)));

  // FPS
  QObject::connect(this->actionSetFPSVisible, SIGNAL(toggled(bool)),
                   q, SLOT(setFPSVisible(bool)));

  // Background color
  QActionGroup* backgroundColorActions = new QActionGroup(this->PopupWidget);
  backgroundColorActions->setExclusive(true);
  visibilityMenu->addSeparator();
  visibilityMenu->addAction(this->actionSetLightBlueBackground);
  visibilityMenu->addAction(this->actionSetBlackBackground);
  visibilityMenu->addAction(this->actionSetWhiteBackground);
  backgroundColorActions->addAction(this->actionSetLightBlueBackground);
  backgroundColorActions->addAction(this->actionSetBlackBackground);
  backgroundColorActions->addAction(this->actionSetWhiteBackground);
  QObject::connect(this->actionSetLightBlueBackground, SIGNAL(triggered()),
                   q, SLOT(setLightBlueBackground()));
  QObject::connect(this->actionSetWhiteBackground, SIGNAL(triggered()),
                   q, SLOT(setWhiteBackground()));
  QObject::connect(this->actionSetBlackBackground, SIGNAL(triggered()),
                   q, SLOT(setBlackBackground()));

  // SpinView, RockView buttons
  this->AnimateViewButtonGroup = new ctkButtonGroup(this->PopupWidget);
  this->AnimateViewButtonGroup->addButton(this->SpinButton, vtkMRMLViewNode::Spin);
  this->AnimateViewButtonGroup->addButton(this->RockButton, vtkMRMLViewNode::Rock);
  QObject::connect(this->SpinButton, SIGNAL(toggled(bool)),
                   q, SLOT(spinView(bool)));
  QObject::connect(this->RockButton, SIGNAL(toggled(bool)),
                   q, SLOT(rockView(bool)));
}

//---------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidgetPrivate::init()
{
  Q_Q(qMRMLThreeDViewControllerWidget);
  this->Superclass::init();

  this->CenterToolButton = new QToolButton(q);
  this->CenterToolButton->setAutoRaise(true);
  this->CenterToolButton->setDefaultAction(this->actionCenter);
  this->CenterToolButton->setFixedSize(15, 15);
  this->BarLayout->insertWidget(2, this->CenterToolButton);

  this->ViewLabel->setText(qMRMLThreeDViewControllerWidget::tr("1"));
  this->BarLayout->addStretch(1);
  this->setColor(qMRMLColors::threeDViewBlue());
}

// --------------------------------------------------------------------------
// qMRMLThreeDViewControllerWidget methods

// --------------------------------------------------------------------------
qMRMLThreeDViewControllerWidget::qMRMLThreeDViewControllerWidget(QWidget* parentWidget)
  : Superclass(new qMRMLThreeDViewControllerWidgetPrivate(*this), parentWidget)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  d->init();
}

// --------------------------------------------------------------------------
qMRMLThreeDViewControllerWidget::~qMRMLThreeDViewControllerWidget()
{
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setThreeDView(qMRMLThreeDView* view)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  d->ThreeDView = view;
  if(d->ThreeDView != 0)
    {
    d->actionSwitchToQuadBufferStereo->setEnabled(
          d->ThreeDView->renderWindow()->GetStereoCapableWindow());
    }
}

//---------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setViewLabel(const QString& newViewLabel)
{
  Q_D(qMRMLThreeDViewControllerWidget);

  if (d->ViewNode)
    {
    qCritical() << "qMRMLThreeDViewControllerWidget::setViewLabel should be called before setViewNode !";
    return;
    }

  d->ThreeDViewLabel = newViewLabel;
  d->ViewLabel->setText(d->ThreeDViewLabel);
}

//---------------------------------------------------------------------------
CTK_GET_CPP(qMRMLThreeDViewControllerWidget, QString, viewLabel, ThreeDViewLabel);


// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setMRMLViewNode(
    vtkMRMLViewNode * viewNode)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  this->qvtkReconnect(d->ViewNode, viewNode, vtkCommand::ModifiedEvent,
                      this, SLOT(updateWidgetFromMRML()));
  d->ViewNode = viewNode;
  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLThreeDViewControllerWidget);
  // Enable buttons
  QList<QWidget*> widgets;
  widgets << d->AxesWidget
    << d->CenterButton << d->OrthoButton << d->VisibilityButton
    << d->ZoomInButton << d->ZoomOutButton << d->StereoButton
    << d->RockButton << d->SpinButton << d->MoreToolButton
    << d->OrientationMarkerButton; // RulerButton enable state is not set here (it depends on render mode)
  foreach(QWidget* w, widgets)
    {
    w->setEnabled(d->ViewNode != 0);
    }

  if (!d->ViewNode)
    {
    return;
    }

  // In the axes widget the order of labels is: +X, -X, +Z, -Z, +Y, -Y
  // and in the view node axis labels order is: -X, +X, -Y, +Y, -Z, +Z.
  QStringList axesLabels;
  axesLabels <<  d->ViewNode->GetAxisLabel(1); // +X
  axesLabels <<  d->ViewNode->GetAxisLabel(0); // -X
  axesLabels <<  d->ViewNode->GetAxisLabel(5); // +Z
  axesLabels <<  d->ViewNode->GetAxisLabel(4); // -Z
  axesLabels <<  d->ViewNode->GetAxisLabel(3); // +Y
  axesLabels <<  d->ViewNode->GetAxisLabel(2); // -Y
  d->AxesWidget->setAxesLabels(axesLabels);

  d->actionSet3DAxisVisible->setChecked(d->ViewNode->GetBoxVisible());
  d->actionSet3DAxisLabelVisible->setChecked(
    d->ViewNode->GetAxisLabelsVisible());

  d->actionUseDepthPeeling->setChecked(d->ViewNode->GetUseDepthPeeling());
  d->actionSetFPSVisible->setChecked(d->ViewNode->GetFPSVisible());

  double* color = d->ViewNode->GetBackgroundColor();
  QColor backgroundColor = QColor::fromRgbF(color[0], color[1], color[2]);
  d->actionSetBlackBackground->setChecked(backgroundColor == Qt::black);
  d->actionSetWhiteBackground->setChecked(backgroundColor == Qt::white);
  d->actionSetLightBlueBackground->setChecked(
    !d->actionSetBlackBackground->isChecked() &&
    !d->actionSetWhiteBackground->isChecked());

  d->OrthoButton->setChecked(d->ViewNode->GetRenderMode() == vtkMRMLViewNode::Orthographic);

  QAction* action = qobject_cast<QAction*>(d->StereoTypesMapper->mapping(d->ViewNode->GetStereoType()));
  if (action)
    {
    action->setChecked(true);
    }
  action = qobject_cast<QAction*>(d->OrientationMarkerTypesMapper->mapping(d->ViewNode->GetOrientationMarkerType()));
  if (action)
    {
    action->setChecked(true);
    }
  action = qobject_cast<QAction*>(d->OrientationMarkerSizesMapper->mapping(d->ViewNode->GetOrientationMarkerSize()));
  if (action)
    {
    action->setChecked(true);
    }
  action = qobject_cast<QAction*>(d->RulerTypesMapper->mapping(d->ViewNode->GetRulerType()));
  if (action)
    {
    action->setChecked(true);
    }
  d->RulerButton->setEnabled(d->ViewNode->GetRenderMode()==vtkMRMLViewNode::Orthographic);

  d->SpinButton->setChecked(d->ViewNode->GetAnimationMode() == vtkMRMLViewNode::Spin);
  d->RockButton->setChecked(d->ViewNode->GetAnimationMode() == vtkMRMLViewNode::Rock);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setQuadBufferStereoSupportEnabled(bool value)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  d->actionSwitchToQuadBufferStereo->setEnabled(value);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setOrthographicModeEnabled(bool enabled)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  d->ViewNode->SetRenderMode(
    enabled ? vtkMRMLViewNode::Orthographic : vtkMRMLViewNode::Perspective);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::lookFromAxis(const ctkAxesWidget::Axis& axis)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ThreeDView)
    {
    return;
    }
  d->ThreeDView->lookFromViewAxis(axis);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::pitchView()
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ThreeDView)
    {
    return;
    }
  d->ThreeDView->pitch();
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::rollView()
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ThreeDView)
    {
    return;
    }
  d->ThreeDView->roll();
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::yawView()
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ThreeDView)
    {
    return;
    }
  d->ThreeDView->yaw();
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::zoomIn()
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ThreeDView->zoomIn();
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::zoomOut()
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ThreeDView->zoomOut();
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::spinView(bool enabled)
{
  this->setAnimationMode(enabled ? vtkMRMLViewNode::Spin : vtkMRMLViewNode::Off);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::rockView(bool enabled)
{
  this->setAnimationMode(enabled ? vtkMRMLViewNode::Rock : vtkMRMLViewNode::Off);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setAnimationMode(int newAnimationMode)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetAnimationMode(newAnimationMode);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::resetFocalPoint()
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ThreeDView)
    {
    return;
    }
  d->ThreeDView->resetFocalPoint();
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::set3DAxisVisible(bool visible)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetBoxVisible(visible);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::set3DAxisLabelVisible(bool visible)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetAxisLabelsVisible(visible);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setUseDepthPeeling(bool use)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetUseDepthPeeling(use ? 1 : 0);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setFPSVisible(bool visible)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetFPSVisible(visible ? 1 : 0);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setLightBlueBackground()
{
  this->setBackgroundColor(QColor::fromRgbF(
    vtkMRMLViewNode::defaultBackgroundColor()[0],
    vtkMRMLViewNode::defaultBackgroundColor()[1],
    vtkMRMLViewNode::defaultBackgroundColor()[2]),
    QColor::fromRgbF(
    vtkMRMLViewNode::defaultBackgroundColor2()[0],
    vtkMRMLViewNode::defaultBackgroundColor2()[1],
    vtkMRMLViewNode::defaultBackgroundColor2()[2]));
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setBlackBackground()
{
  this->setBackgroundColor(Qt::black);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setWhiteBackground()
{
  this->setBackgroundColor(Qt::white);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setBackgroundColor(
  const QColor& newColor, QColor newColor2)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  int wasModifying = d->ViewNode->StartModify();
  // The ThreeDView displayable manager will change the background color of
  // the renderer.
  d->ViewNode->SetBackgroundColor(newColor.redF(), newColor.greenF(), newColor.blueF());
  if (!newColor2.isValid())
    {
    newColor2 = newColor;
    }
  d->ViewNode->SetBackgroundColor2(newColor2.redF(), newColor2.greenF(), newColor2.blueF());
  d->ViewNode->EndModify(wasModifying);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setStereoType(int newStereoType)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetStereoType(newStereoType);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setOrientationMarkerType(int newOrientationMarkerType)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetOrientationMarkerType(newOrientationMarkerType);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setOrientationMarkerSize(int newOrientationMarkerSize)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetOrientationMarkerSize(newOrientationMarkerSize);
}

// --------------------------------------------------------------------------
void qMRMLThreeDViewControllerWidget::setRulerType(int newRulerType)
{
  Q_D(qMRMLThreeDViewControllerWidget);
  if (!d->ViewNode)
    {
    return;
    }
  d->ViewNode->SetRulerType(newRulerType);
  // Switch to orthographic render mode automatically if ruler is enabled
  if (newRulerType!=vtkMRMLViewNode::RulerTypeNone && d->ViewNode->GetRenderMode()!=vtkMRMLViewNode::Orthographic)
    {
    d->ViewNode->SetRenderMode(vtkMRMLViewNode::Orthographic);
    }
}
