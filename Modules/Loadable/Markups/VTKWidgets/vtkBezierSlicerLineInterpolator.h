/*=========================================================================

 Copyright (c) ProxSim ltd., Kwun Tong, Hong Kong. All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 This file was originally developed by Davide Punzo, punzodavide@hotmail.it,
 and development was supported by ProxSim ltd.

=========================================================================*/

/**
 * @class   vtkBezierSlicerLineInterpolator
 * @brief   Interpolates supplied nodes with bezier line segments
 *
 * The line interpolator interpolates supplied nodes (see InterpolateLine)
 * with Bezier line segments. The fitness of the curve may be controlled using
 * SetMaximumCurveError and SetMaximumNumberOfLineSegments.
 *
 * @sa
 * vtkSlicerLineInterpolator
*/

#ifndef vtkBezierSlicerLineInterpolator_h
#define vtkBezierSlicerLineInterpolator_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerLineInterpolator.h"

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkBezierSlicerLineInterpolator
                          : public vtkSlicerLineInterpolator
{
public:
  /// Instantiate this class.
  static vtkBezierSlicerLineInterpolator *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkBezierSlicerLineInterpolator, vtkSlicerLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Interpolate the line between two nodes.
  int InterpolateLine(vtkSlicerAbstractRepresentation *rep,
                      int idx1, int idx2) VTK_OVERRIDE;

  /// The difference between a line segment connecting two points and the curve
  /// connecting the same points. In the limit of the length of the curve
  /// dx -> 0, the two values will be the same. The smaller this number, the
  /// finer the bezier curve will be interpolated. Default is 0.005
  vtkSetClampMacro(MaximumCurveError, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MaximumCurveError, double);

  /// Maximum number of bezier line segments between two nodes. Larger values
  /// create a finer interpolation. Default is 100.
  vtkSetClampMacro(MaximumCurveLineSegments, int, 1, 1000);
  vtkGetMacro(MaximumCurveLineSegments, int);

  /// Span of the interpolator, i.e. the number of control points it's supposed
  /// to interpolate given a node.
  ///
  /// The first argument is the current nodeIndex.
  /// i.e., you'd be trying to interpolate between nodes "nodeIndex" and
  /// "nodeIndex-1", unless you're closing the Slicer, in which case you're
  /// trying to interpolate "nodeIndex" and "Node=0". The node span is
  /// returned in a vtkIntArray.
  ///
  /// The node span is returned in a vtkIntArray. The node span returned by
  /// this interpolator will be a 2-tuple with a span of 4.
  void GetSpan(int nodeIndex, vtkIntArray *nodeIndices,
               vtkSlicerAbstractRepresentation *rep) VTK_OVERRIDE;

protected:
  vtkBezierSlicerLineInterpolator();
  ~vtkBezierSlicerLineInterpolator() VTK_OVERRIDE;

  void ComputeMidpoint(double p1[3], double p2[3], double mid[3])
  {
      mid[0] = (p1[0] + p2[0])/2;
      mid[1] = (p1[1] + p2[1])/2;
      mid[2] = (p1[2] + p2[2])/2;
  }

  double MaximumCurveError;
  int    MaximumCurveLineSegments;

private:
  vtkBezierSlicerLineInterpolator(const vtkBezierSlicerLineInterpolator&) = delete;
  void operator=(const vtkBezierSlicerLineInterpolator&) = delete;
};

#endif
