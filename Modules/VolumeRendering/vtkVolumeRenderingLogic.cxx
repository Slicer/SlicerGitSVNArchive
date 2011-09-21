

// Slicer includes
#include "vtkSlicerVolumeTextureMapper3D.h"
#include "vtkSlicerFixedPointVolumeRayCastMapper.h"
#include "vtkSlicerGPURayCastVolumeMapper.h"
#include "vtkSlicerGPURayCastMultiVolumeMapper.h"
#include "vtkSlicerApplication.h"

// VolumeRendering includes
#include "vtkVolumeRenderingLogic.h"

// KWWidgets includes
#include "vtkKWHistogramSet.h"
#include "vtkKWHistogram.h"

// MRMLLogic includes
#include <vtkMRMLSliceLogic.h>

// MRML includes
#include "vtkMRMLVolumeRenderingParametersNode.h"
#include "vtkMRMLVolumeRenderingScenarioNode.h"
#include "vtkMRMLTransformNode.h"
#include "vtkMRMLROINode.h"
#include "vtkMRMLVolumePropertyNode.h"
#include "vtkMRMLVolumePropertyStorageNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLLabelMapVolumeDisplayNode.h"
#include "vtkMRMLScalarVolumeNode.h"

// VTK includes
#include "vtkObjectFactory.h"
#include "vtkObject.h"
#include "vtkVolumeProperty.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkMatrix4x4.h"
#include "vtkPlanes.h"
#include "vtkPlane.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageGradientMagnitude.h"

// ITK includes
#include <itksys/SystemTools.hxx> 
#include <itksys/Directory.hxx> 

// STD includes
#include <cmath>

bool vtkVolumeRenderingLogic::First = true;

vtkVolumeRenderingLogic::vtkVolumeRenderingLogic(void)
{
  //create instances of mappers
  this->MapperTexture = vtkSlicerVolumeTextureMapper3D::New();

  this->MapperGPURaycast = vtkSlicerGPURayCastVolumeMapper::New();

  this->MapperGPURaycastII = vtkSlicerGPURayCastMultiVolumeMapper::New();

  this->MapperRaycast = vtkSlicerFixedPointVolumeRayCastMapper::New();
  this->MapperGPURaycast3 = vtkGPUVolumeRayCastMapper::New();

  //create instance of the actor
  this->Volume = vtkVolume::New();

  this->Histograms = vtkKWHistogramSet::New();
  this->HistogramsFg = vtkKWHistogramSet::New();

  this->GUICallback = NULL;

  this->VolumePropertyGPURaycastII = NULL;
  //this->VolumePropertyGPURaycast3 = NULL;
}

vtkVolumeRenderingLogic::~vtkVolumeRenderingLogic(void)
{
  //delete instances
  if (this->MapperTexture)
  {
    this->MapperTexture->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperTexture->Delete();
    this->MapperTexture = NULL;
  }
  if (this->MapperGPURaycast)
  {
    this->MapperGPURaycast->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperGPURaycast->Delete();
    this->MapperGPURaycast = NULL;
  }
  if (this->MapperGPURaycastII)
  {
    this->MapperGPURaycastII->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperGPURaycastII->Delete();
    this->MapperGPURaycastII = NULL;
  }
  if (this->MapperRaycast)
  {
    this->MapperRaycast->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperRaycast->RemoveObservers(vtkCommand::ProgressEvent, this->GUICallback);
    this->MapperRaycast->Delete();
    this->MapperRaycast = NULL;
  }

  if (this->MapperGPURaycast3)
  {
    this->MapperGPURaycast3->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperGPURaycast3->Delete();
    this->MapperGPURaycast3 = NULL;
  }

  if (this->Volume)
  {
    this->Volume->Delete();
    this->Volume = NULL;
  }
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

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVolumeRenderingLogic);

void vtkVolumeRenderingLogic::PrintSelf(std::ostream &os, vtkIndent indent)
{
  os<<indent<<"Print logic"<<endl;
}

void vtkVolumeRenderingLogic::SetMRMLScene(vtkMRMLScene *scene)
{
  vtkSlicerModuleLogic::SetMRMLScene(scene);
  this->RegisterNodes();
}

void vtkVolumeRenderingLogic::RegisterNodes()
{
  if (this->MRMLScene && this->First)
  {
    // :NOTE: 20050513 tgl: Guard this so it is only registered once.
    vtkMRMLVolumeRenderingScenarioNode *vrsNode = vtkMRMLVolumeRenderingScenarioNode::New();
    this->MRMLScene->RegisterNodeClass(vrsNode);
    vrsNode->Delete();

    vtkMRMLVolumeRenderingParametersNode *vrpNode = vtkMRMLVolumeRenderingParametersNode::New();
    this->MRMLScene->RegisterNodeClass(vrpNode);
    vrpNode->Delete();

    this->First = false;
  }
}

