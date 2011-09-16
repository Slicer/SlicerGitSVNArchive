#include "vtkITKTransformAdapter.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkITKTransformAdapter, "$Revision: 1.30 $");
vtkStandardNewMacro(vtkITKTransformAdapter);

void vtkITKTransformAdapter::PrintSelf(ostream& os, vtkIndent indent) 
{
  Superclass::PrintSelf(os,indent);
  os << "ITKTransform: (not implemented)" 
     << std::endl;
}
