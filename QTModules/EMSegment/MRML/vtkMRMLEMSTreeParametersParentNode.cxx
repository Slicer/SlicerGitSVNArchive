#include "vtkMRMLEMSTreeParametersParentNode.h"
#include <sstream>
#include "vtkMRMLScene.h"
#include <vtkObjectFactory.h>

#include <vtksys/ios/sstream>

//-----------------------------------------------------------------------------
vtkMRMLEMSTreeParametersParentNode* 
vtkMRMLEMSTreeParametersParentNode::
New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkMRMLEMSTreeParametersParentNode");
  if(ret)
    {
    return (vtkMRMLEMSTreeParametersParentNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLEMSTreeParametersParentNode;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* 
vtkMRMLEMSTreeParametersParentNode::
CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkMRMLEMSTreeParametersParentNode");
  if(ret)
    {
    return (vtkMRMLEMSTreeParametersParentNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLEMSTreeParametersParentNode;
}

//-----------------------------------------------------------------------------
vtkMRMLEMSTreeParametersParentNode::vtkMRMLEMSTreeParametersParentNode()
{
  this->Alpha                         = 0.99;

  this->PrintBias                     = 0;
  this->BiasCalculationMaxIterations  = -1;
  this->SmoothingKernelWidth          = 11;
  this->SmoothingKernelSigma          = 5.0;
  this->BiasCorrectionType            = 0;
  this->UseLLS_Recompute_Means        = 0;

  this->StopEMType                    = 0;
  this->StopEMMaxIterations           = 4;
  this->StopEMValue                   = 0.0;

  this->StopMFAType                   = 0;
  this->StopMFAMaxIterations          = 2;
  this->StopMFAValue                  = 0.0;

  this->PrintFrequency                = 0;
  this->PrintLabelMap                 = 0;
  this->PrintEMLabelMapConvergence    = 0;
  this->PrintEMWeightsConvergence     = 0;
  this->PrintMFALabelMapConvergence   = 0;
  this->PrintMFAWeightsConvergence    = 0;
  this->MFA2DFlag = 0;

  this->GenerateBackgroundProbability = 0;

  this->NumberOfTargetInputChannels   = 0;
}

//-----------------------------------------------------------------------------
vtkMRMLEMSTreeParametersParentNode::~vtkMRMLEMSTreeParametersParentNode()
{
}

//-----------------------------------------------------------------------------
void vtkMRMLEMSTreeParametersParentNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);

  of << indent << "Alpha=\"" << this->Alpha << "\" ";

  of << indent << "PrintBias=\"" << this->PrintBias 
     << "\" ";
  of << indent << "BiasCalculationMaxIterations=\"" 
     << this->BiasCalculationMaxIterations
     << "\" ";
  of << indent << "SmoothingKernelWidth=\"" << this->SmoothingKernelWidth 
     << "\" ";
  of << indent << "SmoothingKernelSigma=\"" << this->SmoothingKernelSigma
     << "\" ";
  of << indent << "BiasCorrectionType=\"" << this->BiasCorrectionType
     << "\" ";
  of << indent << "UseLLS_Recompute_Means=\"" << this->UseLLS_Recompute_Means
     << "\" ";


  of << indent << "StopEMType=\"" << this->StopEMType << "\" ";
  of << indent << "StopEMMaxIterations=\"" << this->StopEMMaxIterations 
     << "\" ";
  of << indent << "StopEMValue=\""<< this->StopEMValue << "\" ";

  of << indent << "StopMFAType=\"" << this->StopMFAType << "\" ";
  of << indent << "StopMFAMaxIterations=\"" << this->StopMFAMaxIterations 
     << "\" ";
  of << indent << "StopMFAValue=\"" << this->StopMFAValue << "\" ";

  of << indent << "PrintFrequency=\"" << this->PrintFrequency << "\" ";

  of << indent << "PrintLabelMap=\"" << this->PrintLabelMap << "\" ";

  of << indent << "PrintEMLabelMapConvergence=\"" 
     << this->PrintEMLabelMapConvergence << "\" ";

  of << indent << "PrintEMWeightsConvergence=\"" 
     << this->PrintEMWeightsConvergence << "\" ";

  of << indent << "PrintMFALabelMapConvergence=\"" 
     << this->PrintEMLabelMapConvergence << "\" ";

  of << indent << "PrintMFAWeightsConvergence=\"" 
     << this->PrintEMWeightsConvergence << "\" ";

  of << indent << "MFA2DFlag=\"" << this->MFA2DFlag << "\" ";

  of << indent << "GenerateBackgroundProbability=\"" 
     << this->GenerateBackgroundProbability << "\" ";
}

//-----------------------------------------------------------------------------
void vtkMRMLEMSTreeParametersParentNode::ReadXMLAttributes(const char** attrs)
{
  Superclass::ReadXMLAttributes(attrs);

  //
  // we assume and even number of attrs
  //
  const char* key;
  const char* val;
  while (*attrs != NULL)
    {
    key = *attrs++;
    val = *attrs++;

    if (!strcmp(key, "Alpha"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->Alpha;
      }
    else if (!strcmp(key, "PrintBias"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->PrintBias;
      }
    else if (!strcmp(key, "BiasCalculationMaxIterations"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->BiasCalculationMaxIterations;
      }
    else if (!strcmp(key, "SmoothingKernelWidth"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->SmoothingKernelWidth;
      }
    else if (!strcmp(key, "SmoothingKernelSigma"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->SmoothingKernelSigma;
      }
    else if (!strcmp(key, "BiasCorrectionType"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->BiasCorrectionType;
      std::cout << "BiasCorrectionType: " << this->BiasCorrectionType << std::endl;
      }
    else if (!strcmp(key, "UseLLS_Recompute_Means"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->UseLLS_Recompute_Means;
      std::cout << "UseLLS_Recompute_Means: " << this->UseLLS_Recompute_Means << std::endl;
      }
    else if (!strcmp(key, "StopEMType"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->StopEMType;
      }
    else if (!strcmp(key, "StopEMMaxIterations"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->StopEMMaxIterations;
      }
    else if (!strcmp(key, "StopEMValue"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->StopEMValue;
      }
    else if (!strcmp(key, "StopMFAType"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->StopMFAType;
      }
    else if (!strcmp(key, "StopMFAMaxIterations"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->StopMFAMaxIterations;
      }
    else if (!strcmp(key, "StopMFAValue"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->StopMFAValue;
      }
    else if (!strcmp(key, "PrintFrequency"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->PrintFrequency;
      }
    else if (!strcmp(key, "PrintLabelMap"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->PrintLabelMap;
      }
    else if (!strcmp(key, "PrintEMLabelMapConvergence"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->PrintEMLabelMapConvergence;
      }
    else if (!strcmp(key, "PrintEMWeightsConvergence"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->PrintEMWeightsConvergence;
      }
    else if (!strcmp(key, "PrintMFALabelMapConvergence"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->PrintMFALabelMapConvergence;
      }
    else if (!strcmp(key, "PrintMFAWeightsConvergence"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->PrintMFAWeightsConvergence;
      }
    else if (!strcmp(key, "MFA2DFlag"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->MFA2DFlag;
      }
    else if (!strcmp(key, "GenerateBackgroundProbability"))
      {
      vtksys_stl::stringstream ss;
      ss << val;
      ss >> this->GenerateBackgroundProbability;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkMRMLEMSTreeParametersParentNode::Copy(vtkMRMLNode *rhs)
{
  Superclass::Copy(rhs);
  vtkMRMLEMSTreeParametersParentNode* node = 
    (vtkMRMLEMSTreeParametersParentNode*) rhs;

  this->SetAlpha(node->Alpha);

  this->SetPrintBias(node->PrintBias);
  this->SetBiasCalculationMaxIterations(node->BiasCalculationMaxIterations);
  this->SetSmoothingKernelWidth(node->SmoothingKernelWidth);
  this->SetSmoothingKernelSigma(node->SmoothingKernelSigma);
  this->SetBiasCorrectionType(node->BiasCorrectionType);
  this->SetUseLLS_Recompute_Means(node->UseLLS_Recompute_Means);

  this->SetStopEMType(node->StopEMType);
  this->SetStopEMMaxIterations(node->StopEMMaxIterations);
  this->SetStopEMValue(node->StopEMValue);

  this->SetStopMFAType(node->StopMFAType);
  this->SetStopMFAMaxIterations(node->StopMFAMaxIterations);
  this->SetStopMFAValue(node->StopMFAValue);
  
  this->SetPrintFrequency(node->PrintFrequency);
  this->SetPrintLabelMap(node->PrintLabelMap);
  this->SetPrintEMLabelMapConvergence(node->PrintEMLabelMapConvergence);
  this->SetPrintEMWeightsConvergence(node->PrintEMWeightsConvergence);
  this->SetPrintMFALabelMapConvergence(node->PrintMFALabelMapConvergence);
  this->SetPrintMFAWeightsConvergence(node->PrintMFAWeightsConvergence);
  this->SetMFA2DFlag(node->MFA2DFlag);
  this->SetGenerateBackgroundProbability(node->GenerateBackgroundProbability);
}

//-----------------------------------------------------------------------------
void vtkMRMLEMSTreeParametersParentNode::PrintSelf(ostream& os, 
                                                   vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);

  os << indent << "Alpha: " << this->Alpha << "\n";

  os << indent << "PrintBias: " << this->PrintBias 
     << "\n";
  os << indent << "BiasCalculationMaxIterations: " 
     << this->BiasCalculationMaxIterations 
     << "\n";
  os << indent << "SmoothingKernelWidth: " << this->SmoothingKernelWidth 
     << "\n";
  os << indent << "SmoothingKernelSigma: " << this->SmoothingKernelSigma 
     << "\n";
  os << indent << "BiasCorrectionType: " << this->BiasCorrectionType
     << "\n";
  os << indent << "UseLLS_Recompute_Means: " << this->UseLLS_Recompute_Means
     << "\n";

  os << indent << "StopEMType: " << this->StopEMType << "\n";
  os << indent << "StopEMMaxIterations: " << this->StopEMMaxIterations << "\n";
  os << indent << "StopEMValue: " << this->StopEMValue << "\n";

  os << indent << "StopMFAType: " << this->StopMFAType << "\n";
  os << indent << "StopMFAMaxIterations: " << this->StopMFAMaxIterations 
     << "\n";
  os << indent << "StopMFAValue: " << this->StopMFAValue << "\n";

  os << indent << "PrintFrequency: " << this->PrintFrequency << "\n";
  os << indent << "PrintLabelMap: " << this->PrintLabelMap << "\n";
  os << indent << "PrintEMLabelMapConvergence: " 
     << this->PrintEMLabelMapConvergence << "\n";
  os << indent << "PrintEMWeightsConvergence: " 
     << this->PrintEMWeightsConvergence << "\n";
  os << indent << "PrintMFALabelMapConvergence: " 
     << this->PrintEMLabelMapConvergence << "\n";
  os << indent << "PrintMFAWeightsConvergence: " 
     << this->PrintEMWeightsConvergence << "\n";
  os << indent << "MFA2DFlag: " 
     << this->MFA2DFlag << "\n";
  os << indent << "GenerateBackgroundProbability: " 
     << this->GenerateBackgroundProbability << "\n";
}

