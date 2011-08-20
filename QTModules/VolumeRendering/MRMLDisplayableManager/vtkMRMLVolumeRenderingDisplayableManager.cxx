

// Slicer includes
#include "vtkImageGradientMagnitude.h"
#include "vtkMRMLVolumeRenderingDisplayableManager.h"
#include "vtkSlicerFixedPointVolumeRayCastMapper.h"
#include "vtkSlicerGPURayCastMultiVolumeMapper.h"
#include "vtkSlicerGPURayCastVolumeMapper.h"
#include "vtkSlicerVolumeRenderingLogic.h"
#include "vtkSlicerVolumeTextureMapper3D.h"

// MRML includes
#include "vtkEventBroker.h"
#include "vtkMRMLAnnotationROINode.h"
#include "vtkMRMLLabelMapVolumeDisplayNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLSliceLogic.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLViewNode.h"
#include "vtkMRMLVolumePropertyNode.h"
#include "vtkMRMLVolumePropertyStorageNode.h"
#include "vtkMRMLVolumeRenderingDisplayNode.h"
#include "vtkMRMLVolumeRenderingScenarioNode.h"

// VTK includes
#include <vtkAbstractTransform.h>
#include <vtkCallbackCommand.h>
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkInteractorStyle.h"
#include "vtkLookupTable.h"
#include "vtkMatrix4x4.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkVolumeProperty.h"

// ITKSys includes
//#include <itksys/SystemTools.hxx> 
//#include <itksys/Directory.hxx> 

// STD includes
#include <cassert>
#include <cmath>
#include <algorithm> // for std::min

// Convenient macro
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLVolumeRenderingDisplayableManager);
vtkCxxRevisionMacro (vtkMRMLVolumeRenderingDisplayableManager, "$Revision: 1.0 $");


bool vtkMRMLVolumeRenderingDisplayableManager::First = true;
int vtkMRMLVolumeRenderingDisplayableManager::DefaultGPUMemorySize = 256;

vtkMRMLVolumeRenderingDisplayableManager::vtkMRMLVolumeRenderingDisplayableManager()
{
  //create instances of mappers
  this->MapperTexture = vtkSlicerVolumeTextureMapper3D::New();

  this->MapperGPURaycast = vtkSlicerGPURayCastVolumeMapper::New();

  this->MapperGPURaycastII = vtkSlicerGPURayCastMultiVolumeMapper::New();

  //this->MapperRaycast = vtkSlicerFixedPointVolumeRayCastMapper::New();
  this->MapperRaycast = vtkFixedPointVolumeRayCastMapper::New();
  this->MapperGPURaycast3 = vtkGPUVolumeRayCastMapper::New();

  //create instance of the actor
  this->Volume = vtkVolume::New();

  this->VolumeRenderingLogic =  vtkSlicerVolumeRenderingLogic::New();;


  //this->Histograms = vtkKWHistogramSet::New();
  //this->HistogramsFg = vtkKWHistogramSet::New();

  this->VolumePropertyGPURaycastII = NULL;
  //this->VolumePropertyGPURaycast3 = NULL;

  this->DisplayedNode = NULL;

  this->ProcessingMRMLFlag = 0;
  this->UpdatingFromMRML = 0;

}

vtkMRMLVolumeRenderingDisplayableManager::~vtkMRMLVolumeRenderingDisplayableManager()
{

  if (this->VolumeRenderingLogic)
  {
    this->VolumeRenderingLogic->Delete();
    this->VolumeRenderingLogic = NULL;
  }

  //delete instances
  if (this->MapperTexture)
  {
    this->MapperTexture->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperTexture->Delete();
    this->MapperTexture = NULL;
  }
  if (this->MapperGPURaycast)
  {
    this->MapperGPURaycast->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperGPURaycast->Delete();
    this->MapperGPURaycast = NULL;
  }
  if (this->MapperGPURaycastII)
  {
    this->MapperGPURaycastII->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperGPURaycastII->Delete();
    this->MapperGPURaycastII = NULL;
  }
  if (this->MapperRaycast)
  {
    this->MapperRaycast->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperRaycast->RemoveObservers(vtkCommand::ProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperRaycast->Delete();
    this->MapperRaycast = NULL;
  }

  if (this->MapperGPURaycast3)
  {
    this->MapperGPURaycast3->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperGPURaycast3->Delete();
    this->MapperGPURaycast3 = NULL;
  }

  if (this->Volume)
  {
    this->Volume->Delete();
    this->Volume = NULL;
  }
  /**
  if(this->Histograms != NULL)
  {
    this->Histograms->RemoveAllHistograms();
    this->Histograms->Delete();
    this->Histograms = NULL;
  }
  if(this->HistogramsFg != NULL)
  {
    this->HistogramsFg->RemoveAllHistograms();
    this->HistogramsFg->Delete();
    this->HistogramsFg = NULL;
  }
  **/
  if (this->VolumePropertyGPURaycastII != NULL)
  {
    this->VolumePropertyGPURaycastII->Delete();
    this->VolumePropertyGPURaycastII = NULL;
  }
  //if (this->VolumePropertyGPURaycast3 != NULL)
  //{
  //  this->VolumePropertyGPURaycast3->Delete();
  //  this->VolumePropertyGPURaycast3 = NULL;
  //}
}

void vtkMRMLVolumeRenderingDisplayableManager::PrintSelf(std::ostream &os, vtkIndent indent)
{
  os<<indent<<"Print logic"<<endl;
}


void vtkMRMLVolumeRenderingDisplayableManager::SetGUICallbackCommand(vtkCommand* callback)
{
  this->MapperTexture->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);

  //cpu ray casting
  this->MapperRaycast->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);
  this->MapperRaycast->AddObserver(vtkCommand::ProgressEvent,callback);

  //hook up the gpu mapper
  this->MapperGPURaycast->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);

  this->MapperGPURaycastII->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);
  this->MapperGPURaycast3->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);

  //this->GetMRMLCallbackCommand() = callback;
}

