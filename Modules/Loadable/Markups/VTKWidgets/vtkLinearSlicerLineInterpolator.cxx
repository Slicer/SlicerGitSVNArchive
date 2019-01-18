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

#include "vtkSlicerAbstractRepresentation.h"
#include "vtkLinearSlicerLineInterpolator.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkLinearSlicerLineInterpolator);

//----------------------------------------------------------------------
vtkLinearSlicerLineInterpolator::vtkLinearSlicerLineInterpolator() = default;

//----------------------------------------------------------------------
vtkLinearSlicerLineInterpolator::~vtkLinearSlicerLineInterpolator() = default;

//----------------------------------------------------------------------
int vtkLinearSlicerLineInterpolator::InterpolateLine(vtkSlicerAbstractRepresentation *rep,
                                                     int idx1, int idx2)
{
  double p1[3] = {0}, p2[3] = {0};
  rep->GetNthNodeWorldPosition(idx1, p1);
  rep->AddIntermediatePointWorldPosition(idx1, p1);
  rep->GetNthNodeWorldPosition(idx2, p2);
  rep->AddIntermediatePointWorldPosition(idx2, p2);
  return 1;
}

//----------------------------------------------------------------------
void vtkLinearSlicerLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

