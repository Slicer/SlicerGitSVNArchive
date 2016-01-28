/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Andras Lasso, PerkLab, Queen's University.

==============================================================================*/

// MRMLDisplayableManager includes
#include "vtkMRMLOrientationMarkerDisplayableManager.h"

// MRML includes
#include <vtkMRMLAbstractViewNode.h>
#include <vtkMRMLLogic.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkAxesActor.h>
#include <vtkCamera.h>
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>
#include <vtksys/SystemTools.hxx>

// STD includes

// Constants
static const int RENDERER_LAYER = 1; // layer ID where the orientation marker will be displayed
static const char* AXES_LABELS[] = {"L", "R", "P", "A", "I", "S"};
static const char ORIENTATION_MARKERS_DIR[] = "OrientationMarkers";
static const char HUMAN_MODEL_VTP_FILENAME[] = "Human.vtp";

//---------------------------------------------------------------------------
class vtkRendererUpdateObserver : public vtkCommand
{
public:
  static vtkRendererUpdateObserver *New()
    {
    return new vtkRendererUpdateObserver;
    }
  vtkRendererUpdateObserver()
    {
    this->DisplayableManager = 0;
    }
  virtual void Execute(vtkObject* vtkNotUsed(wdg), unsigned long vtkNotUsed(event), void* vtkNotUsed(calldata))
    {
    if (this->DisplayableManager)
      {
      this->DisplayableManager->UpdateFromRenderer();
      }
  }
  vtkWeakPointer<vtkMRMLOrientationMarkerDisplayableManager> DisplayableManager;
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMRMLOrientationMarkerDisplayableManager );

//---------------------------------------------------------------------------
class vtkMRMLOrientationMarkerDisplayableManager::vtkInternal
{
public:
  vtkInternal(vtkMRMLOrientationMarkerDisplayableManager * external);
  ~vtkInternal();

  void SetupMarkerRenderer();
  void AddRendererUpdateObserver(vtkRenderer* renderer);
  void RemoveRendererUpdateObserver();

  vtkProp3D* GetCubeActor();
  vtkProp3D* GetHumanActor();
  vtkProp3D* GetAxesActor();

  void UpdateMarkerType();
  void UpdateMarkerSize();
  void UpdateMarkerOrientation();

  std::string GetOrientationMarkerModelPath(const char* modelFileName);

  vtkSmartPointer<vtkRenderer> MarkerRenderer;

  vtkSmartPointer<vtkAnnotatedCubeActor> CubeActor;
  vtkSmartPointer<vtkActor> HumanActor;
  vtkSmartPointer<vtkAxesActor> AxesActor;
  vtkProp3D* DisplayedActor;

  // Keep a reference to it to allow modifying polydata input
  vtkSmartPointer<vtkPolyData> HumanPolyData;
  vtkSmartPointer<vtkPolyDataMapper> HumanPolyDataMapper;

  vtkSmartPointer<vtkRendererUpdateObserver> RendererUpdateObserver;
  int RendererUpdateObservationId;
  vtkWeakPointer<vtkRenderer> ObservedRenderer;