void vtkVolumeRenderingLogic::SetGUICallbackCommand(vtkCommand* callback)
{
  this->MapperTexture->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);

  //cpu ray casting
  this->MapperRaycast->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);
  this->MapperRaycast->AddObserver(vtkCommand::ProgressEvent,callback);

  //hook up the gpu mapper
  this->MapperGPURaycast->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);

  this->MapperGPURaycastII->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);
  this->MapperGPURaycast3->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, callback);

  this->GUICallback = callback;
}

void vtkVolumeRenderingLogic::Reset()
{
  //delete instances
  if (this->MapperTexture)
  {
    this->MapperTexture->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperTexture->Delete();
    this->MapperTexture = NULL;
  }
  if (this->MapperGPURaycast)
  {
    this->MapperGPURaycast->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperGPURaycast->Delete();
    this->MapperGPURaycast = NULL;
  }
  if (this->MapperGPURaycastII)
  {
    this->MapperGPURaycastII->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperGPURaycastII->Delete();
    this->MapperGPURaycastII = NULL;
  }
  if (this->MapperRaycast)
  {
    this->MapperRaycast->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperRaycast->RemoveObservers(vtkCommand::ProgressEvent, this->GUICallback);
    this->MapperRaycast->Delete();
    this->MapperRaycast = NULL;
  }
  if (this->MapperGPURaycast3)
  {
    this->MapperGPURaycast3->RemoveObservers(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
    this->MapperGPURaycast3->Delete();
    this->MapperGPURaycast3 = NULL;
  }
  if (this->Volume)
  {
    this->Volume->Delete();
    this->Volume = NULL;
  }
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

  this->MapperRaycast = vtkSlicerFixedPointVolumeRayCastMapper::New();

  this->MapperGPURaycast3 = vtkGPUVolumeRayCastMapper::New();
  
  //create instance of the actor
  this->Volume = vtkVolume::New();

  this->Histograms = vtkKWHistogramSet::New();
  this->HistogramsFg = vtkKWHistogramSet::New();

  this->MapperTexture->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);

  //cpu ray casting
  this->MapperRaycast->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
  this->MapperRaycast->AddObserver(vtkCommand::ProgressEvent, this->GUICallback);

  //hook up the gpu mapper
  this->MapperGPURaycast->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);

  this->MapperGPURaycastII->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
  this->MapperGPURaycast3->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, this->GUICallback);
}

vtkMRMLVolumeRenderingParametersNode* vtkVolumeRenderingLogic::CreateParametersNode()
{
  vtkMRMLVolumeRenderingParametersNode *node = NULL;

  if (this->MRMLScene)
  {
    node = vtkMRMLVolumeRenderingParametersNode::New();
    this->MRMLScene->AddNode(node);
    node->Delete();
  }

  return node;
}

vtkMRMLVolumeRenderingScenarioNode* vtkVolumeRenderingLogic::CreateScenarioNode()
{
  vtkMRMLVolumeRenderingScenarioNode *node = NULL;

  if (this->MRMLScene)
  {
    node = vtkMRMLVolumeRenderingScenarioNode::New();
    this->MRMLScene->AddNode(node);
    node->Delete();
  }

  return node;
}

void vtkVolumeRenderingLogic::SetupHistograms(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData();
  if (input == NULL)
    {
    return;
    }

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
}

void vtkVolumeRenderingLogic::SetupHistogramsFg(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetFgVolumeNode())->GetImageData();

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
}

void vtkVolumeRenderingLogic::UpdateVolumePropertyScalarRange(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  if (vspNode == NULL)
    {
    return;
    }
  vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData();
  vtkMRMLVolumePropertyNode* propNode = vspNode->GetVolumePropertyNode();
  vtkVolumeProperty *prop =
    propNode ? propNode->GetVolumeProperty() : NULL;
  if (input == NULL || prop == NULL)
    {
    return;
    }

  //update scalar range
  vtkColorTransferFunction *functionColor = prop->GetRGBTransferFunction();

  double rangeNew[2];
  input->GetPointData()->GetScalars()->GetRange(rangeNew);
  functionColor->AdjustRange(rangeNew);

  vtkPiecewiseFunction *functionOpacity = prop->GetScalarOpacity();
  functionOpacity->AdjustRange(rangeNew);

  rangeNew[1] = (rangeNew[1] - rangeNew[0])*0.25;
  rangeNew[0] = 0;

  functionOpacity = prop->GetGradientOpacity();
  functionOpacity->RemovePoint(255);//Remove the standard value
  functionOpacity->AdjustRange(rangeNew);
}

