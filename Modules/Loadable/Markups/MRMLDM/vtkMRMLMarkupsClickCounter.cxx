/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsClickCounter.h"

// VTK includes
#include <vtk/Common/Core/vtkObjectFactory.h>
#include <vtk/Common/Core/vtkSmartPointer.h>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsClickCounter);

//---------------------------------------------------------------------------
void vtkMRMLMarkupsClickCounter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsClickCounter::vtkMRMLMarkupsClickCounter()
{
  this->m_Clicks = 0;
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsClickCounter::~vtkMRMLMarkupsClickCounter()
{
  // TODO Auto-generated destructor stub
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsClickCounter::Reset()
{
  this->m_Clicks = 0;
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsClickCounter::HasEnoughClicks(int clicks)
{
  this->m_Clicks++;

  if (this->m_Clicks==clicks)
    {
      this->Reset();
      return true;
    }

  return false;
}
