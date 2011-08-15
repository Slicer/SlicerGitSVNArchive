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

#ifndef __qMRMLViewControllerBar_p_h
#define __qMRMLViewControllerBar_p_h

// Qt includes
#include <QObject>
class QLabel;
class QToolButton;

// CTK includes
class ctkPopupWidget;

// qMRML includes
#include "qMRMLViewControllerBar.h"

//-----------------------------------------------------------------------------
class QMRML_WIDGETS_EXPORT qMRMLViewControllerBarPrivate: public QObject
{
  Q_OBJECT
  Q_DECLARE_PUBLIC(qMRMLViewControllerBar);

protected:
  qMRMLViewControllerBar* const q_ptr;

public:
  typedef QObject Superclass;
  qMRMLViewControllerBarPrivate(qMRMLViewControllerBar& object);
  virtual ~qMRMLViewControllerBarPrivate();

  virtual void init();
  void setColor(QColor color);

  QToolButton*                     PinButton;
  QLabel*                          ViewLabel;
  ctkPopupWidget*                  PopupWidget;

  virtual bool eventFilter(QObject* object, QEvent* event);

protected:
  virtual void setupPopupUi();
};

#endif