void vtkVolumeRenderingLogic::UpdateFgVolumePropertyScalarRange(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetFgVolumeNode())->GetImageData();
  vtkVolumeProperty *prop = vspNode->GetFgVolumePropertyNode()->GetVolumeProperty();

  //update scalar range
  vtkColorTransferFunction *functionColor = prop->GetRGBTransferFunction();

  double rangeNew[2];
  input->GetPointData()->GetScalars()->GetRange(rangeNew);
  functionColor->AdjustRange(rangeNew);

  vtkPiecewiseFunction *functionOpacity = prop->GetScalarOpacity();
  functionOpacity->AdjustRange(rangeNew);

  rangeNew[1] = (rangeNew[1] - rangeNew[0])*0.25;
  rangeNew[0] = 0;

  functionOpacity = prop->GetGradientOpacity();
  functionOpacity->RemovePoint(255);//Remove the standard value
  functionOpacity->AdjustRange(rangeNew);
}

void vtkVolumeRenderingLogic::UpdateVolumePropertyByDisplayNode(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  char histogramName[1024];
  vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData();
  this->Histograms->ComputeHistogramName(input->GetPointData()->GetScalars()->GetName(), 0, 0, histogramName);

  //add points into transfer functions
  vtkKWHistogram *histogram = this->Histograms->GetHistogramWithName(histogramName);

  double range[2];
  histogram->GetRange(range);
  
  vtkMRMLScalarVolumeDisplayNode *vdNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(vspNode->GetVolumeNode()->GetDisplayNode());
  
  double windowLevel[2];
  windowLevel[0] = vdNode->GetWindow();
  windowLevel[1] = vdNode->GetLevel();

  vspNode->SetWindowLevel(windowLevel);

  double threshold[2];
  threshold[0] = vdNode->GetLowerThreshold();
  threshold[1] = vdNode->GetUpperThreshold();

  //when volumes module is in auto threshold mode, the value of threshold is invalid
  threshold[0] = threshold[0] < range[0] ? range[0] : threshold[0];
  threshold[1] = threshold[1] > range[1] ? range[1] : threshold[1];
  
  vspNode->SetThreshold(threshold);
  
  vtkMRMLVolumePropertyNode* vpNode = vspNode ? vspNode->GetVolumePropertyNode() : NULL;
  vtkVolumeProperty *prop = vpNode ? vpNode->GetVolumeProperty() : NULL;
  if (prop == NULL)
    {
    return;
    }
  prop->SetInterpolationTypeToLinear();

  vtkPiecewiseFunction *opacity = prop->GetScalarOpacity();

  double step = (range[1] - range[0]) * 0.001;
  
  opacity->RemoveAllPoints();
  opacity->AddPoint(range[0], 0.0);
  opacity->AddPoint(threshold[0], 0.0);
  opacity->AddPoint(threshold[0] + step, 1.0);
  opacity->AddPoint(threshold[1] - step, 1.0);
  opacity->AddPoint(threshold[1], 0.0);
  opacity->AddPoint(range[1], 0.0);
    
  vtkColorTransferFunction *colorTransfer = prop->GetRGBTransferFunction();

  colorTransfer->RemoveAllPoints();

  vtkLookupTable* pLut = vdNode->GetColorNode()->GetLookupTable();

  if (pLut == NULL)
  {
    colorTransfer->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
    colorTransfer->AddRGBPoint(windowLevel[1] - windowLevel[0]*0.5, 0.0, 0.0, 0.0);
    colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, 1.0, 1.0, 1.0);
    colorTransfer->AddRGBPoint(range[1], 1.0, 1.0, 1.0);
  }
  else
  {
    int size = pLut->GetNumberOfTableValues();

    double color[4];
    pLut->GetTableValue(0, color);
  
    if (size == 1)
    {
      colorTransfer->AddRGBPoint(range[0], color[0], color[1], color[2]);
      colorTransfer->AddRGBPoint(windowLevel[1] - windowLevel[0]*0.5, color[0], color[1], color[2]);
      colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, color[0], color[1], color[2]);
      colorTransfer->AddRGBPoint(range[1], color[0], color[1], color[2]);
    }
    else
    {
      colorTransfer->AddRGBPoint(range[0], color[0], color[1], color[2]);
    
      double value = windowLevel[1] - windowLevel[0]*0.5;

      step = windowLevel[0] / (size - 1);

      int downSamplingFactor = 64;
      
      for (int i = 0; i < size; i += downSamplingFactor, value += downSamplingFactor*step)
      {
        pLut->GetTableValue(i, color);
        colorTransfer->AddRGBPoint(value, color[0], color[1], color[2]);
      }

      pLut->GetTableValue(size - 1, color);
      colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, color[0], color[1], color[2]);
      colorTransfer->AddRGBPoint(range[1], color[0], color[1], color[2]);
    }
  }
  
  prop->ShadeOn();
  prop->SetAmbient(0.30);
  prop->SetDiffuse(0.60);
  prop->SetSpecular(0.50);
  prop->SetSpecularPower(40);
}