void vtkMRMLVolumeRenderingDisplayableManager::Reset()
{
  //delete instances
  if (this->MapperTexture)
  {
    this->MapperTexture->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperTexture->Delete();
    this->MapperTexture = NULL;
  }
  if (this->MapperGPURaycast)
  {
    this->MapperGPURaycast->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperGPURaycast->Delete();
    this->MapperGPURaycast = NULL;
  }
  if (this->MapperGPURaycastII)
  {
    this->MapperGPURaycastII->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperGPURaycastII->Delete();
    this->MapperGPURaycastII = NULL;
  }
  if (this->MapperRaycast)
  {
    this->MapperRaycast->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperRaycast->RemoveObservers(vtkCommand::ProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperRaycast->Delete();
    this->MapperRaycast = NULL;
  }
  if (this->MapperGPURaycast3)
  {
    this->MapperGPURaycast3->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
    this->MapperGPURaycast3->Delete();
    this->MapperGPURaycast3 = NULL;
  }
  if (this->Volume)
  {
    this->Volume->Delete();
    this->Volume = NULL;
  }
  /**
  if(this->Histograms != NULL)
  {
    this->Histograms->RemoveAllHistograms();
    this->Histograms->Delete();
    this->Histograms = NULL;
  }
  if(this->HistogramsFg != NULL)
  {
    this->HistogramsFg->RemoveAllHistograms();
    this->HistogramsFg->Delete();
    this->HistogramsFg = NULL;
  }
  ***/
  if (this->VolumePropertyGPURaycastII != NULL)
  {
    this->VolumePropertyGPURaycastII->Delete();
    this->VolumePropertyGPURaycastII = NULL;
  }
  //if (this->VolumePropertyGPURaycast3 != NULL)
  //{
  //  this->VolumePropertyGPURaycast3->Delete();
  //  this->VolumePropertyGPURaycast3 = NULL;
  //}
  
  //create instances of mappers
  this->MapperTexture = vtkSlicerVolumeTextureMapper3D::New();

  this->MapperGPURaycast = vtkSlicerGPURayCastVolumeMapper::New();

  this->MapperGPURaycastII = vtkSlicerGPURayCastMultiVolumeMapper::New();

  this->MapperRaycast = vtkFixedPointVolumeRayCastMapper::New();

  this->MapperGPURaycast3 = vtkGPUVolumeRayCastMapper::New();
  
  //create instance of the actor
  this->Volume = vtkVolume::New();

  //this->Histograms = vtkKWHistogramSet::New();
  //this->HistogramsFg = vtkKWHistogramSet::New();

  this->MapperTexture->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());

  //cpu ray casting
  this->MapperRaycast->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
  this->MapperRaycast->AddObserver(vtkCommand::ProgressEvent, this->GetMRMLCallbackCommand());

  //hook up the gpu mapper
  this->MapperGPURaycast->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());

  this->MapperGPURaycastII->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
  this->MapperGPURaycast3->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GetMRMLCallbackCommand());
}


void vtkMRMLVolumeRenderingDisplayableManager::SetupHistograms(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData();
  if (input == NULL)
    {
    return;
    }
/***
  //-----------------------------------------
  //  remove old histogram
  //-----------------------------------------
  if(this->Histograms != NULL)
  {
    this->Histograms->RemoveAllHistograms();
    this->Histograms->Delete();
    this->Histograms = vtkKWHistogramSet::New();
  }

  //setup histograms
  this->Histograms->AddHistograms(input->GetPointData()->GetScalars());

  //gradient histogram
  vtkImageGradientMagnitude *grad = vtkImageGradientMagnitude::New();
  grad->SetDimensionality(3);
  grad->SetInput(input);
  grad->Update();

  vtkKWHistogram *gradHisto = vtkKWHistogram::New();
  gradHisto->BuildHistogram(grad->GetOutput()->GetPointData()->GetScalars(), 0);
  this->Histograms->AddHistogram(gradHisto, "0gradient");

  grad->Delete();
  gradHisto->Delete();
****/
}

void vtkMRMLVolumeRenderingDisplayableManager::SetupHistogramsFg(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetFgVolumeNode())->GetImageData();
  if (input == NULL)
    {
    return;
    }
/****
  //-----------------------------------------
  //  remove old histogram
  //-----------------------------------------
  if(this->HistogramsFg != NULL)
  {
    this->HistogramsFg->RemoveAllHistograms();
    this->HistogramsFg->Delete();
    this->HistogramsFg = vtkKWHistogramSet::New();
  }

  //setup histograms
  this->HistogramsFg->AddHistograms(input->GetPointData()->GetScalars());

  //gradient histogram
  vtkImageGradientMagnitude *grad = vtkImageGradientMagnitude::New();
  grad->SetDimensionality(3);
  grad->SetInput(input);
  grad->Update();

  vtkKWHistogram *gradHisto = vtkKWHistogram::New();
  gradHisto->BuildHistogram(grad->GetOutput()->GetPointData()->GetScalars(), 0);
  this->HistogramsFg->AddHistogram(gradHisto, "0gradient");

  grad->Delete();
  gradHisto->Delete();
  ****/
}


void vtkMRMLVolumeRenderingDisplayableManager::ComputeInternalVolumeSize(int memory)
{
  if (memory <= 128)
    {
    this->MapperGPURaycast->SetInternalVolumeSize(200);
    this->MapperGPURaycastII->SetInternalVolumeSize(200);
    this->MapperGPURaycast3->SetMaxMemoryInBytes(128*1024*1024);
    this->MapperTexture->SetInternalVolumeSize(128);//has to be power-of-two in this mapper
    }
  else if (memory <= 256)
    {
    this->MapperGPURaycast->SetInternalVolumeSize(256);//256^3
    this->MapperGPURaycastII->SetInternalVolumeSize(320);
    this->MapperGPURaycast3->SetMaxMemoryInBytes(256*1024*1024);
    this->MapperTexture->SetInternalVolumeSize(256);
    }
  else if (memory <= 512)
    {
    this->MapperGPURaycast->SetInternalVolumeSize(320);
    this->MapperGPURaycastII->SetInternalVolumeSize(500);
    this->MapperGPURaycast3->SetMaxMemoryInBytes(512*1024*1024);
    this->MapperTexture->SetInternalVolumeSize(256);
    }
  else if (memory <= 1024)
    {
    this->MapperGPURaycast->SetInternalVolumeSize(400);
    this->MapperGPURaycastII->SetInternalVolumeSize(620);
    this->MapperGPURaycast3->SetMaxMemoryInBytes(1024*1024*1024);
    this->MapperTexture->SetInternalVolumeSize(256);
    }
  else if (memory <= 1536)
    {
    this->MapperGPURaycast->SetInternalVolumeSize(460);
    this->MapperGPURaycastII->SetInternalVolumeSize(700);
    this->MapperGPURaycast3->SetMaxMemoryInBytes(1536*1024*1024);
    this->MapperTexture->SetInternalVolumeSize(256);
    }
  else if (memory <= 2048)
    {
    this->MapperGPURaycast->SetInternalVolumeSize(512);
    this->MapperGPURaycastII->SetInternalVolumeSize(775);
    this->MapperGPURaycast3->SetMaxMemoryInBytes(2047*1024*1024);
    this->MapperTexture->SetInternalVolumeSize(512);
    }
  else if (memory <= 3072)
    {
    this->MapperGPURaycast->SetInternalVolumeSize(700);
    this->MapperGPURaycastII->SetInternalVolumeSize(900);
//    this->MapperGPURaycast3->SetMaxMemoryInBytes(3071*1024*1024);
    this->MapperGPURaycast3->SetMaxMemoryInBytes(2047*1024*1024);
    this->MapperTexture->SetInternalVolumeSize(512);
    }
  else if (memory <= 4096)
    {
    this->MapperGPURaycast->SetInternalVolumeSize(800);
    this->MapperGPURaycastII->SetInternalVolumeSize(1000);
//    this->MapperGPURaycast3->SetMaxMemoryInBytes(4095*1024*1024);
    this->MapperGPURaycast3->SetMaxMemoryInBytes(2047*1024*1024);
    this->MapperTexture->SetInternalVolumeSize(512);
    }
}

