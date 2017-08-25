/*==============================================================================

  Copyright (c) Kapteyn Astronomical Institute
  University of Groningen, Groningen, Netherlands. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Davide Punzo, Kapteyn Astronomical Institute,
  and was supported through the European Research Consil grant nr. 291531.

==============================================================================*/

#ifndef __qMRMLPlotViewControllerWidget_p_h
#define __qMRMLPlotViewControllerWidget_p_h

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Slicer API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

// qMRML includes
#include "qMRMLPlotViewControllerWidget.h"
#include "qMRMLViewControllerBar_p.h"
#include "ui_qMRMLPlotViewControllerWidget.h"

// VTK includes
#include <vtkWeakPointer.h>

class QAction;
class qMRMLSceneViewMenu;
class vtkMRMLPlotViewNode;
class vtkMRMLPlotLayoutNode;
class vtkMRMLPlotNode;
class QString;

//-----------------------------------------------------------------------------
class qMRMLPlotViewControllerWidgetPrivate
  : public qMRMLViewControllerBarPrivate
  , public Ui_qMRMLPlotViewControllerWidget
{
  Q_OBJECT
  QVTK_OBJECT
  Q_DECLARE_PUBLIC(qMRMLPlotViewControllerWidget);

public:
  typedef qMRMLViewControllerBarPrivate Superclass;
  qMRMLPlotViewControllerWidgetPrivate(qMRMLPlotViewControllerWidget& object);
  virtual ~qMRMLPlotViewControllerWidgetPrivate();

  virtual void init();

  vtkWeakPointer<vtkMRMLPlotLayoutNode>  PlotLayoutNode;
  vtkWeakPointer<vtkMRMLPlotViewNode>    PlotViewNode;
  qMRMLPlotView*                         PlotView;

  QString                          PlotViewLabel;

  vtkMRMLPlotLayoutNode* GetPlotLayoutNodeFromView();

public slots:
  /// Called after a Plot node is selected
  /// using the associated qMRMLNodeComboBox
  void onPlotLayoutNodeSelected(vtkMRMLNode* node);

  /// Called after an array node is selected using the qMRMLCheckableNodeComboBox
  void onPlotNodesSelected();

  /// Called after a Plot type is selected using the qComboBox
  /// Modify all the PlotNode selected in the PlotLayoutNode
  void onPlotTypeSelected(const QString& Type);

  void updatePlotTypes(const char* Type);

protected:
  virtual void setupPopupUi();

public:

};

#endif