void vtkVolumeRenderingLogic::SetupVolumePropertyFromImageData(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  this->UpdateVolumePropertyScalarRange(vspNode);
  this->SetupHistograms(vspNode);

  if (vspNode->GetFollowVolumeDisplayNode())
  {
    this->UpdateVolumePropertyByDisplayNode(vspNode);
  }
  else if (vspNode->GetVolumeNode())
  {
    vtkMRMLScalarVolumeDisplayNode *vpNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(vspNode->GetVolumeNode()->GetDisplayNode());
    vtkMRMLLabelMapVolumeDisplayNode *vlNode = vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(vspNode->GetVolumeNode()->GetDisplayNode());
    
    if (vpNode)
      {
      char histogramName[1024];
      vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData();
      this->Histograms->ComputeHistogramName(input->GetPointData()->GetScalars()->GetName(), 0, 0, histogramName);

      //add points into transfer functions
      vtkKWHistogram *histogram = this->Histograms->GetHistogramWithName(histogramName);

      double range[2];
      histogram->GetRange(range);
          
      double windowLevel[2];
      windowLevel[0] = vpNode->GetWindow();
      windowLevel[1] = vpNode->GetLevel();

      vspNode->SetWindowLevel(windowLevel);
      
      vtkVolumeProperty *prop = vspNode->GetVolumePropertyNode()->GetVolumeProperty();
      prop->SetInterpolationTypeToLinear();

      vtkPiecewiseFunction *opacity = prop->GetScalarOpacity();
      
      opacity->RemoveAllPoints();
      opacity->AddPoint(range[0], 0.0);
      opacity->AddPoint(windowLevel[1] - windowLevel[0]*0.5, 0.0);
      opacity->AddPoint(windowLevel[1] + windowLevel[0]*0.5, 1.0);
      opacity->AddPoint(range[1], 1.0);
        
      vtkColorTransferFunction *colorTransfer = prop->GetRGBTransferFunction();

      colorTransfer->RemoveAllPoints();

      vtkLookupTable* pLut = vpNode->GetColorNode()->GetLookupTable();

      if (pLut == NULL)
      {
        colorTransfer->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
        colorTransfer->AddRGBPoint(windowLevel[1] - windowLevel[0]*0.5, 0.0, 0.0, 0.0);
        colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, 1.0, 1.0, 1.0);
        colorTransfer->AddRGBPoint(range[1], 1.0, 1.0, 1.0);
      }
      else
      {
        int size = pLut->GetNumberOfTableValues();

        double color[4];
        pLut->GetTableValue(0, color);
      
        if (size == 1)
        {
          colorTransfer->AddRGBPoint(range[0], color[0], color[1], color[2]);
          colorTransfer->AddRGBPoint(windowLevel[1] - windowLevel[0]*0.5, color[0], color[1], color[2]);
          colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, color[0], color[1], color[2]);
          colorTransfer->AddRGBPoint(range[1], color[0], color[1], color[2]);
        }
        else
        {
          colorTransfer->AddRGBPoint(range[0], color[0], color[1], color[2]);
        
          double value = windowLevel[1] - windowLevel[0]*0.5;
          double step;

          step = windowLevel[0] / (size - 1);

          int downSamplingFactor = 64;
          
          for (int i = 0; i < size; i += downSamplingFactor, value += downSamplingFactor*step)
          {
            pLut->GetTableValue(i, color);
            colorTransfer->AddRGBPoint(value, color[0], color[1], color[2]);
          }

          pLut->GetTableValue(size - 1, color);
          colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, color[0], color[1], color[2]);
          colorTransfer->AddRGBPoint(range[1], color[0], color[1], color[2]);
        }
      }
      
      prop->ShadeOn();
      prop->SetAmbient(0.30);
      prop->SetDiffuse(0.60);
      prop->SetSpecular(0.50);
      prop->SetSpecularPower(40);
    }
  else if (vlNode)
    {
    //TODO label map
    } 
  }

}

