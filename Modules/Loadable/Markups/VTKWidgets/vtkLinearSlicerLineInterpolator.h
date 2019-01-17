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
 * @class   vtkLinearSlicerLineInterpolator
 * @brief   Interpolates supplied nodes with line segments
 *
 * The line interpolator interpolates supplied nodes (see InterpolateLine)
 * with line segments. The finess of the curve may be controlled using
 * SetMaximumCurveError and SetMaximumNumberOfLineSegments.
 *
 * @sa
 * vtkSlicerLineInterpolator
*/

#ifndef vtkLinearSlicerLineInterpolator_h
#define vtkLinearSlicerLineInterpolator_h

#include "vtkSlicerMarkupsModuleVTKWidgetsExport.h"
#include "vtkSlicerLineInterpolator.h"

class VTK_SLICER_MARKUPS_MODULE_VTKWIDGETS_EXPORT vtkLinearSlicerLineInterpolator
                          : public vtkSlicerLineInterpolator
{
public:
  /// Instantiate this class.
  static vtkLinearSlicerLineInterpolator *New();

  /// Standard methods for instances of this class.
  vtkTypeMacro(vtkLinearSlicerLineInterpolator,vtkSlicerLineInterpolator);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /// Interpolate the line between two nodes.
  int InterpolateLine( vtkSlicerAbstractRepresentation *rep,
                       int idx1, int idx2 ) VTK_OVERRIDE;

protected:
  vtkLinearSlicerLineInterpolator();
  ~vtkLinearSlicerLineInterpolator() VTK_OVERRIDE;

private:
  vtkLinearSlicerLineInterpolator(const vtkLinearSlicerLineInterpolator&) = delete;
  void operator=(const vtkLinearSlicerLineInterpolator&) = delete;
};

#endif
