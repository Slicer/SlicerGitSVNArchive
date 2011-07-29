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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLSliceViewDisplayableManagerFactory.h"

// VTK includes
#include <vtkDebugLeaks.h>
#include <vtkObjectFactory.h>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMRMLSliceViewDisplayableManagerFactory, "$Revision: 13859 $");

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkMRMLSliceViewDisplayableManagerFactory);

//----------------------------------------------------------------------------
// vtkMRMLSliceViewDisplayableManagerFactory methods

//----------------------------------------------------------------------------
// Up the reference count so it behaves like New
vtkMRMLSliceViewDisplayableManagerFactory* vtkMRMLSliceViewDisplayableManagerFactory::New()
{
  vtkMRMLSliceViewDisplayableManagerFactory* instance = Self::GetInstance();
  instance->Register(0);
  return instance;
}

//----------------------------------------------------------------------------
vtkMRMLSliceViewDisplayableManagerFactory* vtkMRMLSliceViewDisplayableManagerFactory::GetInstance()
{
  if(!Self::Instance)
    {
    // Try the factory first
    Self::Instance = (vtkMRMLSliceViewDisplayableManagerFactory*)
                     vtkObjectFactory::CreateInstance("vtkMRMLSliceViewDisplayableManagerFactory");

    // if the factory did not provide one, then create it here
    if(!Self::Instance)
      {
      // if the factory failed to create the object,
      // then destroy it now, as vtkDebugLeaks::ConstructClass was called
      // with "vtkMRMLSliceViewDisplayableManagerFactory", and not the real name of the class
#ifdef VTK_DEBUG_LEAKS
      vtkDebugLeaks::DestructClass("vtkMRMLSliceViewDisplayableManagerFactory");
#endif
      Self::Instance = new vtkMRMLSliceViewDisplayableManagerFactory;
      }
    }
  // return the instance
  return Self::Instance;
}

//----------------------------------------------------------------------------
vtkMRMLSliceViewDisplayableManagerFactory::
    vtkMRMLSliceViewDisplayableManagerFactory():Superclass()
{
}

//----------------------------------------------------------------------------
vtkMRMLSliceViewDisplayableManagerFactory::~vtkMRMLSliceViewDisplayableManagerFactory()
{
}

//----------------------------------------------------------------------------
void vtkMRMLSliceViewDisplayableManagerFactory::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_SINGLETON_CXX(vtkMRMLSliceViewDisplayableManagerFactory);