void vtkVolumeRenderingLogic::SetupFgVolumePropertyFromImageData(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  this->UpdateFgVolumePropertyScalarRange(vspNode);
  this->SetupHistogramsFg(vspNode);

  if (vspNode->GetFgVolumeNode())
  {
    vtkMRMLScalarVolumeDisplayNode *vpNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(vspNode->GetFgVolumeNode()->GetDisplayNode());
    vtkMRMLLabelMapVolumeDisplayNode *vlNode = vtkMRMLLabelMapVolumeDisplayNode::SafeDownCast(vspNode->GetFgVolumeNode()->GetDisplayNode());
    
    if (vpNode)
    {
      char histogramName[1024];
      vtkImageData *input = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData();
      this->Histograms->ComputeHistogramName(input->GetPointData()->GetScalars()->GetName(), 0, 0, histogramName);

      //add points into transfer functions
      vtkKWHistogram *histogram = this->Histograms->GetHistogramWithName(histogramName);

      double range[2];
      histogram->GetRange(range);
          
      double windowLevel[2];
      windowLevel[0] = vpNode->GetWindow();
      windowLevel[1] = vpNode->GetLevel();

      vspNode->SetWindowLevelFg(windowLevel);
      
      vtkVolumeProperty *prop = vspNode->GetFgVolumePropertyNode()->GetVolumeProperty();
      prop->SetInterpolationTypeToLinear();

      vtkPiecewiseFunction *opacity = prop->GetScalarOpacity();
      
      opacity->RemoveAllPoints();
      opacity->AddPoint(range[0], 0.0);
      opacity->AddPoint(windowLevel[1] - windowLevel[0]*0.5, 0.0);
      opacity->AddPoint(windowLevel[1] + windowLevel[0]*0.5, 1.0);
      opacity->AddPoint(range[1], 1.0);
        
      vtkColorTransferFunction *colorTransfer = prop->GetRGBTransferFunction();

      colorTransfer->RemoveAllPoints();

      vtkLookupTable* pLut = vpNode->GetColorNode()->GetLookupTable();

      if (pLut == NULL)
      {
        colorTransfer->AddRGBPoint(range[0], 0.0, 0.0, 0.0);
        colorTransfer->AddRGBPoint(windowLevel[1] - windowLevel[0]*0.5, 0.0, 0.0, 0.0);
        colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, 1.0, 1.0, 1.0);
        colorTransfer->AddRGBPoint(range[1], 1.0, 1.0, 1.0);
      }
      else
      {
        int size = pLut->GetNumberOfTableValues();

        double color[4];
        pLut->GetTableValue(0, color);
      
        if (size == 1)
        {
          colorTransfer->AddRGBPoint(range[0], color[0], color[1], color[2]);
          colorTransfer->AddRGBPoint(windowLevel[1] - windowLevel[0]*0.5, color[0], color[1], color[2]);
          colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, color[0], color[1], color[2]);
          colorTransfer->AddRGBPoint(range[1], color[0], color[1], color[2]);
        }
        else
        {
          colorTransfer->AddRGBPoint(range[0], color[0], color[1], color[2]);
        
          double value = windowLevel[1] - windowLevel[0]*0.5;
          double step;

          step = windowLevel[0] / (size - 1);

          int downSamplingFactor = 64;
          
          for (int i = 0; i < size; i += downSamplingFactor, value += downSamplingFactor*step)
          {
            pLut->GetTableValue(i, color);
            colorTransfer->AddRGBPoint(value, color[0], color[1], color[2]);
          }

          pLut->GetTableValue(size - 1, color);
          colorTransfer->AddRGBPoint(windowLevel[1] + windowLevel[0]*0.5, color[0], color[1], color[2]);
          colorTransfer->AddRGBPoint(range[1], color[0], color[1], color[2]);
        }
      }
      
      prop->ShadeOn();
      prop->SetAmbient(0.30);
      prop->SetDiffuse(0.60);
      prop->SetSpecular(0.50);
      prop->SetSpecularPower(40);
    }
    else if (vlNode)
    {
    //TODO label map
    } 
  }
}

void vtkVolumeRenderingLogic::ComputeInternalVolumeSize(int memory)
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

void vtkVolumeRenderingLogic::CalculateMatrix(vtkMRMLVolumeRenderingParametersNode *vspNode, vtkMatrix4x4 *output)
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

void vtkVolumeRenderingLogic::SetExpectedFPS(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  float fps;
  if (vspNode->GetExpectedFPS() == 0)
    fps = 0.001;
  else
    fps = vspNode->GetExpectedFPS();

  this->MapperTexture->SetFramerate(fps);
  this->MapperGPURaycast->SetFramerate(fps);
  this->MapperGPURaycastII->SetFramerate(fps);
  //this->MapperGPURaycast3->SetFramerate(fps);
}

void vtkVolumeRenderingLogic::SetGPUMemorySize(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  this->ComputeInternalVolumeSize(vspNode->GetGPUMemorySize());
}

void vtkVolumeRenderingLogic::SetCPURaycastParameters(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  if (vspNode->GetCPURaycastMode())
    this->MapperRaycast->SetBlendModeToMaximumIntensity();
  else
    this->MapperRaycast->SetBlendModeToComposite();
}

void vtkVolumeRenderingLogic::SetGPURaycastParameters(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  this->MapperGPURaycast->SetDepthPeelingThreshold(vspNode->GetDepthPeelingThreshold());
  this->MapperGPURaycast->SetDistanceColorBlending(vspNode->GetDistanceColorBlending());
  this->MapperGPURaycast->SetICPEScale(vspNode->GetICPEScale());
  this->MapperGPURaycast->SetICPESmoothness(vspNode->GetICPESmoothness());
  this->MapperGPURaycast->SetTechnique(vspNode->GetGPURaycastTechnique());
}

