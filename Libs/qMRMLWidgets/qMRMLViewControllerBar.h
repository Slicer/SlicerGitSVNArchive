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

#ifndef __qMRMLViewControllerBar_h
#define __qMRMLViewControllerBar_h

// qMRMLWidget includes
#include "qMRMLWidget.h"
#include "qMRMLWidgetsExport.h"
class qMRMLViewControllerBarPrivate;

/// qMRMLViewControllerBar is the base class of all the bars over views.
/// A controller bar typically contains a pin button, a view label to uniquely
/// define a view, a popup widget to control the view and a unique color per
/// type of view.
class QMRML_WIDGETS_EXPORT qMRMLViewControllerBar
  : public qMRMLWidget
{
  Q_OBJECT
public:
  /// Superclass typedef
  typedef qMRMLWidget Superclass;

  /// Constructors
  explicit qMRMLViewControllerBar(QWidget* parent = 0);
  virtual ~qMRMLViewControllerBar();

protected:
  QScopedPointer<qMRMLViewControllerBarPrivate> d_ptr;
  qMRMLViewControllerBar(qMRMLViewControllerBarPrivate* pimpl, QWidget* parent = 0);

private:
  Q_DECLARE_PRIVATE(qMRMLViewControllerBar);
  Q_DISABLE_COPY(qMRMLViewControllerBar);
};

#endif
