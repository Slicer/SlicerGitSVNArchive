/*=auto==============================================================================

  Program: 3D Slicer

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

===============================================================================auto=*/

#include <vtkAddonMathUtilities.h>
#include <vtkMath.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <math.h>

vtkStandardNewMacro(vtkAddonMathUtilities);

//----------------------------------------------------------------------------
vtkAddonMathUtilities::vtkAddonMathUtilities()
{
}

//----------------------------------------------------------------------------
vtkAddonMathUtilities::~vtkAddonMathUtilities()
{
}

//----------------------------------------------------------------------------
void vtkAddonMathUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);  
}

bool vtkAddonMathUtilities::Matrix4x4AreEqual(const vtkMatrix4x4* m1, const vtkMatrix4x4* m2, double tolerance)
{
  int i,j;
  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      if ( m1->GetElement(i, j) - m2->GetElement(i, j) > tolerance )
        {
        return 0;
        }
      }
    }
    return 1;
}