void vtkMRMLVolumeRenderingDisplayableManager::CalculateMatrix(vtkMRMLVolumeRenderingDisplayNode *vspNode, vtkMatrix4x4 *output)
{
  //Update matrix
  //Check for NUll Pointer

  vtkMRMLTransformNode *tmp = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetParentTransformNode();
  //check if we have a TransformNode
  if(tmp == NULL)
  {
    vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetIJKToRASMatrix(output);
    return;
  }

  //IJK to ras
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetIJKToRASMatrix(matrix);

  // Parent transforms
  vtkMatrix4x4   *transform = vtkMatrix4x4::New();
  tmp->GetMatrixTransformToWorld(transform);

  //Transform world to ras
  vtkMatrix4x4::Multiply4x4(transform, matrix, output);

  matrix->Delete();
  transform->Delete();
}

void vtkMRMLVolumeRenderingDisplayableManager::SetExpectedFPS(double fps)
{
  fps = std::max(fps, 0.001);

  this->MapperTexture->SetFramerate(fps);
  this->MapperGPURaycast->SetFramerate(fps);
  this->MapperGPURaycastII->SetFramerate(fps);
}

void vtkMRMLVolumeRenderingDisplayableManager::SetCPURaycastParameters(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  switch(vspNode->GetRaycastTechnique())
    {
    case vtkMRMLVolumeRenderingDisplayNode::MaximumIntensityProjection:
      this->MapperRaycast->SetBlendMode(vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND);
      break;
    case vtkMRMLVolumeRenderingDisplayNode::MinimumIntensityProjection:
      this->MapperRaycast->SetBlendMode(vtkVolumeMapper::MINIMUM_INTENSITY_BLEND);
      break;
    case vtkMRMLVolumeRenderingDisplayNode::Composite:
    default:
      this->MapperRaycast->SetBlendMode(vtkVolumeMapper::COMPOSITE_BLEND);
    }
}

void vtkMRMLVolumeRenderingDisplayableManager::SetGPURaycastParameters(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  this->MapperGPURaycast->SetDepthPeelingThreshold(vspNode->GetDepthPeelingThreshold());
  this->MapperGPURaycast->SetDistanceColorBlending(vspNode->GetDistanceColorBlending());
  this->MapperGPURaycast->SetICPEScale(vspNode->GetICPEScale());
  this->MapperGPURaycast->SetICPESmoothness(vspNode->GetICPESmoothness());
  this->MapperGPURaycast->SetTechnique(vspNode->GetRaycastTechnique());
}

void vtkMRMLVolumeRenderingDisplayableManager::SetGPURaycastIIParameters(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  //ratio may not be used depending on techniques selected
  this->MapperGPURaycastII->SetFgBgRatio(vspNode->GetBgFgRatio());
  this->MapperGPURaycastII->SetTechniques(
    vspNode->GetRaycastTechnique(), vspNode->GetRaycastTechniqueFg());
  this->MapperGPURaycastII->SetColorOpacityFusion(vspNode->GetMultiVolumeFusionMethod());
}

void vtkMRMLVolumeRenderingDisplayableManager::SetGPURaycast3Parameters(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  switch(vspNode->GetRaycastTechnique())
    {
    case vtkMRMLVolumeRenderingDisplayNode::MaximumIntensityProjection:
      this->MapperGPURaycast3->SetBlendMode(vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND);
      break;
    case vtkMRMLVolumeRenderingDisplayNode::MinimumIntensityProjection:
      this->MapperGPURaycast3->SetBlendMode(vtkVolumeMapper::MINIMUM_INTENSITY_BLEND);
      break;
    case vtkMRMLVolumeRenderingDisplayNode::Composite:
    default:
      this->MapperGPURaycast3->SetBlendMode(vtkVolumeMapper::COMPOSITE_BLEND);
      break;
    }
}

void vtkMRMLVolumeRenderingDisplayableManager::EstimateSampleDistance(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (vspNode == 0 || vspNode->GetVolumeNode() == 0)
  {
    return;
  }
  double *spacing = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetSpacing();

  if (spacing)
  {
    double minSpace = spacing[0];
    double maxSpace = spacing[0];

    for(int i = 1; i < 3; i++)
    {
      if (spacing[i] > maxSpace)
        maxSpace = spacing[i];
      if (spacing[i] < minSpace)
        minSpace = spacing[i];
    }

    vspNode->SetEstimatedSampleDistance(minSpace * 0.5f);
  }
  else
    vspNode->SetEstimatedSampleDistance( 1.0f);
}

int vtkMRMLVolumeRenderingDisplayableManager::IsCurrentMapperSupported(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (vspNode == NULL)
    return 0;

  vtkRenderWindow* window = this->GetRenderer()->GetRenderWindow();

  switch(vspNode->GetCurrentVolumeMapper())//mapper specific initialization
  {
  case 0:
    return 1;
  case 3:
    {
      vtkSlicerGPURayCastVolumeMapper* MapperGPURaycast = vtkSlicerGPURayCastVolumeMapper::New();

      MapperGPURaycast->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );
      
      if (MapperGPURaycast->IsRenderSupported(window,vspNode->GetVolumePropertyNode()->GetVolumeProperty()))
      {
        MapperGPURaycast->Delete();
        return 1;
      }
      else
      {
        MapperGPURaycast->Delete();
        return 0;
      }
    }
  case 4:
    {
      vtkSlicerGPURayCastMultiVolumeMapper* MapperGPURaycastII = vtkSlicerGPURayCastMultiVolumeMapper::New();
      
      MapperGPURaycastII->SetNthInput(0, vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData());
      if (vspNode->GetFgVolumeNode())
        MapperGPURaycastII->SetNthInput(1, vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetFgVolumeNode())->GetImageData());
      
      if (MapperGPURaycastII->IsRenderSupported(window, vspNode->GetVolumePropertyNode()->GetVolumeProperty()))
      {
        MapperGPURaycastII->Delete();
        return 1;
      }
      else
      {
        MapperGPURaycastII->Delete();
        return 0;
      }
    }
  case 2:
    {
      vtkSlicerVolumeTextureMapper3D* MapperTexture = vtkSlicerVolumeTextureMapper3D::New();
    
      MapperTexture->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );

      if (MapperTexture->IsRenderSupported(window, vspNode->GetVolumePropertyNode()->GetVolumeProperty()))
      {
        MapperTexture->Delete();
        return 1;
      }
      else
      {
        MapperTexture->Delete();
        return 0;
      }
    }
  case 1:
    {
    vtkGPUVolumeRayCastMapper* VTKGPURaycast = vtkGPUVolumeRayCastMapper::New();
    VTKGPURaycast->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );
    if (VTKGPURaycast->IsRenderSupported(window,vspNode->GetVolumePropertyNode()->GetVolumeProperty()))
      {
        VTKGPURaycast->Delete();
        return 1;
      }
      else
      {
        VTKGPURaycast->Delete();
        return 0;
      }
    }
  default:
    return 0;
  }
}

