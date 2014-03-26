#ifndef __vtkHyperStreamlineTeem_h
#define __vtkHyperStreamlineTeem_h

#include "vtkTeemConfigure.h"

#include "vtkHyperStreamline.h"
#include "vtkHyperStreamlineDTMRI.h"
#include "vtkDiffusionTensorMathematics.h"
#include "vtkFloatArray.h"

/* avoid name conflicts with symbols from python */
#undef ECHO
#undef B0

#include "teem/ten.h"
#include "teem/nrrd.h"

class VTK_Teem_EXPORT vtkHyperStreamlineTeem : public vtkHyperStreamlineDTMRI
{

 public:
  vtkTypeRevisionMacro(vtkHyperStreamlineTeem,vtkHyperStreamlineDTMRI);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkHyperStreamlineTeem *New();

 protected:
  vtkHyperStreamlineTeem();
  ~vtkHyperStreamlineTeem();

  void Execute();
  void StartFiberFrom( const vtkFloatingPointType position[3], tenFiberContext *fibercontext );
  void VisualizeFibers( const Nrrd *fibers );

 private:
  tenFiberContext *ProduceFiberContext();
  bool DatasetOrSettingsChanged();
};

#endif