  vtkMRMLOrientationMarkerDisplayableManager* External;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::vtkInternal(vtkMRMLOrientationMarkerDisplayableManager * external)
{
  this->External = external;
  this->RendererUpdateObserver = vtkSmartPointer<vtkRendererUpdateObserver>::New();
  this->RendererUpdateObserver->DisplayableManager = this->External;
  this->RendererUpdateObservationId = 0;
  this->DisplayedActor = NULL;
  this->MarkerRenderer = vtkSmartPointer<vtkRenderer>::New();
  this->HumanPolyData = vtkSmartPointer<vtkPolyData>::New();
  this->HumanPolyDataMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  // Orientation marker actors are not created here to converve resources
  // (especially the human marker may be expensive). These actors are created
  // when they are first needed.
}

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::~vtkInternal()
{
  RemoveRendererUpdateObserver();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::AddRendererUpdateObserver(vtkRenderer* renderer)
{
  RemoveRendererUpdateObserver();
  if (renderer)
    {
    this->ObservedRenderer = renderer;
    this->RendererUpdateObservationId = renderer->AddObserver(vtkCommand::StartEvent, this->RendererUpdateObserver);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::RemoveRendererUpdateObserver()
{
  if (this->ObservedRenderer)
    {
    this->ObservedRenderer->RemoveObserver(this->RendererUpdateObservationId);
    this->RendererUpdateObservationId = 0;
    this->ObservedRenderer = NULL;
    }
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::SetupMarkerRenderer()
{
  vtkRenderer* renderer = this->External->GetRenderer();
  if (renderer==NULL)
    {
    vtkErrorWithObjectMacro(this->External, "vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::SetupMarkerRenderer() failed: renderer is invalid");
    return;
    }

  this->MarkerRenderer->InteractiveOff();

  vtkRenderWindow* renderWindow = renderer->GetRenderWindow();
  if (renderWindow->GetNumberOfLayers() < RENDERER_LAYER+1)
    {
    renderWindow->SetNumberOfLayers( RENDERER_LAYER+1 );
    }
  this->MarkerRenderer->SetLayer(RENDERER_LAYER);
  renderWindow->AddRenderer(this->MarkerRenderer);

  // In 3D viewers we need to follow the renderer and update the orientation marker accordingly
  vtkMRMLViewNode* threeDViewNode = vtkMRMLViewNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (threeDViewNode)
    {
    this->AddRendererUpdateObserver(renderer);
    }
}


//---------------------------------------------------------------------------
vtkProp3D* vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::GetCubeActor()
{
  if (!this->CubeActor)
    {
    this->CubeActor = vtkSmartPointer<vtkAnnotatedCubeActor>::New();
    this->CubeActor->SetXMinusFaceText(AXES_LABELS[0]);
    this->CubeActor->SetXPlusFaceText(AXES_LABELS[1]);
    this->CubeActor->SetYMinusFaceText(AXES_LABELS[2]);
    this->CubeActor->SetYPlusFaceText(AXES_LABELS[3]);
    this->CubeActor->SetZMinusFaceText(AXES_LABELS[4]);
    this->CubeActor->SetZPlusFaceText(AXES_LABELS[5]);
    this->CubeActor->SetZFaceTextRotation(90);
    this->CubeActor->GetTextEdgesProperty()->SetColor(0.95,0.95,0.95);
    this->CubeActor->GetTextEdgesProperty()->SetLineWidth(2);
    this->CubeActor->GetCubeProperty()->SetColor(0.15,0.15,0.15);
    this->CubeActor->PickableOff();
    this->CubeActor->DragableOff();
    }
  return this->CubeActor;
}

//----------------------------------------------------------------------------
std::string vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::GetOrientationMarkerModelPath(const char* modelFileName)
{
  std::vector<std::string> filesVector;
  filesVector.push_back(""); // The first two components do not add a slash.
  filesVector.push_back(vtkMRMLLogic::GetApplicationShareDirectory());
  filesVector.push_back(ORIENTATION_MARKERS_DIR);
  filesVector.push_back(modelFileName);
  std::string fullPath = vtksys::SystemTools::JoinPath(filesVector);
  return fullPath;
}

//---------------------------------------------------------------------------
vtkProp3D* vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::GetHumanActor()
{
  if (!this->HumanActor)
    {
    vtkNew<vtkXMLPolyDataReader> polyDataReader;
    polyDataReader->SetFileName(this->GetOrientationMarkerModelPath(HUMAN_MODEL_VTP_FILENAME).c_str());
    polyDataReader->Update();
    this->HumanPolyData->ShallowCopy(polyDataReader->GetOutput());
    this->HumanPolyData->GetPointData()->SetActiveScalars("Color");

    this->HumanPolyDataMapper->SetInputData(this->HumanPolyData);
    this->HumanPolyDataMapper->SetColorModeToDirectScalars();

    this->HumanActor = vtkSmartPointer<vtkActor>::New();
    this->HumanActor->SetMapper(this->HumanPolyDataMapper);
    const double scale = 0.01;
    this->HumanActor->SetScale(scale,scale,scale);
    this->HumanActor->PickableOff();
    this->HumanActor->DragableOff();
    }
  return this->HumanActor;
}

//---------------------------------------------------------------------------
vtkProp3D* vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::GetAxesActor()
{
  if (!this->AxesActor)
    {
    this->AxesActor = vtkSmartPointer<vtkAxesActor>::New();
    this->AxesActor->SetXAxisLabelText(AXES_LABELS[1]);
    this->AxesActor->SetYAxisLabelText(AXES_LABELS[3]);
    this->AxesActor->SetZAxisLabelText(AXES_LABELS[5]);
    this->AxesActor->PickableOff();
    this->AxesActor->DragableOff();
    }
  return this->AxesActor;
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerType()
{
  vtkMRMLAbstractViewNode* viewNode = vtkMRMLAbstractViewNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (!viewNode || !viewNode->GetOrientationMarkerEnabled() || !this->MarkerRenderer)
    {
    return;
    }

  // Determine what actor to display
  vtkProp3D* actorToDisplay = NULL;
  switch (viewNode->GetOrientationMarkerType())
    {
    case vtkMRMLAbstractViewNode::OrientationMarkerTypeCube:
      actorToDisplay = this->GetCubeActor();
      break;
    case vtkMRMLAbstractViewNode::OrientationMarkerTypeHuman:
      actorToDisplay = this->GetHumanActor();
      break;
    case vtkMRMLAbstractViewNode::OrientationMarkerTypeAxes:
      actorToDisplay = this->GetAxesActor();
      break;
    case vtkMRMLAbstractViewNode::OrientationMarkerTypeNone:
    default:
      break;
    }

  // Display that actor
  if (this->DisplayedActor != actorToDisplay)
    {
    if (this->DisplayedActor != NULL)
      {
      this->MarkerRenderer->RemoveViewProp(this->DisplayedActor);
      }
    if (actorToDisplay != NULL)
      {
      this->MarkerRenderer->AddViewProp(actorToDisplay);
      }
    this->DisplayedActor = actorToDisplay;
    }

  if (viewNode->GetOrientationMarkerType() == vtkMRMLAbstractViewNode::OrientationMarkerTypeHuman)
    {
    vtkMRMLModelNode* humanModelNode = viewNode->GetOrientationMarkerHumanModelNode();
    if (humanModelNode && humanModelNode->GetPolyData())
      {
      vtkPolyData* polyData = humanModelNode->GetPolyData();
      this->HumanPolyDataMapper->SetInputData(polyData);
      if (polyData->GetPointData() && polyData->GetPointData()->HasArray("Color"))
        {
        polyData->GetPointData()->SetActiveScalars("Color");
        this->HumanPolyDataMapper->SetColorModeToDirectScalars();
        }
      else
        {
        this->HumanPolyDataMapper->SetScalarModeToDefault();
        vtkMRMLModelDisplayNode* displayNode = vtkMRMLModelDisplayNode::SafeDownCast(humanModelNode->GetDisplayNode());
        if (displayNode)
          {
          this->HumanActor->GetProperty()->SetColor(displayNode->GetColor());
          }
        else
          {
          this->HumanActor->GetProperty()->SetColor(1,1,1);
          }
        }
      }
    else if (this->HumanPolyDataMapper)
      {
      this->HumanPolyDataMapper->SetInputData(this->HumanPolyData);
      this->HumanPolyDataMapper->SetColorModeToDirectScalars();
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerOrientation()
{
  if (this->MarkerRenderer==NULL)
    {
    return;
    }
  vtkRenderer* renderer = this->External->GetRenderer();
  if (renderer==NULL)
    {
    vtkErrorWithObjectMacro(this->External, "vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerOrientation() failed: renderer is invalid");
    return;
    }
  vtkMRMLAbstractViewNode* viewNode = vtkMRMLAbstractViewNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (!viewNode || !viewNode->GetOrientationMarkerEnabled())
    {
    vtkErrorWithObjectMacro(this->External, "vtkMRMLOrientationMarkerDisplayableManager::UpdateMarkerOrientation() failed: displayable node is invalid");
    return;
    }
  if (viewNode->GetOrientationMarkerType() == vtkMRMLAbstractViewNode::OrientationMarkerTypeNone)
    {
    // not visible - no need for update
    return;
    }

  vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(viewNode);
  if (sliceNode)
    {
    // Calculate the camera position and viewup based on XYToRAS matrix
    vtkMatrix4x4* sliceToRas = sliceNode->GetSliceToRAS();
    vtkNew<vtkMatrix3x3> sliceToRasOrientation;
    for (int r=0; r<3; r++)
      {
      for (int c=0; c<3; c++)
        {
        sliceToRasOrientation->SetElement(r,c,sliceToRas->GetElement(r,c));
        }
      }
    double det = sliceToRasOrientation->Determinant();
    const double cameraDistance = 100.0; // any positive number works here, as the position will be adjusted at the end by ResetCamera()
    double y[3] = {0,0, det>0 ? -cameraDistance : cameraDistance};
    // Calculating camer position
    double position[3] = {0};
    sliceToRasOrientation->MultiplyPoint(y,position);
    // Calculating camera viewUp
    const double n[3] = {0,1,0};
    double viewUp[3] = {0};
    sliceToRasOrientation->MultiplyPoint(n,viewUp);

    vtkCamera* camera = this->MarkerRenderer->GetActiveCamera();
    camera->SetPosition(-position[0],-position[1],-position[2]);
    camera->SetViewUp(viewUp[0],viewUp[1],viewUp[2]);

    this->MarkerRenderer->ResetCamera();
    return;
    }

  vtkMRMLViewNode* threeDViewNode = vtkMRMLViewNode::SafeDownCast(viewNode);
  if (threeDViewNode && this->ObservedRenderer)
    {
    vtkCamera *cam = this->ObservedRenderer->GetActiveCamera();
    double pos[3], fp[3], viewup[3];
    cam->GetPosition( pos );
    cam->GetFocalPoint( fp );
    cam->GetViewUp( viewup );

    cam = this->MarkerRenderer->GetActiveCamera();
    cam->SetPosition( pos );
    cam->SetFocalPoint( fp );
    cam->SetViewUp( viewup );
    this->MarkerRenderer->ResetCamera();
    return;
    }

  vtkErrorWithObjectMacro(this->External, "vtkMRMLOrientationMarkerDisplayableManager::UpdateMarkerOrientation() failed: displayable node is invalid");
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerSize()
{
  if (this->MarkerRenderer==NULL)
    {
    vtkErrorWithObjectMacro(this->External, "vtkMRMLOrientationMarkerDisplayableManager::vtkInternal::UpdateMarkerSize() failed: MarkerRenderer is invalid");
    return;
    }
  vtkMRMLAbstractViewNode* viewNode = vtkMRMLAbstractViewNode::SafeDownCast(this->External->GetMRMLDisplayableNode());
  if (!viewNode || !viewNode->GetOrientationMarkerEnabled())
    {
    return;
    }
  if (viewNode->GetOrientationMarkerType() == vtkMRMLAbstractViewNode::OrientationMarkerTypeNone)
    {
    // not visible - no need for update
    return;
    }

  int sizePercent = 25;
  switch (viewNode->GetOrientationMarkerSize())
    {
    case vtkMRMLAbstractViewNode::OrientationMarkerSizeSmall: sizePercent=15; break;
    case vtkMRMLAbstractViewNode::OrientationMarkerSizeLarge: sizePercent=40; break;
    case vtkMRMLAbstractViewNode::OrientationMarkerSizeMedium:
    default:
      // keep default
      break;
    }

  // Viewport: xmin, ymin, xmax, ymax; range: 0.0-1.0; origin is bottom left
  double* viewport = this->MarkerRenderer->GetViewport();
  // Determine the available renderer size in pixels
  double minX = 0;
  double minY = 0;
  this->MarkerRenderer->NormalizedDisplayToDisplay(minX, minY);
  double maxX = 1;
  double maxY = 1;
  this->MarkerRenderer->NormalizedDisplayToDisplay(maxX, maxY);
  int rendererSizeInPixels[2] = {maxX-minX, maxY-minY};
  if (rendererSizeInPixels[0]>0 && rendererSizeInPixels[1]>0)
    {
    // Compute normalized size for a square-shaped viewport. Square size is defined a percentage of renderer height.
    double viewPortSizeInPixels = double(rendererSizeInPixels[1])*(0.01*sizePercent);
    double newViewport[4] =
      {
      1.0-viewPortSizeInPixels/double(rendererSizeInPixels[0]), 0.0,
      1.0, viewPortSizeInPixels/double(rendererSizeInPixels[1])
      };

    // Clip viewport to valid range
    if (newViewport[0]<0.0) newViewport[0] = 0.0;
    if (newViewport[1]<0.0) newViewport[1] = 0.0;
    if (newViewport[2]>1.0) newViewport[1] = 1.0;
    if (newViewport[3]>1.0) newViewport[3] = 1.0;

    // Update the viewport
    if (newViewport[0] != viewport[0] || newViewport[1] != viewport[1]
      || newViewport[2] != viewport[2] || newViewport[3] != viewport[3])
      {
      this->MarkerRenderer->SetViewport(newViewport);
      }
    }
}

//---------------------------------------------------------------------------
// vtkMRMLOrientationMarkerDisplayableManager methods

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::vtkMRMLOrientationMarkerDisplayableManager()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLOrientationMarkerDisplayableManager::~vtkMRMLOrientationMarkerDisplayableManager()
{
  delete this->Internal;
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::Create()
{
  this->Internal->SetupMarkerRenderer();
  this->Superclass::Create();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::UpdateFromViewNode()
{
  // View node is changed, which may mean that either the marker type (visibility), size, or orientation is changed
  this->Internal->UpdateMarkerType();
  this->Internal->UpdateMarkerSize();
  this->Internal->UpdateMarkerOrientation();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::OnMRMLDisplayableNodeModifiedEvent(vtkObject* vtkNotUsed(caller))
{
  // view node is changed
  this->UpdateFromViewNode();
}

//---------------------------------------------------------------------------
void vtkMRMLOrientationMarkerDisplayableManager::UpdateFromRenderer()
{
  // Rendering is performed, so let's re-render the marker with up-to-date orientation
  this->Internal->UpdateMarkerOrientation();
  this->Internal->UpdateMarkerSize(); // update size if render window size changes
}