/*
 * return values:
 * -1: requested mapper not supported
 *  0: invalid input parameter
 *  1: success
 */
int vtkMRMLVolumeRenderingDisplayableManager
::SetupMapperFromParametersNode(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (vspNode == NULL || vspNode->GetVolumeNode() == NULL)
    {
    return 0;
    }

  this->Volume->SetMapper(NULL);
  this->Volume->SetProperty(NULL);
  this->EstimateSampleDistance(vspNode);

  vtkRenderWindow* window = this->GetRenderer()->GetRenderWindow();

  switch(vspNode->GetCurrentVolumeMapper())//mapper specific initialization
    {
    case vtkMRMLVolumeRenderingDisplayNode::VTKCPURayCast:
      this->MapperRaycast->SetInput(
        vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())
                                   ->GetImageData() );
      this->MapperRaycast->SetSampleDistance(
        vspNode->GetEstimatedSampleDistance());
      this->SetCPURaycastParameters(vspNode);
      this->Volume->SetMapper(this->MapperRaycast);
      if (vspNode->GetVolumePropertyNode())
        {
        this->Volume->SetProperty(vspNode->GetVolumePropertyNode()->GetVolumeProperty());
        }
      break;
    case vtkMRMLVolumeRenderingDisplayNode::NCIGPURayCast:
      this->MapperGPURaycast->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );
      this->SetGPURaycastParameters(vspNode);
      if ( vspNode->GetVolumePropertyNode() && vspNode->GetVolumePropertyNode()->GetVolumeProperty() )
        {
        if (this->MapperGPURaycast->IsRenderSupported(window,vspNode->GetVolumePropertyNode()->GetVolumeProperty()))
          {
          this->Volume->SetMapper(this->MapperGPURaycast);
          if (vspNode->GetVolumePropertyNode())
            {
            this->Volume->SetProperty(vspNode->GetVolumePropertyNode()->GetVolumeProperty());
            }
          }
        }
      else
        {
        return -1;
        }
      break;
    case vtkMRMLVolumeRenderingDisplayNode::NCIGPURayCastMultiVolume:
      this->MapperGPURaycastII->SetNthInput(0, vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData());
      if (vspNode->GetFgVolumeNode())
        {
        this->MapperGPURaycastII->SetNthInput(1, vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetFgVolumeNode())->GetImageData());
        }
      this->SetGPURaycastIIParameters(vspNode);
      if (this->MapperGPURaycastII->IsRenderSupported(window, vspNode->GetVolumePropertyNode()->GetVolumeProperty()))
        {
        this->Volume->SetMapper(this->MapperGPURaycastII);
        this->CreateVolumePropertyGPURaycastII(vspNode);
        this->Volume->SetProperty(this->VolumePropertyGPURaycastII);
        }
      else
        {
        return -1;
        }
      break;
    case vtkMRMLVolumeRenderingDisplayNode::VTKGPUTextureMapping:
      this->MapperTexture->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );
      this->MapperTexture->SetSampleDistance(vspNode->GetEstimatedSampleDistance());
      if (this->MapperTexture->IsRenderSupported(window, vspNode->GetVolumePropertyNode()->GetVolumeProperty()))
        {
        this->Volume->SetMapper(this->MapperTexture);
        if (vspNode->GetVolumePropertyNode())
          {
          this->Volume->SetProperty(vspNode->GetVolumePropertyNode()->GetVolumeProperty());
          }
        }
      else
        {
        return -1;
        }
      break;
    case vtkMRMLVolumeRenderingDisplayNode::VTKGPURayCast:
      this->MapperGPURaycast3->SetInput(vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData());
      this->MapperGPURaycast3->SetSampleDistance(vspNode->GetEstimatedSampleDistance());
      this->SetGPURaycast3Parameters(vspNode);
       if (this->MapperGPURaycast3->IsRenderSupported(window, vspNode->GetVolumePropertyNode()->GetVolumeProperty()))
        {
        this->Volume->SetMapper(this->MapperGPURaycast3);
        if (vspNode->GetVolumePropertyNode())
          {
          this->Volume->SetProperty(vspNode->GetVolumePropertyNode()->GetVolumeProperty());
          }
        }
      //else
      //  return -1;
      break;
    }

  this->SetExpectedFPS(vspNode->GetExpectedFPS());
  this->ComputeInternalVolumeSize( vspNode->GetGPUMemorySize() ?
    vspNode->GetGPUMemorySize() :
    vtkMRMLVolumeRenderingDisplayableManager::DefaultGPUMemorySize);

  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  this->CalculateMatrix(vspNode, matrix);
  this->Volume->PokeMatrix(matrix);
  matrix->Delete();

  return 1;
}

/*
void vtkMRMLVolumeRenderingDisplayableManager::SetupVolumeRenderingInteractive(
  vtkMRMLVolumeRenderingDisplayNode* vspNode, int buttonDown)
{
  // When start (rendering??) set CPU ray casting to be interactive
  if (buttonDown == 1 && vspNode->GetExpectedFPS() > 0)
    {
    this->SetExpectedFPS(vspNode->GetExpectedFPS());
    }
  else
    {
    this->MapperGPURaycast->SetFramerate(1.0);
    this->MapperGPURaycastII->SetFramerate(1.0);
    this->MapperTexture->SetFramerate(1.0);
    }

  return 1;
}
*/

void vtkMRMLVolumeRenderingDisplayableManager::SetVolumeVisibility(int isVisible)
{
  if (isVisible)
    this->Volume->VisibilityOn();
  else
    this->Volume->VisibilityOff();
}



