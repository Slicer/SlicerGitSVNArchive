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

#include "vtkSlicerLineInterpolator.h"

#include "vtkSlicerAbstractRepresentation.h"
#include "vtkIntArray.h"

//----------------------------------------------------------------------
vtkSlicerLineInterpolator::vtkSlicerLineInterpolator() = default;

//----------------------------------------------------------------------
vtkSlicerLineInterpolator::~vtkSlicerLineInterpolator() = default;

//----------------------------------------------------------------------
void vtkSlicerLineInterpolator::GetSpan(int nodeIndex,
                                        vtkIntArray *nodeIndices,
                                        vtkSlicerAbstractRepresentation *rep)
{
  int start = nodeIndex - 1;
  int end   = nodeIndex;
  int index[2];

  // Clear the array
  nodeIndices->Reset();
  nodeIndices->Squeeze();
  nodeIndices->SetNumberOfComponents(2);

  for (int i = 0; i < 3; i++)
    {
    index[0] = start++;
    index[1] = end++;

    if (rep->GetClosedLoop())
      {
      if (index[0] < 0)
        {
        index[0] += rep->GetNumberOfNodes();
        }
      if (index[1] < 0)
        {
        index[1] += rep->GetNumberOfNodes();
        }
      if (index[0] >= rep->GetNumberOfNodes())
        {
        index[0] -= rep->GetNumberOfNodes();
        }
      if (index[1] >= rep->GetNumberOfNodes())
        {
        index[1] -= rep->GetNumberOfNodes();
        }
      }

    if (index[0] >= 0 && index[0] < rep->GetNumberOfNodes() &&
         index[1] >= 0 && index[1] < rep->GetNumberOfNodes())
      {
      nodeIndices->InsertNextTypedTuple(index);
      }
    }
}

//----------------------------------------------------------------------
void vtkSlicerLineInterpolator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
