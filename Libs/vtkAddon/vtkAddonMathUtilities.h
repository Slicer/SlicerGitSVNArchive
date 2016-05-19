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

/// \brief vtkAddonMathUtilities.
///
///

#ifndef __vtkAddonMathUtilities_h
#define __vtkAddonMathUtilities_h

#include <vtkAddon.h>
#include <vtkObject.h>

class vtkMatrix4x4;

class VTK_ADDON_EXPORT vtkAddonMathUtilities : public vtkObject
{
public:
  static vtkAddonMathUtilities *New();
  vtkTypeMacro(vtkAddonMathUtilities,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  static bool Matrix4x4AreEqual(const vtkMatrix4x4* m1,
                                const vtkMatrix4x4* m2,
                                double tolerance = 1e-3);

protected:
  vtkAddonMathUtilities();
  ~vtkAddonMathUtilities();

private:
  vtkAddonMathUtilities(const vtkAddonMathUtilities&);  // Not implemented.
  void operator=(const vtkAddonMathUtilities&);  // Not implemented.
};

#endif