void vtkMRMLVolumeRenderingDisplayableManager::SetROI(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (!vspNode
      || vspNode->GetROINode() == NULL
      || !vspNode->GetCroppingEnabled())
    {
    this->MapperTexture->RemoveAllClippingPlanes();
    this->MapperRaycast->RemoveAllClippingPlanes();
    this->MapperGPURaycast->RemoveAllClippingPlanes();
    this->MapperGPURaycast->ClippingOff();
    this->MapperGPURaycastII->RemoveAllClippingPlanes();
    this->MapperGPURaycastII->ClippingOff();
    this->MapperGPURaycast3->RemoveAllClippingPlanes();

    return;
    }

  vtkPlanes *planes = vtkPlanes::New();
  vspNode->GetROINode()->GetTransformedPlanes(planes);
  if ( planes->GetTransform() )
    {
    double zero[3] = {0,0,0};
    double translation[3];
    planes->GetTransform()->TransformPoint(zero, translation);

    // apply the translation to the planes

    int numPlanes = planes->GetNumberOfPlanes();
    vtkPoints *points = planes->GetPoints();
    for (int i=0; i<numPlanes && i<6; i++)
      {
      vtkPlane *plane = planes->GetPlane(i);
      double origin[3];
      plane->GetOrigin(origin);
      points->GetData()->SetTuple3(i,
                                   origin[0]-translation[0],
                                   origin[1]-translation[1],
                                   origin[2]-translation[2]);
      }
    }

  this->MapperTexture->SetClippingPlanes(planes);
  this->MapperRaycast->SetClippingPlanes(planes);

  this->MapperGPURaycast->SetClippingPlanes(planes);
  this->MapperGPURaycast->ClippingOn();

  this->MapperGPURaycastII->SetClippingPlanes(planes);
  this->MapperGPURaycastII->ClippingOn();

  this->MapperGPURaycast3->SetClippingPlanes(planes);

  planes->Delete();
}

void vtkMRMLVolumeRenderingDisplayableManager::TransformModified(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  this->CalculateMatrix(vspNode, matrix);
  this->Volume->PokeMatrix(matrix);

  this->VolumeRenderingLogic->FitROIToVolume(vspNode);
}

void vtkMRMLVolumeRenderingDisplayableManager::UpdateVolumePropertyGPURaycastII(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (vspNode->GetCurrentVolumeMapper() == 4)
  {
    this->CreateVolumePropertyGPURaycastII(vspNode);
    this->Volume->SetProperty(this->VolumePropertyGPURaycastII);
  }
}

void vtkMRMLVolumeRenderingDisplayableManager::CreateVolumePropertyGPURaycastII(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (vspNode->GetCurrentVolumeMapper() != 4)
    return;

  if (this->VolumePropertyGPURaycastII != NULL)
    this->VolumePropertyGPURaycastII->Delete();

  this->VolumePropertyGPURaycastII = vtkVolumeProperty::New();

  //copy bg property into 1st compoent property
  vtkVolumeProperty* prop = vspNode->GetVolumePropertyNode()->GetVolumeProperty();
  {
    int colorChannels = prop->GetColorChannels(0);

    switch(colorChannels)
    {
    case 1:
      this->VolumePropertyGPURaycastII->SetColor(0, prop->GetGrayTransferFunction(0));
      break;
    case 3:
      this->VolumePropertyGPURaycastII->SetColor(0, prop->GetRGBTransferFunction(0));
      break;
    }

    this->VolumePropertyGPURaycastII->SetScalarOpacity(0, prop->GetScalarOpacity(0));
    this->VolumePropertyGPURaycastII->SetGradientOpacity(0, prop->GetGradientOpacity(0));
    this->VolumePropertyGPURaycastII->SetScalarOpacityUnitDistance(0, prop->GetScalarOpacityUnitDistance(0));

    this->VolumePropertyGPURaycastII->SetDisableGradientOpacity(0, prop->GetDisableGradientOpacity(0));

    this->VolumePropertyGPURaycastII->SetShade(0, prop->GetShade(0));
    this->VolumePropertyGPURaycastII->SetAmbient(0, prop->GetAmbient(0));
    this->VolumePropertyGPURaycastII->SetDiffuse(0, prop->GetDiffuse(0));
    this->VolumePropertyGPURaycastII->SetSpecular(0, prop->GetSpecular(0));
    this->VolumePropertyGPURaycastII->SetSpecularPower(0, prop->GetSpecularPower(0));

    this->VolumePropertyGPURaycastII->SetIndependentComponents(prop->GetIndependentComponents());
    this->VolumePropertyGPURaycastII->SetInterpolationType(prop->GetInterpolationType());
  }

  if (vspNode->GetUseSingleVolumeProperty())
  {
    vtkVolumeProperty* propFg = vspNode->GetVolumePropertyNode()->GetVolumeProperty();
    int colorChannels = propFg->GetColorChannels(0);

    switch(colorChannels)
    {
    case 1:
      this->VolumePropertyGPURaycastII->SetColor(1, propFg->GetGrayTransferFunction(0));
      break;
    case 3:
      this->VolumePropertyGPURaycastII->SetColor(1, propFg->GetRGBTransferFunction(0));
      break;
    }

    this->VolumePropertyGPURaycastII->SetScalarOpacity(1, propFg->GetScalarOpacity(0));
    this->VolumePropertyGPURaycastII->SetGradientOpacity(1, propFg->GetGradientOpacity(0));
    this->VolumePropertyGPURaycastII->SetScalarOpacityUnitDistance(1, propFg->GetScalarOpacityUnitDistance(0));
    this->VolumePropertyGPURaycastII->SetDisableGradientOpacity(1, propFg->GetDisableGradientOpacity(0));
  }
  else if (vspNode->GetFgVolumePropertyNode())//copy fg property into 2nd component property
  {
    vtkVolumeProperty* propFg = vspNode->GetFgVolumePropertyNode()->GetVolumeProperty();
    int colorChannels = propFg->GetColorChannels(0);

    switch(colorChannels)
    {
    case 1:
      this->VolumePropertyGPURaycastII->SetColor(1, propFg->GetGrayTransferFunction(0));
      break;
    case 3:
      this->VolumePropertyGPURaycastII->SetColor(1, propFg->GetRGBTransferFunction(0));
      break;
    }

    this->VolumePropertyGPURaycastII->SetScalarOpacity(1, propFg->GetScalarOpacity(0));
    this->VolumePropertyGPURaycastII->SetGradientOpacity(1, propFg->GetGradientOpacity(0));
    this->VolumePropertyGPURaycastII->SetScalarOpacityUnitDistance(1, propFg->GetScalarOpacityUnitDistance(0));
    this->VolumePropertyGPURaycastII->SetDisableGradientOpacity(1, propFg->GetDisableGradientOpacity(0));
  }

  this->Volume->SetProperty(this->VolumePropertyGPURaycastII);
}
/*
void vtkMRMLVolumeRenderingDisplayableManager::UpdateVolumePropertyGPURaycast3(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (vspNode->GetCurrentVolumeMapper() == 5)
  {
    this->CreateVolumePropertyGPURaycast3(vspNode);
    this->Volume->SetProperty(this->VolumePropertyGPURaycast3);
  }
}

void vtkMRMLVolumeRenderingDisplayableManager::CreateVolumePropertyGPURaycast3(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (vspNode->GetCurrentVolumeMapper() != 5)
    return;

  if (this->VolumePropertyGPURaycast3 != NULL)
    this->VolumePropertyGPURaycast3->Delete();

  this->VolumePropertyGPURaycast3 = vtkVolumeProperty::New();

  //copy bg property into 1st compoent property
  vtkVolumeProperty* prop = vspNode->GetVolumePropertyNode()->GetVolumeProperty();
  {
    int colorChannels = prop->GetColorChannels(0);

    switch(colorChannels)
    {
    case 1:
      this->VolumePropertyGPURaycast3->SetColor(0, prop->GetGrayTransferFunction(0));
      break;
    case 3:
      this->VolumePropertyGPURaycast3->SetColor(0, prop->GetRGBTransferFunction(0));
      break;
    }

    this->VolumePropertyGPURaycast3->SetScalarOpacity(0, prop->GetScalarOpacity(0));
    this->VolumePropertyGPURaycast3->SetGradientOpacity(0, prop->GetGradientOpacity(0));
    this->VolumePropertyGPURaycast3->SetScalarOpacityUnitDistance(0, prop->GetScalarOpacityUnitDistance(0));

    this->VolumePropertyGPURaycast3->SetDisableGradientOpacity(0, prop->GetDisableGradientOpacity(0));

    this->VolumePropertyGPURaycast3->SetShade(0, prop->GetShade(0));
    this->VolumePropertyGPURaycast3->SetAmbient(0, prop->GetAmbient(0));
    this->VolumePropertyGPURaycast3->SetDiffuse(0, prop->GetDiffuse(0));
    this->VolumePropertyGPURaycast3->SetSpecular(0, prop->GetSpecular(0));
    this->VolumePropertyGPURaycast3->SetSpecularPower(0, prop->GetSpecularPower(0));

    this->VolumePropertyGPURaycast3->SetIndependentComponents(prop->GetIndependentComponents());
    this->VolumePropertyGPURaycast3->SetInterpolationType(prop->GetInterpolationType());
  }

  this->Volume->SetProperty(this->VolumePropertyGPURaycast3);
}
*/

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnScenarioNodeModified()
{
  /* Not sure what special should be done when the scenario (not used) is
     modified
  vtkMRMLViewNode *viewNode = this->GetMRMLViewNode();
  if (viewNode)
    {
    this->UpdateDisplayNodeList();
    this->InitializePipelineFromDisplayNode();
    }
  */
}



