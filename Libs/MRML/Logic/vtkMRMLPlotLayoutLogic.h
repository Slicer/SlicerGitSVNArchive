///
///  vtkMRMLPlotLayoutLogic.cxx - slicer logic class for handling the observations
///  of PlotLayout Nodes to Plot Nodes

#ifndef __vtkMRMLPlotLayoutLogic_h
#define __vtkMRMLPlotLayoutLogic_h

// MRMLLogic includes
#include "vtkMRMLAbstractLogic.h"


class VTK_MRML_LOGIC_EXPORT vtkMRMLPlotLayoutLogic : public vtkMRMLAbstractLogic
{
public:
  static vtkMRMLPlotLayoutLogic *New();
  vtkTypeMacro(vtkMRMLPlotLayoutLogic,vtkMRMLAbstractLogic);

  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMRMLPlotLayoutLogic();
  virtual ~vtkMRMLPlotLayoutLogic();

  // On a change in scene, we need to manage the observations.
  virtual void SetMRMLSceneInternal(vtkMRMLScene * newScene);
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);

private:
  vtkMRMLPlotLayoutLogic(const vtkMRMLPlotLayoutLogic&);
  void operator=(const vtkMRMLPlotLayoutLogic&);

};

#endif