void vtkVolumeRenderingLogic::SetGPURaycastIIParameters(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  this->MapperGPURaycastII->SetFgBgRatio(vspNode->GetGPURaycastIIBgFgRatio());//ratio may not be used depending on techniques selected
  this->MapperGPURaycastII->SetTechniques(vspNode->GetGPURaycastTechniqueII(), vspNode->GetGPURaycastTechniqueIIFg());
  this->MapperGPURaycastII->SetColorOpacityFusion(vspNode->GetGPURaycastIIFusion());
}

void vtkVolumeRenderingLogic::SetGPURaycast3Parameters(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  switch(vspNode->GetGPURaycastTechnique3())
    {
    default:
    case 0:
      this->MapperGPURaycast3->SetBlendMode(vtkVolumeMapper::COMPOSITE_BLEND);
      break;
    case 1:
      this->MapperGPURaycast3->SetBlendMode(vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND);
      break;
    case 2:
      this->MapperGPURaycast3->SetBlendMode(vtkVolumeMapper::MINIMUM_INTENSITY_BLEND);
      break;
    }
}

void vtkVolumeRenderingLogic::EstimateSampleDistance(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
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

int vtkVolumeRenderingLogic::IsCurrentMapperSupported(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  if (vspNode == NULL)
    {
    return 0;
    }

  vtkRenderWindow* window = vtkSlicerApplication::GetInstance()
    ->GetApplicationGUI()->GetActiveViewerWidget()->GetMainViewer()
    ->GetRenderWindow();
  vtkMRMLVolumePropertyNode* vpNode = vspNode ? vspNode->GetVolumePropertyNode() : NULL;
  vtkVolumeProperty* vProp = vpNode ? vpNode->GetVolumeProperty() : NULL;

  switch(vspNode->GetCurrentVolumeMapper())//mapper specific initialization
  {
  case 0:
    return 1;
  case 3:
    {
      vtkSlicerGPURayCastVolumeMapper* MapperGPURaycast = vtkSlicerGPURayCastVolumeMapper::New();

      MapperGPURaycast->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );
      
      if (MapperGPURaycast->IsRenderSupported(window,vProp))
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
      
      if (MapperGPURaycastII->IsRenderSupported(window, vProp))
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

      if (MapperTexture->IsRenderSupported(window, vProp))
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
    if (VTKGPURaycast->IsRenderSupported(window, vProp))
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
int vtkVolumeRenderingLogic::SetupMapperFromParametersNode(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  if (vspNode == NULL)
    return 0;

  this->Volume->SetMapper(NULL);
  this->EstimateSampleDistance(vspNode);

  vtkRenderWindow* window = vtkSlicerApplication::GetInstance()
    ->GetApplicationGUI()->GetActiveViewerWidget()->GetMainViewer()
    ->GetRenderWindow();
  
  vtkMRMLVolumePropertyNode* vpNode = vspNode ? vspNode->GetVolumePropertyNode() : NULL;
  vtkVolumeProperty* volProp = vpNode ? vpNode->GetVolumeProperty() : NULL;

  switch(vspNode->GetCurrentVolumeMapper())//mapper specific initialization
  {
  case 0:
    this->MapperRaycast->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );
    this->MapperRaycast->SetSampleDistance(vspNode->GetEstimatedSampleDistance());
    this->Volume->SetMapper(this->MapperRaycast);
    this->Volume->SetProperty(volProp);
    break;
  case 3:
    this->MapperGPURaycast->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );
    this->MapperGPURaycast->SetFramerate(vspNode->GetExpectedFPS());
    this->Volume->SetProperty(volProp);
    if (this->MapperGPURaycast->IsRenderSupported(window,volProp))
      {
      this->Volume->SetMapper(this->MapperGPURaycast);
      }
    else
      {
      return -1;
      }
    break;
  case 4:
    this->MapperGPURaycastII->SetNthInput(0, vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData());
    if (vspNode->GetFgVolumeNode())
      this->MapperGPURaycastII->SetNthInput(1, vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetFgVolumeNode())->GetImageData());
    this->MapperGPURaycastII->SetFramerate(vspNode->GetExpectedFPS());
    this->CreateVolumePropertyGPURaycastII(vspNode);
    this->Volume->SetProperty(this->VolumePropertyGPURaycastII);
    if (this->MapperGPURaycastII->IsRenderSupported(window, volProp))
      {
      this->Volume->SetMapper(this->MapperGPURaycastII);
      }
    else
      {
      return -1;
      }
    break;
  case 2:
    this->MapperTexture->SetInput( vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData() );
    this->MapperTexture->SetSampleDistance(vspNode->GetEstimatedSampleDistance());
    this->MapperTexture->SetFramerate(vspNode->GetExpectedFPS());
    this->Volume->SetProperty(volProp);
    if (this->MapperTexture->IsRenderSupported(window, volProp))
      {
      this->Volume->SetMapper(this->MapperTexture);
      }
    else
      {
      return -1;
      }
    break;
  case 1:
    this->MapperGPURaycast3->SetInput(vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode())->GetImageData());
    this->MapperGPURaycast3->SetSampleDistance(vspNode->GetEstimatedSampleDistance());
    //this->MapperGPURaycast3->SetFramerate(vspNode->GetExpectedFPS());
    this->Volume->SetProperty(volProp);
    if (this->MapperGPURaycast3->IsRenderSupported(window, volProp))
      {
      this->Volume->SetMapper(this->MapperGPURaycast3);
      //this->CreateVolumePropertyGPURaycast3(vspNode);
      //this->Volume->SetProperty(this->VolumePropertyGPURaycast3);
      }
    else
      {
      return -1;
      }
    break;
  }

  this->SetExpectedFPS(vspNode);
  this->ComputeInternalVolumeSize(vspNode->GetGPUMemorySize());

  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  this->CalculateMatrix(vspNode, matrix);
  this->Volume->PokeMatrix(matrix);
  matrix->Delete();

  return 1;
}

