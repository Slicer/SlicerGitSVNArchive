
#include "vtkXNATPermissionPrompter.h"
#include "vtk/Common/Core/vtkCallbackCommand.h"


vtkStandardNewMacro ( vtkXNATPermissionPrompter );

//----------------------------------------------------------------------------
vtkXNATPermissionPrompter::vtkXNATPermissionPrompter()
{
  this->Host = NULL;
}


//----------------------------------------------------------------------------
vtkXNATPermissionPrompter::~vtkXNATPermissionPrompter()
{
  if ( this->Host )
    {
    delete [] this->Host;
    }
}


//----------------------------------------------------------------------------
void vtkXNATPermissionPrompter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf ( os, indent );
  os << indent << "Host: " << (this->Host ? this->Host : "(none)") << "\n";
}
