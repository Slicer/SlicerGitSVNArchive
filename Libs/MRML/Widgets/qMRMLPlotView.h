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
  and was supported through the European Research Council grant nr. 291531.

==============================================================================*/

#ifndef __qMRMLPlotView_h
#define __qMRMLPlotView_h

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKChartView.h>

#include "qMRMLWidgetsExport.h"

class qMRMLPlotViewPrivate;

// MRML includes
class vtkMRMLPlotViewNode;
class vtkMRMLScene;

// VTK includes
class vtkCollection;
class vtkStringArray;

/// \brief qMRMLPlotView is the display canvas for a Plot.
///
/// qMRMLPlotView supports only 2D plots.
/// For extending this class to 3DPlots it is needed to expand the mother class
/// cktVTKChartView to use also vtkChartXYZ (currently exploiting only vtkChartXY).

class QMRML_WIDGETS_EXPORT qMRMLPlotView : public ctkVTKChartView
{
  Q_OBJECT
public:
  /// Superclass typedef
  typedef ctkVTKChartView Superclass;

  /// Constructors
  explicit qMRMLPlotView(QWidget* parent = 0);
  virtual ~qMRMLPlotView();

  /// Return a pointer on the current MRML scene.
  vtkMRMLScene* mrmlScene() const;

  /// Get the PlotView node observed by view.
  vtkMRMLPlotViewNode* mrmlPlotViewNode()const;

  /// Redefine the sizeHint so layouts work properly.
  virtual QSize sizeHint() const;

public slots:

  /// Set the MRML \a scene that should be listened for events.
  void setMRMLScene(vtkMRMLScene* newScene);

  /// Set the current \a viewNode to observe.
  void setMRMLPlotViewNode(vtkMRMLPlotViewNode* newPlotViewNode);

  /// Change axis limits to show all content.
  void fitToContent();

signals:

  /// When designing custom qMRMLWidget in the designer, you can connect the
  /// mrmlSceneChanged signal directly to the aggregated MRML widgets that
  /// have a setMRMLScene slot.
  void mrmlSceneChanged(vtkMRMLScene*);

  /// Signal emitted when a data point or more has been selected. Returns
  /// the MRMLPlotSeriesNodes IDs and the correspective arrays with
  /// the data points ids (vtkIdTypeArray).
  void dataSelected(vtkStringArray* mrmlPlotSeriesIDs, vtkCollection* selectionCol);

protected:
  QScopedPointer<qMRMLPlotViewPrivate> d_ptr;

  /// Handle keyboard events
  virtual void keyPressEvent(QKeyEvent* event);

  virtual void keyReleaseEvent(QKeyEvent* event);

private:
  Q_DECLARE_PRIVATE(qMRMLPlotView);
  Q_DISABLE_COPY(qMRMLPlotView);
};

#endif