/* return values:
 * 0: vtk gpu ray cast mapper used
 * 1: success
 */
int vtkVolumeRenderingLogic::SetupVolumeRenderingInteractive(vtkMRMLVolumeRenderingParametersNode* vspNode, int buttonDown)
{
  if (vspNode->GetCurrentVolumeMapper() == 1)//vtk gpu mapper has different defination of interaction
    return 0;

  //when start (rendering??) set CPU ray casting to be interactive
  if (buttonDown == 1 && vspNode->GetExpectedFPS() > 0)
  {
    float desiredTime = 1.0f/vspNode->GetExpectedFPS();

    this->MapperRaycast->SetAutoAdjustSampleDistances(1);
    this->MapperRaycast->ManualInteractiveOn();
    this->MapperRaycast->SetManualInteractiveRate(desiredTime);

    this->MapperGPURaycast->SetFramerate(vspNode->GetExpectedFPS());
    this->MapperGPURaycastII->SetFramerate(vspNode->GetExpectedFPS());
    this->MapperTexture->SetFramerate(vspNode->GetExpectedFPS());
  }
  else
  {
    //when end (rendering??) set CPU ray casting to be non-interactive high quality
    this->MapperRaycast->SetAutoAdjustSampleDistances(0);
    this->MapperRaycast->SetSampleDistance(vspNode->GetEstimatedSampleDistance());
    this->MapperRaycast->SetImageSampleDistance(1.0f);
    this->MapperRaycast->ManualInteractiveOff();

    this->MapperGPURaycast->SetFramerate(1.0);
    this->MapperGPURaycastII->SetFramerate(1.0);
    this->MapperTexture->SetFramerate(1.0);
  }

  return 1;
}

void vtkVolumeRenderingLogic::SetVolumeVisibility(int isVisible)
{
  if (isVisible)
    this->Volume->VisibilityOn();
  else
    this->Volume->VisibilityOff();
}

void vtkVolumeRenderingLogic::FitROIToVolume(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  // resize the ROI to fit the volume
  vtkMRMLROINode *roiNode = vtkMRMLROINode::SafeDownCast(vspNode->GetROINode());
  vtkMRMLScalarVolumeNode *volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(vspNode->GetVolumeNode());

  if (volumeNode && roiNode)
  {
    double xyz[3];
    double center[3];

    vtkMRMLSliceLogic::GetVolumeRASBox(volumeNode, xyz,  center);
    for (int i = 0; i < 3; i++)
    {
      xyz[i] *= 0.5;
    }

    roiNode->SetXYZ(center);
    roiNode->SetRadiusXYZ(xyz);
  }
}

void vtkVolumeRenderingLogic::SetROI(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  if (vspNode->GetROINode() == NULL)
    return;

  this->MapperTexture->RemoveAllClippingPlanes();
  this->MapperRaycast->RemoveAllClippingPlanes();
  this->MapperGPURaycast->RemoveAllClippingPlanes();
  this->MapperGPURaycast->ClippingOff();
  this->MapperGPURaycastII->RemoveAllClippingPlanes();
  this->MapperGPURaycastII->ClippingOff();
  this->MapperGPURaycast3->RemoveAllClippingPlanes();

  if (vspNode->GetCroppingEnabled())
  {
    vtkPlanes *planes = vtkPlanes::New();
    vspNode->GetROINode()->GetTransformedPlanes(planes);
    
    this->MapperTexture->SetClippingPlanes(planes);
    this->MapperRaycast->SetClippingPlanes(planes);

    this->MapperGPURaycast->SetClippingPlanes(planes);
    this->MapperGPURaycast->ClippingOn();

    this->MapperGPURaycastII->SetClippingPlanes(planes);
    this->MapperGPURaycastII->ClippingOn();
    
    this->MapperGPURaycast3->SetClippingPlanes(planes);

    planes->Delete();
  }
}

void vtkVolumeRenderingLogic::TransformModified(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  this->CalculateMatrix(vspNode, matrix);
  this->Volume->PokeMatrix(matrix);

  this->FitROIToVolume(vspNode);
}

