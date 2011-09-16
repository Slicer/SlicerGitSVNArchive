#ifndef __vtkMRMLEMSTreeParametersParentNode_h
#define __vtkMRMLEMSTreeParametersParentNode_h

#include "vtkMRML.h"
#include "vtkMRMLNode.h"
#include "vtkEMSegment.h"
#include "vtkMRMLScene.h"

class VTK_EMSEGMENT_EXPORT vtkMRMLEMSTreeParametersParentNode : 
  public vtkMRMLNode
{
public:
  static vtkMRMLEMSTreeParametersParentNode *New();
  vtkTypeMacro(vtkMRMLEMSTreeParametersParentNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes(const char** atts);

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "EMSTreeParametersParent";}

  // manipulate target input channels
  vtkGetMacro(NumberOfTargetInputChannels, unsigned int);
  vtkSetMacro(NumberOfTargetInputChannels, unsigned int);
  virtual void AddTargetInputChannel() {}
  virtual void RemoveNthTargetInputChannel(int vtkNotUsed(index)) {}
  virtual void MoveNthTargetInputChannel(int vtkNotUsed(fromIndex), int vtkNotUsed(toIndex)) {}

  // Alpha determines the influence of the Markov random field
  // 0 => no influence, 1 => maximum influence
  vtkGetMacro(Alpha, double);
  vtkSetMacro(Alpha, double);

  //
  // options for inhomogeneity computation
  //

  vtkGetMacro(PrintBias, int);
  vtkSetMacro(PrintBias, int);

  // stop inhomogeneity computation after n iterations; use -1 if no
  // stopping condition is desired
  vtkGetMacro(BiasCalculationMaxIterations, int);
  vtkSetMacro(BiasCalculationMaxIterations, int);

  // smoothing kernel width and sigma for inhomogeneity correction 
  vtkGetMacro(SmoothingKernelWidth, int);
  vtkSetMacro(SmoothingKernelWidth, int);

  vtkGetMacro(SmoothingKernelSigma, double);
  vtkSetMacro(SmoothingKernelSigma, double);

  vtkGetMacro(BiasCorrectionType, double);
  vtkSetMacro(BiasCorrectionType, double);

  vtkGetMacro(UseLLS_Recompute_Means, double);
  vtkSetMacro(UseLLS_Recompute_Means, double);

  // EM stopping conditions
  // Type:
  //   0) fixed number of iterations specified by MaxIterations
  //   1) absolute measure specified by MaxValue
  //   2) relative measure specified by MaxValue
  vtkGetMacro(StopEMType, int);
  vtkSetMacro(StopEMType, int);
  vtkGetMacro(StopEMMaxIterations, int);
  vtkSetMacro(StopEMMaxIterations, int);
  vtkGetMacro(StopEMValue, double);
  vtkSetMacro(StopEMValue, double);

  // MFA stopping conditions
  // Type:
  //   0) fixed number of iterations specified by MaxIterations
  //   1) absolute measure specified by MaxValue
  //   2) relative measure specified by MaxValue
  vtkGetMacro(StopMFAType, int);
  vtkSetMacro(StopMFAType, int);
  vtkGetMacro(StopMFAMaxIterations, int);
  vtkSetMacro(StopMFAMaxIterations, int);
  vtkGetMacro(StopMFAValue, double);
  vtkSetMacro(StopMFAValue, double);

  //
  // printing
  //
  vtkGetMacro(PrintFrequency, int);
  vtkSetMacro(PrintFrequency, int);

  vtkGetMacro(PrintLabelMap, int);
  vtkSetMacro(PrintLabelMap, int);

  vtkGetMacro(PrintEMLabelMapConvergence, int);
  vtkSetMacro(PrintEMLabelMapConvergence, int);

  vtkGetMacro(PrintEMWeightsConvergence, int);
  vtkSetMacro(PrintEMWeightsConvergence, int);

  vtkGetMacro(PrintMFALabelMapConvergence, int);
  vtkSetMacro(PrintMFALabelMapConvergence, int);

  vtkGetMacro(PrintMFAWeightsConvergence, int);
  vtkSetMacro(PrintMFAWeightsConvergence, int);

  vtkGetMacro(MFA2DFlag, int);
  vtkSetMacro(MFA2DFlag, int);

  //
  // misc.
  //
  vtkGetMacro(GenerateBackgroundProbability, int);
  vtkSetMacro(GenerateBackgroundProbability, int);

protected:
  vtkMRMLEMSTreeParametersParentNode();
  ~vtkMRMLEMSTreeParametersParentNode();
  vtkMRMLEMSTreeParametersParentNode(const vtkMRMLEMSTreeParametersParentNode&);
  void operator=(const vtkMRMLEMSTreeParametersParentNode&);

  // Markov field influence
  double                              Alpha;
  
  // inhomogeneity
  int                                 PrintBias;
  int                                 BiasCalculationMaxIterations;
  double                              SmoothingKernelSigma;
  int                                 SmoothingKernelWidth;
  bool                                BiasCorrectionType;
  bool                                UseLLS_Recompute_Means;
  
  // EM stopping conditions
  int                                 StopEMType;
  int                                 StopEMMaxIterations;
  double                              StopEMValue;
  
  // MFA stopping conditions
  int                                 StopMFAType;
  int                                 StopMFAMaxIterations;
  double                              StopMFAValue;

  // printing
  int                                 PrintFrequency;
  int                                 PrintLabelMap;
  int                                 PrintEMLabelMapConvergence;
  int                                 PrintEMWeightsConvergence;
  int                                 PrintMFALabelMapConvergence;
  int                                 PrintMFAWeightsConvergence;

  int                                 MFA2DFlag;

  // misc.
  int                                 GenerateBackgroundProbability;

  unsigned int                        NumberOfTargetInputChannels;
};

#endif