//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnVolumeRenderingDisplayNodeModified(vtkMRMLVolumeRenderingDisplayNode* dnode)
{
  if (!dnode)
    {
    return;
    }
  if (!this->ValidateDisplayNode(dnode))
    {
    if (dnode == this->DisplayedNode)
      {
      this->DisplayedNode = 0;
      this->RemoveVolumeFromView();
      }
    return;
    }
  this->InitializePipelineFromDisplayNode(dnode);
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::Create()
{
//  if (this->ProcessingMRMLFlag)
//    {
//    return;
//    }
//  this->ProcessingMRMLFlag = 1;

  this->OnCreate();

//  this->ProcessingMRMLFlag = 0;
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnCreate()
{
  assert(this->GetMRMLViewNode());
  if (this->GetMRMLViewNode() &&
      this->GetMRMLViewNode()->HasObserver(
        vtkMRMLViewNode::GraphicalResourcesCreatedEvent,
        (vtkCommand *) this->GetMRMLCallbackCommand()))
    {
    this->GetMRMLViewNode()->AddObserver(
      vtkMRMLViewNode::GraphicalResourcesCreatedEvent,
      (vtkCommand *) this->GetMRMLCallbackCommand());
    }

  this->UpdateDisplayNodeList();

  //this->OnVolumeRenderingDisplayNodeModified();
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::ProcessMRMLEvents(vtkObject *caller,
                                                unsigned long event,
                                                void *callData)
{
  if (this->ProcessingMRMLFlag || this->UpdatingFromMRML)
    {
    return;
    }
  this->ProcessingMRMLFlag = 1;

  vtkMRMLNode *node = NULL;

  // Observe ViewNode, Scenario Node, and Parameter node for modify events
  if (event == vtkCommand::ModifiedEvent && this->GetMRMLScene())
    {
    node = reinterpret_cast<vtkMRMLNode *>(caller);

    if (node != NULL && node->IsA("vtkMRMLVolumeRenderingScenarioNode") )
      {
      this->OnScenarioNodeModified();
      }
// on view node modified should be processed in OnMRMLViewNodeModifiedEvent()
//    else if (node != NULL && node == this->GetMRMLViewNode() )
//      {
//      //this->UpdateDisplayNodeList();
//      }
//    else if (node != NULL && node == this->GetDisplayNode() )
//      {
//      this->OnVolumeRenderingDisplayNodeModified();
//      }
    else if (node->IsA("vtkMRMLVolumeRenderingDisplayNode") )
      {
      //this->UpdateDisplayNodeList();
      this->OnVolumeRenderingDisplayNodeModified(
        vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(node));
      }
    else if(vtkMRMLAnnotationROINode::SafeDownCast(caller))
      {
      vtkMRMLAnnotationROINode *roiNode =
        vtkMRMLAnnotationROINode::SafeDownCast(caller);
      vtkMRMLVolumeRenderingDisplayNode* vspNode =
        this->VolumeRenderingLogic->
        GetFirstVolumeRenderingDisplayNodeByROINode(roiNode);

      this->SetROI(vspNode);
      this->RequestRender();
      }
    }
  else if (event == vtkMRMLScene::NodeAddedEvent)
    {
    node = reinterpret_cast<vtkMRMLNode *>(callData);
    this->OnMRMLSceneNodeAddedEvent(node);
    }
  else if (event == vtkMRMLScene::NodeRemovedEvent)
    {
    node = reinterpret_cast<vtkMRMLNode *>(callData);
    this->OnMRMLSceneNodeRemovedEvent(node);
    }
  else if (event == vtkMRMLScene::SceneRestoredEvent)
    {
    this->OnMRMLSceneRestoredEvent();
    }
  else if (event == vtkMRMLScene::SceneImportedEvent)
    {
    this->OnMRMLSceneImportedEvent();
    }
  else if (event == vtkMRMLScene::SceneClosedEvent)
    {
    this->OnMRMLSceneClosedEvent();
    }
  else if (event == vtkMRMLScene::SceneAboutToBeClosedEvent)
    {
    this->OnMRMLSceneAboutToBeClosedEvent();
    }
  else if (event == vtkMRMLScalarVolumeNode::ImageDataModifiedEvent)
    {
    this->RequestRender();
    }
  else if (event == vtkMRMLTransformableNode::TransformModifiedEvent)
    {
    node = reinterpret_cast<vtkMRMLNode *>(caller);
    vtkMRMLVolumeRenderingDisplayNode* vspNode =
      this->VolumeRenderingLogic->GetVolumeRenderingDisplayNodeForViewNode(
        vtkMRMLVolumeNode::SafeDownCast(node),
        this->GetMRMLViewNode());
    this->TransformModified(vspNode);

    this->RequestRender();
    }
  else if (event == vtkMRMLVolumeNode::DisplayModifiedEvent)
    {
    // who calls that ?
    assert(false);
    //vtkMRMLVolumeRenderingDisplayNode* vspNode = this->GetDisplayNode();
    //this->VolumeRenderingLogic->UpdateVolumePropertyFromDisplayNode(vspNode);
    }
  else if (event == vtkMRMLViewNode::GraphicalResourcesCreatedEvent)
    {
    this->RemoveVolumeFromView();
    this->AddVolumeToView();
    }

  // TODO add code from vtkVolumeRenderingGUI::ProcessGUIEvents
  // to observe ParametersNode (ROI, VolumeProp, volumes etc)
  this->ProcessingMRMLFlag = 0;

}


//initialize pipeline from a loaded or user selected parameters node
void vtkMRMLVolumeRenderingDisplayableManager
::InitializePipelineFromDisplayNode(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (!vspNode ||
      !this->ValidateDisplayNode(vspNode))
    {
    return;
    }

  //this->RemoveVolumeFromView();

  //this->GetInteractor()->Disable();

  //this->SetupHistograms(vspNode);
  //if (vspNode->GetFgVolumeNode())
  //  this->SetupHistogramsFg(vspNode);

  if (vspNode->GetROINode())
    {
    vspNode->GetROINode()->InsideOutOn();
    if (!vspNode->GetROINode()->HasObserver(vtkCommand::ModifiedEvent, (vtkCommand *) this->GetMRMLCallbackCommand()))
      {
      vspNode->GetROINode()->AddObserver(vtkCommand::ModifiedEvent, (vtkCommand *) this->GetMRMLCallbackCommand());
      }
    }
  this->SetROI(vspNode);


  //prepare rendering frame
  //this->DeleteRenderingFrame();
  //this->CreateRenderingFrame();


  vtkMRMLVolumeNode *volumeNode = vspNode->GetVolumeNode();
  //Add observer to trigger update of transform
  if (volumeNode)
    {
    if (!volumeNode->HasObserver(vtkMRMLTransformableNode::TransformModifiedEvent,(vtkCommand *) this->GetMRMLCallbackCommand()))
      {
      volumeNode->AddObserver(vtkMRMLTransformableNode::TransformModifiedEvent,(vtkCommand *) this->GetMRMLCallbackCommand());
      }
    if (!volumeNode->HasObserver(vtkMRMLScalarVolumeNode::ImageDataModifiedEvent, (vtkCommand *) this->GetMRMLCallbackCommand() ))
      {
      volumeNode->AddObserver(vtkMRMLScalarVolumeNode::ImageDataModifiedEvent, (vtkCommand *) this->GetMRMLCallbackCommand() );
      }
    }

  volumeNode = vspNode->GetFgVolumeNode();
  if (volumeNode)
    {
    //Add observer to trigger update of transform
    if (!volumeNode->HasObserver(vtkMRMLTransformableNode::TransformModifiedEvent,(vtkCommand *) this->GetMRMLCallbackCommand()))
      {
      volumeNode->AddObserver(vtkMRMLTransformableNode::TransformModifiedEvent,(vtkCommand *) this->GetMRMLCallbackCommand());
      }
    if(!volumeNode->AddObserver(vtkMRMLScalarVolumeNode::ImageDataModifiedEvent, (vtkCommand *) this->GetMRMLCallbackCommand() ))
      {
      volumeNode->AddObserver(vtkMRMLScalarVolumeNode::ImageDataModifiedEvent, (vtkCommand *) this->GetMRMLCallbackCommand() );
      }
    }


  this->SetupMapperFromParametersNode(vspNode);

  this->DisplayedNode = vspNode;
  
  vtkRenderWindow* renderWindow = this->GetRenderer()->GetRenderWindow();
  renderWindow->SetDesiredUpdateRate(this->DisplayedNode->GetExpectedFPS());
  vtkRenderWindowInteractor* renderWindowInteractor =
    renderWindow ? renderWindow->GetInteractor() : 0;
  if (renderWindowInteractor)
    {
    renderWindowInteractor->SetDesiredUpdateRate(this->DisplayedNode->GetExpectedFPS());
    }
  // TODO: do the connection at initialization time
  vtkInteractorObserver* interactorStyle =
    renderWindowInteractor ? renderWindowInteractor->GetInteractorStyle() : 0;
  if (interactorStyle)
    {
    if (!interactorStyle->HasObserver(vtkCommand::StartInteractionEvent,
                                     (vtkCommand*) this->GetLogicCallbackCommand()))
      {
      interactorStyle->AddObserver(vtkCommand::StartInteractionEvent,
                                   (vtkCommand*) this->GetLogicCallbackCommand());
      interactorStyle->AddObserver(vtkCommand::EndInteractionEvent,
                                   (vtkCommand*) this->GetLogicCallbackCommand());
      }
    }
  this->AddVolumeToView();
}

//----------------------------------------------------------------------------

int vtkMRMLVolumeRenderingDisplayableManager::ValidateDisplayNode(vtkMRMLVolumeRenderingDisplayNode* vspNode)
{
  if (vspNode == NULL)
    {
    return 0;
    } 
  //check all inputs
  if (vspNode->GetVolumeNode() == NULL)
    {
    return 0;
    }
  if (vspNode->GetROINode() == NULL)
    {
    //return 0;
    }
  if (vspNode->GetVolumePropertyNode() == NULL)
    {
    return 0;
    }

  if (vspNode->GetVolumeNodeID() && strcmp(vspNode->GetVolumeNodeID(), "NULL") != 0 && vspNode->GetVolumeNode() == NULL)
    {
    return 0;
    }
  if (vspNode->GetFgVolumeNodeID() && strcmp(vspNode->GetFgVolumeNodeID(), "NULL") != 0 && vspNode->GetFgVolumeNode() == NULL)
    {
    return 0;
    }
  if (this->GetMRMLViewNode() == NULL)
    {
    return 0;
    }
  if (!vspNode->IsDisplayableInView(this->GetMRMLViewNode()->GetID()))
    {
    return 0;
    }
  if (!vspNode->GetVisibility())
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::ProcessLogicEvents(
  vtkObject * caller, unsigned long event, void * vtkNotUsed(callData))
{
  vtkInteractorStyle* interactorStyle = vtkInteractorStyle::SafeDownCast(caller);
  if (interactorStyle)
    {
    if (event == vtkCommand::StartInteractionEvent)
      {
      this->SetExpectedFPS(
        this->DisplayedNode ? this->DisplayedNode->GetExpectedFPS() : 8);
      }
    else if (event == vtkCommand::EndInteractionEvent)
      {
      this->SetExpectedFPS(0.);
      }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::AddVolumeToView()
{
  bool modified = false;
//  vtkMRMLVolumeRenderingDisplayNode* vspNode = this->GetDisplayNode();

  // Only support 1 volume per view, remove any existing volume
  vtkVolumeCollection *vols = this->GetRenderer()->GetVolumes();
  if (vols && vols->GetNextVolume() != this->GetVolumeActor())
    {
    this->RemoveVolumeFromView(vols->GetNextVolume());
    modified = true;
    }

  if (vols != NULL && this->GetVolumeActor() &&
     !vols->IsItemPresent(this->GetVolumeActor()) )
    {
    this->GetRenderer()->AddVolume(this->GetVolumeActor() );
    modified = true;
    }

  if (modified)
    {
    this->GetInteractor()->Enable();
    this->RequestRender();
    }
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::RemoveVolumeFromView()
{
  this->RemoveVolumeFromView(this->GetVolumeActor());
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::RemoveVolumeFromView(vtkVolume* volume)
{
  this->GetRenderer()->RemoveVolume( volume );
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneAboutToBeClosedEvent()
{
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneClosedEvent()
{
  // Clean
  this->SetUpdateFromMRMLRequested(1);
  this->RemoveVolumeFromView();
  this->RemoveDisplayNodes();
  this->Reset();

  //this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneImportedEvent()
{
  // UpdateFromMRML will be executed only if there has been some actions
  // during the import that requested it (don't call
  // SetUpdateFromMRMLRequested(1) here, it should be done somewhere else
  // maybe in OnMRMLSceneNodeAddedEvent, OnMRMLSceneNodeRemovedEvent or
  // OnMRMLDisplayableModelNodeModifiedEvent).
  this->OnCreate();
  //this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneRestoredEvent()
{
  // UpdateFromMRML will be executed only if there has been some actions
  // during the restoration that requested it (don't call
  // SetUpdateFromMRMLRequested(1) here, it should be done somewhere else
  // maybe in OnMRMLSceneNodeAddedEvent, OnMRMLSceneNodeRemovedEvent or
  // OnMRMLDisplayableModelNodeModifiedEvent).
  this->OnCreate();
  //this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneNodeAddedEvent(vtkMRMLNode* node)
{
  if (!node->IsA("vtkMRMLVolumeNode") &&
      !node->IsA("vtkMRMLVolumeRenderingDisplayNode") &&
      !node->IsA("vtkMRMLVolumePropertyNode") &&
      !node->IsA("vtkMRMLAnnotationROINode") &&
      !node->IsA("vtkMRMLVolumeRenderingScenarioNode"))
    {
    return;
    }

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->GetIsUpdating())
    {
    return;
    }

  if (node->IsA("vtkMRMLVolumeRenderingDisplayNode") )
    {
    this->AddDisplayNode(vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(node));
    }

  if (node->IsA("vtkMRMLVolumeRenderingScenarioNode") )
    {
    //remember the newly added scenarioNode
    //vtkMRMLVolumeRenderingScenarioNode *sNode = vtkMRMLVolumeRenderingScenarioNode::SafeDownCast(node);
    this->OnScenarioNodeModified();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::OnMRMLSceneNodeRemovedEvent(vtkMRMLNode* node)
{
  assert(node);

  if (!node->IsA("vtkMRMLVolumeNode") &&
      !node->IsA("vtkMRMLVolumeRenderingDisplayNode") &&
      !node->IsA("vtkMRMLVolumePropertyNode") &&
      !node->IsA("vtkMRMLAnnotationROINode") &&
      !node->IsA("vtkMRMLVolumeRenderingScenarioNode"))
    {
    return;
    }

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->GetIsUpdating())
    {
    return;
    }

  if (node->IsA("vtkMRMLVolumeRenderingDisplayNode"))
    {
    this->RemoveDisplayNode(vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(node));
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::UpdateFromMRML()
{
  if (this->ProcessingMRMLFlag || this->UpdatingFromMRML)
    {
    return;
    }
  this->UpdatingFromMRML = 1;
  //this->InitializePipelineFromDisplayNode();
  this->UpdatingFromMRML = 0;
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::UpdateDisplayNodeList()
{
  std::vector<vtkMRMLNode *> nodes;
  if (this->GetMRMLScene())
    {
    this->GetMRMLScene()->GetNodesByClass(
      "vtkMRMLVolumeRenderingDisplayNode", nodes);
    }

  for (unsigned int i = 0; i < nodes.size(); ++i)
    {
    this->AddDisplayNode(
      vtkMRMLVolumeRenderingDisplayNode::SafeDownCast(nodes[i]));
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager
::AddDisplayNode(vtkMRMLVolumeRenderingDisplayNode *dnode)
{
  if (!dnode || this->DisplayNodes.count(std::string(dnode->GetID())) )
    {
    return;
    }

  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  ///TODO: have vtkEventBroker::AddObservation to uniquely add an observation
  std::vector< vtkObservation *> observations =
    broker->GetObservations(dnode, vtkCommand::ModifiedEvent,
                            this, this->GetMRMLCallbackCommand());
  if (observations.size() == 0)
    {
    broker->AddObservation(dnode,vtkCommand::ModifiedEvent,
                           this, this->GetMRMLCallbackCommand());
    }
  this->DisplayNodes[dnode->GetID()] = dnode;
  this->OnVolumeRenderingDisplayNodeModified(dnode);
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager
::RemoveDisplayNode(vtkMRMLVolumeRenderingDisplayNode *dnode)
{
  vtkEventBroker *broker = vtkEventBroker::GetInstance();
  ///TODO: have vtkEventBroker::AddObservation to uniquely add an observation
  std::vector< vtkObservation *> observations =
    broker->GetObservations(dnode, vtkCommand::ModifiedEvent,
                            this, this->GetMRMLCallbackCommand());
  if (observations.size() != 0)
    {
    broker->RemoveObservations(observations);
    }
  this->DisplayNodes.erase(dnode->GetID());

  if (dnode == this->DisplayedNode)
    {
    this->DisplayedNode = 0;
    this->RemoveVolumeFromView();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLVolumeRenderingDisplayableManager::RemoveDisplayNodes()
{
  while (!this->DisplayNodes.empty())
    {
    this->RemoveDisplayNode(this->DisplayNodes.begin()->second);
    }
}