void vtkVolumeRenderingLogic::UpdateVolumePropertyGPURaycastII(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  if (vspNode->GetCurrentVolumeMapper() == 4)
  {
    this->CreateVolumePropertyGPURaycastII(vspNode);
    this->Volume->SetProperty(this->VolumePropertyGPURaycastII);
  }
}

void vtkVolumeRenderingLogic::CreateVolumePropertyGPURaycastII(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  if (vspNode->GetCurrentVolumeMapper() != 4)
    return;

  if (this->VolumePropertyGPURaycastII != NULL)
    this->VolumePropertyGPURaycastII->Delete();

  vtkMRMLVolumePropertyNode* vpNode = vspNode ? vspNode->GetVolumePropertyNode() : NULL;
  vtkVolumeProperty* prop = vpNode ? vpNode->GetVolumeProperty() : NULL;
  if (prop == NULL)
    {
    this->VolumePropertyGPURaycastII = NULL;
    return;
    }

  this->VolumePropertyGPURaycastII = vtkVolumeProperty::New();

  //copy bg property into 1st compoent property
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
    vtkVolumeProperty* propFg = prop;//vspNode->GetVolumePropertyNode()->GetVolumeProperty();
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
void vtkVolumeRenderingLogic::UpdateVolumePropertyGPURaycast3(vtkMRMLVolumeRenderingParametersNode* vspNode)
{
  if (vspNode->GetCurrentVolumeMapper() == 5)
  {
    this->CreateVolumePropertyGPURaycast3(vspNode);
    this->Volume->SetProperty(this->VolumePropertyGPURaycast3);
  }
}

void vtkVolumeRenderingLogic::CreateVolumePropertyGPURaycast3(vtkMRMLVolumeRenderingParametersNode* vspNode)
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
void vtkVolumeRenderingLogic::ProcessMRMLEvents(vtkObject *vtkNotUsed(caller),
                                                unsigned long vtkNotUsed(event),
                                                void *vtkNotUsed(callData))
{
}




//----------------------------------------------------------------------------
vtkMRMLVolumePropertyNode* vtkVolumeRenderingLogic::AddVolumePropertyFromFile (const char* filename)
{
  vtkMRMLVolumePropertyNode *vpNode = vtkMRMLVolumePropertyNode::New();
  vtkMRMLVolumePropertyStorageNode *vpStorageNode = vtkMRMLVolumePropertyStorageNode::New();

  // check for local or remote files
  int useURI = 0; // false;
  if (this->GetMRMLScene()->GetCacheManager() != NULL)
    {
    useURI = this->GetMRMLScene()->GetCacheManager()->IsRemoteReference(filename);
    }
  
  itksys_stl::string name;
  const char *localFile;
  if (useURI)
    {
    vpStorageNode->SetURI(filename);
     // reset filename to the local file name
    localFile = ((this->GetMRMLScene())->GetCacheManager())->GetFilenameFromURI(filename);
    }
  else
    {
    vpStorageNode->SetFileName(filename);
    localFile = filename;
    }
  const itksys_stl::string fname(localFile);
  // the model name is based on the file name (itksys call should work even if
  // file is not on disk yet)
  name = itksys::SystemTools::GetFilenameName(fname);
  
  // check to see which node can read this type of file
  if (!vpStorageNode->SupportedFileType(name.c_str()))
    {
    vpStorageNode->Delete();
    vpStorageNode = NULL;
    }

  /* don't read just yet, need to add to the scene first for remote reading
  if (vpStorageNode->ReadData(vpNode) != 0)
    {
    storageNode = vpStorageNode;
    }
  */
  if (vpStorageNode != NULL)
    {
    std::string uname( this->MRMLScene->GetUniqueNameByString(name.c_str()));

    vpNode->SetName(uname.c_str());

    this->GetMRMLScene()->SaveStateForUndo();

    vpNode->SetScene(this->GetMRMLScene());
    vpStorageNode->SetScene(this->GetMRMLScene());

    this->GetMRMLScene()->AddNodeNoNotify(vpStorageNode);  
    vpNode->SetAndObserveStorageNodeID(vpStorageNode->GetID());

    this->GetMRMLScene()->AddNode(vpNode);  

    //this->Modified();  

    // the scene points to it still
    vpNode->Delete();

    // now set up the reading
    int retval = vpStorageNode->ReadData(vpNode);
    if (retval != 1)
      {
      vtkErrorMacro("AddVolumePropertyFromFile: error reading " << filename);
      this->GetMRMLScene()->RemoveNode(vpNode);
      this->GetMRMLScene()->RemoveNode(vpStorageNode);
      vpNode = NULL;
      }
    }
  else
    {
    vtkDebugMacro("Couldn't read file, returning null model node: " << filename);
    vpNode->Delete();
    vpNode = NULL;
    }
  if (vpStorageNode)
    {
    vpStorageNode->Delete();
    }
  return vpNode;  
}
