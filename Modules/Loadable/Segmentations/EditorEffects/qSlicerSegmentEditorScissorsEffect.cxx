/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "qSlicerSegmentEditorScissorsEffect.h"

#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkMRMLSegmentEditorNode.h"

// Qt includes
#include <QApplication>
#include <QDebug>
#include <QComboBox>

// VTK includes
#include <vtkActor2D.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkImageChangeInformation.h>
#include <vtkImageStencilData.h>
#include <vtkImageStencilToImage.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkProperty2D.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkVector.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkEventBroker.h>
#include "qMRMLSliceView.h"
#include "qMRMLThreeDView.h"

// Slicer includes
#include "qMRMLSliceWidget.h"
#include "qMRMLThreeDWidget.h"
#include "vtkMRMLSliceLayerLogic.h"
#include "vtkMRMLSliceLogic.h"

//-----------------------------------------------------------------------------
/// Visualization objects and pipeline for each slice view for drawing cutting outline
class ScissorsPipeline: public QObject
{
public:
  ScissorsPipeline()
    {
    this->IsDragging = false;
    this->PolyData = vtkSmartPointer<vtkPolyData>::New();
    this->Mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
    this->Mapper->SetInputData(this->PolyData);
    this->Actor = vtkSmartPointer<vtkActor2D>::New();
    this->Actor->SetMapper(this->Mapper);
    this->Actor->VisibilityOff();
    vtkProperty2D* outlineProperty = this->Actor->GetProperty();
    outlineProperty->SetColor(1,1,0);
    outlineProperty->SetLineWidth(2);

    this->PolyDataThin = vtkSmartPointer<vtkPolyData>::New();
    this->MapperThin = vtkSmartPointer<vtkPolyDataMapper2D>::New();
    this->MapperThin->SetInputData(this->PolyDataThin);
    this->ActorThin = vtkSmartPointer<vtkActor2D>::New();
    this->ActorThin->SetMapper(this->MapperThin);
    this->ActorThin->VisibilityOff();
    vtkProperty2D* outlinePropertyThin = this->ActorThin->GetProperty();
    // Make the line a bit thinner and darker to distinguish from the thick line
    outlinePropertyThin->SetColor(0.7, 0.7, 0);
    outlinePropertyThin->SetLineStipplePattern(0xff00); // Note: line stipple may not be supported in VTK OpenGL2 backend
    outlinePropertyThin->SetLineWidth(1);
    };
  ~ScissorsPipeline()
    {
    };
public:
  bool IsDragging;
  vtkSmartPointer<vtkActor2D> Actor;
  vtkSmartPointer<vtkPolyDataMapper2D> Mapper;
  vtkSmartPointer<vtkPolyData> PolyData;
  vtkSmartPointer<vtkActor2D> ActorThin;
  vtkSmartPointer<vtkPolyDataMapper2D> MapperThin;
  vtkSmartPointer<vtkPolyData> PolyDataThin;
};

//-----------------------------------------------------------------------------
class qSlicerSegmentEditorScissorsEffectPrivate: public QObject
{
  Q_DECLARE_PUBLIC(qSlicerSegmentEditorScissorsEffect);
protected:
  qSlicerSegmentEditorScissorsEffect* const q_ptr;
public:
  qSlicerSegmentEditorScissorsEffectPrivate(qSlicerSegmentEditorScissorsEffect& object);
  ~qSlicerSegmentEditorScissorsEffectPrivate();
public:
  enum
    {
    ShapeFreeForm,
    ShapeCircle,
    ShapeRectangle,
    Shape_Last// must be the last item
    };

  enum
    {
    OperationEraseInside,
    OperationEraseOutside,
    OperationFillInside,
    OperationFillOutside,
    Operation_Last // must be the last item
    };

  /// Draw outline glyph
  void createGlyph(ScissorsPipeline* pipeline, const vtkVector2i& eventPosition);
  /// Update outline glyph based on positions
  void updateGlyphWithNewPosition(ScissorsPipeline* pipeline, const vtkVector2i& eventPosition, bool finalize);
  void finalizeGlyph(ScissorsPipeline* pipeline, const vtkVector2i& eventPosition);
  /// Update 3D mesh of the brush
  bool updateBrushModel(qMRMLWidget* viewWidget);
  /// Update image stencil from mesh
  bool updateBrushStencil(qMRMLWidget* viewWidget);
  /// Paint brush into segment
  void paintApply(qMRMLWidget* viewWidget);

  static int ConvertShapeFromString(const QString& shapeStr);
  static QString ConvertShapeToString(int shape);
  static QString ConvertOperationToString(int operation);
  static int ConvertOperationFromString(const QString& operationStr);

  bool operationErase();
  bool operationInside();
  int shape();

protected:
  /// Get pipeline object for widget. Create if does not exist
  ScissorsPipeline* scissorsPipelineForWidget(qMRMLWidget* viewWidget);
  /// Delete all scissors pipelines
  void clearScissorsPipelines();
public:
  QIcon ScissorsIcon;

  vtkVector2i DragStartPosition;
  int CircleNumberOfPoints;

  vtkSmartPointer<vtkPoints> PaintCoordinates_View;

  vtkSmartPointer<vtkPolyDataNormals> BrushPolyDataNormals;
  vtkSmartPointer<vtkTransformPolyDataFilter> WorldToModifierLabelmapIjkTransformer;
  // transforms from polydata source to modifierLabelmap's IJK coordinate system (brush origin in IJK origin)
  vtkSmartPointer<vtkTransform> WorldToModifierLabelmapIjkTransform;
  vtkSmartPointer<vtkPolyDataToImageStencil> BrushPolyDataToStencil;

  QMap<qMRMLWidget*, ScissorsPipeline*> ScissorsPipelines;

  QComboBox* OperationSelectorComboBox;
  QComboBox* ShapeSelectorComboBox;
};

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScissorsEffectPrivate::qSlicerSegmentEditorScissorsEffectPrivate(qSlicerSegmentEditorScissorsEffect& object)
  : q_ptr(&object)
{
  this->ScissorsIcon = QIcon(":Icons/Medium/SlicerEditCut.png");
  this->CircleNumberOfPoints = 36;
  this->PaintCoordinates_View = vtkSmartPointer<vtkPoints>::New();

  this->BrushPolyDataNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
  this->BrushPolyDataNormals->AutoOrientNormalsOn();

  this->WorldToModifierLabelmapIjkTransformer = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  this->WorldToModifierLabelmapIjkTransform = vtkSmartPointer<vtkTransform>::New();
  this->WorldToModifierLabelmapIjkTransformer->SetTransform(this->WorldToModifierLabelmapIjkTransform);
  this->WorldToModifierLabelmapIjkTransformer->SetInputConnection(this->BrushPolyDataNormals->GetOutputPort());
  this->BrushPolyDataToStencil = vtkSmartPointer<vtkPolyDataToImageStencil>::New();
  this->BrushPolyDataToStencil->SetOutputSpacing(1.0, 1.0, 1.0);
  this->BrushPolyDataToStencil->SetInputConnection(this->WorldToModifierLabelmapIjkTransformer->GetOutputPort());
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorScissorsEffectPrivate::~qSlicerSegmentEditorScissorsEffectPrivate()
{
  this->clearScissorsPipelines();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffectPrivate::clearScissorsPipelines()
{
  Q_Q(qSlicerSegmentEditorScissorsEffect);
  QMapIterator<qMRMLWidget*, ScissorsPipeline*> it(this->ScissorsPipelines);
  while (it.hasNext())
    {
    it.next();
    qMRMLWidget* viewWidget = it.key();
    ScissorsPipeline* pipeline = it.value();
    q->removeActor2D(viewWidget, pipeline->Actor);
    q->removeActor2D(viewWidget, pipeline->ActorThin);
    delete pipeline;
    }
  this->ScissorsPipelines.clear();
}

//-----------------------------------------------------------------------------
ScissorsPipeline* qSlicerSegmentEditorScissorsEffectPrivate::scissorsPipelineForWidget(qMRMLWidget* viewWidget)
{
  Q_Q(qSlicerSegmentEditorScissorsEffect);

  if (this->ScissorsPipelines.contains(viewWidget))
    {
    return this->ScissorsPipelines[viewWidget];
    }

  // Create pipeline if does not yet exist
  ScissorsPipeline* pipeline = new ScissorsPipeline();
  vtkVector2i eventPosition;
  this->createGlyph(pipeline, eventPosition);

  vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(viewWidget);
  if (!renderer)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get renderer!";
    }
  else
    {
    q->addActor2D(viewWidget, pipeline->Actor);
    q->addActor2D(viewWidget, pipeline->ActorThin);
    }

  this->ScissorsPipelines[viewWidget] = pipeline;
  return pipeline;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffectPrivate::createGlyph(ScissorsPipeline* pipeline, const vtkVector2i& eventPosition)
{
  if (pipeline->IsDragging)
    {
    int numberOfPoints = 0;
    switch (this->shape())
      {
      case ShapeRectangle:
        numberOfPoints = 4;
        break;
      case ShapeCircle:
        numberOfPoints = this->CircleNumberOfPoints;
        break;
      case ShapeFreeForm:
        numberOfPoints = 1;
        break;
      }
    this->DragStartPosition = eventPosition;

    pipeline->PolyData->Initialize();
    pipeline->PolyDataThin->Initialize();

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    pipeline->PolyData->SetPoints(points);
    pipeline->PolyData->SetLines(lines);

    int previousPointIndex = -1;
    int firstPointIndex = -1;
    int pointIndex = -1;
    for (int corner = 0; corner<numberOfPoints; ++corner)
      {
      pointIndex = points->InsertNextPoint(eventPosition[0], eventPosition[1], 0.0);
      if (previousPointIndex != -1)
        {
        vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
        idList->InsertNextId(previousPointIndex);
        idList->InsertNextId(pointIndex);
        pipeline->PolyData->InsertNextCell(VTK_LINE, idList);
        }
      previousPointIndex = pointIndex;
      if (firstPointIndex == -1)
        {
        firstPointIndex = pointIndex;
        }
      }

    // Make the last line in the polydata (connects first and last point)
    // Free-form shape uses a separate actor to show it with a thinner line
    if (this->shape() == ShapeFreeForm)
      {
      // pipeline->PolyDataThin contains the thin closing line (from the first to last point)
      vtkSmartPointer<vtkPoints> pointsThin = vtkSmartPointer<vtkPoints>::New();
      vtkSmartPointer<vtkCellArray> linesThin = vtkSmartPointer<vtkCellArray>::New();
      pipeline->PolyDataThin->SetPoints(pointsThin);
      pipeline->PolyDataThin->SetLines(linesThin);

      pointsThin->InsertNextPoint(eventPosition[0], eventPosition[1], 0.0);
      pointsThin->InsertNextPoint(eventPosition[0], eventPosition[1], 0.0);

      vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
      idList->InsertNextId(0);
      idList->InsertNextId(1);
      pipeline->PolyDataThin->InsertNextCell(VTK_LINE, idList);

      pipeline->ActorThin->VisibilityOn();
      }
    else
      {
      vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
      idList->InsertNextId(pointIndex);
      idList->InsertNextId(firstPointIndex);
      pipeline->PolyData->InsertNextCell(VTK_LINE, idList);
      pipeline->ActorThin->VisibilityOff();
      }

    pipeline->Actor->VisibilityOn();
    }
  else
    {
    pipeline->Actor->VisibilityOff();
    pipeline->ActorThin->VisibilityOff();
    }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffectPrivate::updateGlyphWithNewPosition(ScissorsPipeline* pipeline, const vtkVector2i& eventPosition, bool finalize)
{
  if (pipeline->IsDragging)
    {
    vtkPoints* points = pipeline->PolyData->GetPoints();
    int numberOfPoints = points->GetNumberOfPoints();
    switch (this->shape())
      {
      case ShapeRectangle:
        {
        if (numberOfPoints != 4)
          {
          qWarning() << Q_FUNC_INFO << "Rectangle glyph is expected to have 4 points";
          break;
          }
        points->SetPoint(1, this->DragStartPosition[0], eventPosition[1], 0.0);
        points->SetPoint(2, eventPosition[0], eventPosition[1], 0.0);
        points->SetPoint(3, eventPosition[0], this->DragStartPosition[1], 0.0);
        points->Modified();
        break;
        }
      case ShapeCircle:
        {
        if (numberOfPoints < 4)
          {
          qWarning() << Q_FUNC_INFO << "Circle glyph is expected to have at least 4 points";
          break;
          }
        double radius = sqrt((eventPosition[0] - this->DragStartPosition[0])*(eventPosition[0] - this->DragStartPosition[0])
          + (eventPosition[1] - this->DragStartPosition[1])*(eventPosition[1] - this->DragStartPosition[1]));
        double position[3] = { 0 };
        for (int i = 0; i < numberOfPoints; i++)
          {
          double angle = 2.0 * vtkMath::Pi() * i / double(numberOfPoints);
          position[0] = this->DragStartPosition[0] + radius * sin(angle);
          position[1] = this->DragStartPosition[1] + radius * cos(angle);
          points->SetPoint(i, position);
          }
        points->Modified();
        break;
        }
      case ShapeFreeForm:
        {
        if (numberOfPoints < 1)
          {
          qWarning() << Q_FUNC_INFO << "Free-form glyph is expected to have at least 1 point";
          break;
          }
        vtkIdType newPointId = points->InsertNextPoint(eventPosition[0], eventPosition[1], 0.0);
        vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
        if (finalize)
          {
          idList->InsertNextId(newPointId);
          idList->InsertNextId(0);
          }
        else
          {
          idList->InsertNextId(newPointId - 1);
          idList->InsertNextId(newPointId);
          }
        pipeline->PolyData->InsertNextCell(VTK_LINE, idList);
        points->Modified();
        pipeline->PolyDataThin->GetPoints()->SetPoint(1, eventPosition[0], eventPosition[1], 0.0);
        pipeline->PolyDataThin->GetPoints()->Modified();
        break;
        }
      }
    }
  else
    {
    pipeline->Actor->VisibilityOff();
    pipeline->ActorThin->VisibilityOff();
    }
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorScissorsEffectPrivate::updateBrushModel(qMRMLWidget* viewWidget)
{
  Q_Q(qSlicerSegmentEditorScissorsEffect);
  ScissorsPipeline* pipeline = this->scissorsPipelineForWidget(viewWidget);
  if (!pipeline)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get pipeline";
    return false;
    }

  vtkNew<vtkPoints> closedSurfacePoints; // p0Top, p0Bottom, p1Top, p1Bottom, ...
  vtkNew<vtkCellArray> closedSurfaceStrips;
  vtkNew<vtkCellArray> closedSurfacePolys;

  vtkPoints* pointsXY = pipeline->PolyData->GetPoints();
  int numberOfPoints = pointsXY ? pointsXY->GetNumberOfPoints() : 0;
  if (numberOfPoints <= 1)
    {
    return false;
    }

  // Get bounds of modifier labelmap
  if (!q->parameterSetNode())
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return false;
    }
  vtkMRMLSegmentationNode* segmentationNode = q->parameterSetNode()->GetSegmentationNode();
  if (!segmentationNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentationNode";
    return false;
    }
  vtkOrientedImageData* modifierLabelmap = q->modifierLabelmap();
  if (!modifierLabelmap)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid modifierLabelmap";
    return false;
    }
  vtkNew<vtkMatrix4x4> segmentationToWorldMatrix;
  // We don't support painting in non-linearly transformed node (it could be implemented, but would probably slow down things too much)
  // TODO: show a meaningful error message to the user if attempted
  vtkMRMLTransformNode::GetMatrixTransformBetweenNodes(segmentationNode->GetParentTransformNode(), NULL, segmentationToWorldMatrix.GetPointer());

  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (sliceWidget)
    {
    vtkMRMLSliceNode* sliceNode = vtkMRMLSliceNode::SafeDownCast(qSlicerSegmentEditorAbstractEffect::viewNode(sliceWidget));
    if (!sliceNode)
      {
      qCritical() << Q_FUNC_INFO << ": Failed to get slice node";
      return false;
      }

    // Get modifier labelmap extent in slice coordinate system to know how much we
    // have to cut through
    vtkNew<vtkTransform> segmentationToSliceXYTransform;
    vtkNew<vtkMatrix4x4> worldToSliceXYMatrix;
    vtkMatrix4x4::Invert(sliceNode->GetXYToRAS(), worldToSliceXYMatrix.GetPointer());
    segmentationToSliceXYTransform->Concatenate(worldToSliceXYMatrix.GetPointer());
    segmentationToSliceXYTransform->Concatenate(segmentationToWorldMatrix.GetPointer());
    double segmentationBounds_SliceXY[6] = { 0, -1, 0, -1, 0, -1 };
    vtkOrientedImageDataResample::TransformOrientedImageDataBounds(modifierLabelmap, segmentationToSliceXYTransform.GetPointer(), segmentationBounds_SliceXY);
    // Extend bounds by half slice to make sure the boundaries are included
    if (segmentationBounds_SliceXY[4] < segmentationBounds_SliceXY[5])
      {
      segmentationBounds_SliceXY[4] -= 0.5;
      segmentationBounds_SliceXY[5] += 0.5;
      }
    else
      {
      segmentationBounds_SliceXY[4] += 0.5;
      segmentationBounds_SliceXY[5] -= 0.5;
      }

    for (int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++)
      {
      double pointXY[4] = { 0., 0., 0., 1. };
      double pointWorld[4] = { 0., 0., 0., 1. };
      pointsXY->GetPoint(pointIndex, pointXY);

      pointXY[2] = segmentationBounds_SliceXY[4];
      sliceNode->GetXYToRAS()->MultiplyPoint(pointXY, pointWorld);
      closedSurfacePoints->InsertNextPoint(pointWorld);

      pointXY[2] = segmentationBounds_SliceXY[5];
      sliceNode->GetXYToRAS()->MultiplyPoint(pointXY, pointWorld);
      closedSurfacePoints->InsertNextPoint(pointWorld);
      }
    }
  else if(threeDWidget)
    {
    vtkRenderer* renderer = qSlicerSegmentEditorAbstractEffect::renderer(threeDWidget);
    if (!renderer)
      {
      qCritical() << Q_FUNC_INFO << ": Failed to get renderer";
      return false;
      }
    vtkCamera *camera = renderer->GetActiveCamera();
    if (!camera)
      {
      qCritical() << Q_FUNC_INFO << ": Failed to get camera";
      return false;
      }

    // Camera parameters
    // Camera position
    double cameraPos[4] = { 0 };
    camera->GetPosition(cameraPos);
    cameraPos[3] = 1.0;
    // Focal point position
    double cameraFP[4] = { 0 };
    camera->GetFocalPoint(cameraFP);
    cameraFP[3] = 1.0;
    // Direction of projection
    double cameraDOP[3] = { 0 };
    for (int i = 0; i < 3; i++)
      {
      cameraDOP[i] = cameraFP[i] - cameraPos[i];
      }
    vtkMath::Normalize(cameraDOP);
    // Camera view up
    double cameraViewUp[3] = { 0 };
    camera->GetViewUp(cameraViewUp);
    vtkMath::Normalize(cameraViewUp);

    renderer->SetWorldPoint(cameraFP[0], cameraFP[1], cameraFP[2], cameraFP[3]);
    renderer->WorldToDisplay();
    double* displayCoords = renderer->GetDisplayPoint();
    double selectionZ = displayCoords[2];

    // Get modifier labelmap extent in camera coordinate system to know how much we
    // have to cut through
    vtkNew<vtkMatrix4x4> cameraToWorldMatrix;
    double cameraViewRight[3] = { 1, 0, 0 };
    vtkMath::Cross(cameraDOP, cameraViewUp, cameraViewRight);
    for (int i = 0; i < 3; i++)
      {
      cameraToWorldMatrix->SetElement(i, 3, cameraPos[i]);
      cameraToWorldMatrix->SetElement(i, 0, cameraViewUp[i]);
      cameraToWorldMatrix->SetElement(i, 1, cameraViewRight[i]);
      cameraToWorldMatrix->SetElement(i, 2, cameraDOP[i]);
      }
    vtkNew<vtkMatrix4x4> worldToCameraMatrix;
    vtkMatrix4x4::Invert(cameraToWorldMatrix.GetPointer(), worldToCameraMatrix.GetPointer());
    vtkNew<vtkTransform> segmentationToCameraTransform;
    segmentationToCameraTransform->Concatenate(worldToCameraMatrix.GetPointer());
    segmentationToCameraTransform->Concatenate(segmentationToWorldMatrix.GetPointer());
    double segmentationBounds_Camera[6] = { 0, -1, 0, -1, 0, -1 };
    vtkOrientedImageDataResample::TransformOrientedImageDataBounds(modifierLabelmap, segmentationToCameraTransform.GetPointer(), segmentationBounds_Camera);
    double clipRangeFromModifierLabelmap[2] =
      {
      std::min(segmentationBounds_Camera[4], segmentationBounds_Camera[5]),
      std::max(segmentationBounds_Camera[4], segmentationBounds_Camera[5])
      };
    // Extend bounds by half slice to make sure the boundaries are included
    clipRangeFromModifierLabelmap[0] -= 0.5;
    clipRangeFromModifierLabelmap[1] += 0.5;

    // Clip what we see on the camera but reduce it to the modifier labelmap's range
    // to keep the stencil as small as possible
    double* clipRangeFromCamera = camera->GetClippingRange();
    double clipRange[2] =
      {
      std::max(clipRangeFromModifierLabelmap[0], clipRangeFromCamera[0]),
      std::min(clipRangeFromModifierLabelmap[1], clipRangeFromCamera[1]),
      };

    for (int pointIndex = 0; pointIndex < numberOfPoints; pointIndex++)
      {
      // Convert the selection point into world coordinates.
      //
      double pointXY[3] = { 0., 0., 0. };
      pointsXY->GetPoint(pointIndex, pointXY);
      renderer->SetDisplayPoint(pointXY[0], pointXY[1], selectionZ);
      renderer->DisplayToWorld();
      double* worldCoords = renderer->GetWorldPoint();
      if (worldCoords[3] == 0.0)
        {
        qWarning() << Q_FUNC_INFO << ": Bad homogeneous coordinates";
        return false;
        }
      double pickPosition[3] = { 0 };
      for (int i = 0; i < 3; i++)
        {
        pickPosition[i] = worldCoords[i] / worldCoords[3];
        }

      //  Compute the ray endpoints.  The ray is along the line running from
      //  the camera position to the selection point, starting where this line
      //  intersects the front clipping plane, and terminating where this
      //  line intersects the back clipping plane.
      double ray[3] = { 0 };
      for (int i = 0; i < 3; i++)
        {
        ray[i] = pickPosition[i] - cameraPos[i];
        }

      double rayLength = vtkMath::Dot(cameraDOP, ray);
      if (rayLength == 0.0)
        {
        qWarning() << Q_FUNC_INFO << ": Cannot process points";
        return false;
        }

      double p1World[4] = { 0 };
      double p2World[4] = { 0 };
      double tF = 0;
      double tB = 0;
      if (camera->GetParallelProjection())
        {
        tF = clipRange[0] - rayLength;
        tB = clipRange[1] - rayLength;
        for (int i = 0; i < 3; i++)
          {
          p1World[i] = pickPosition[i] + tF*cameraDOP[i];
          p2World[i] = pickPosition[i] + tB*cameraDOP[i];
          }
        }
      else
        {
        tF = clipRange[0] / rayLength;
        tB = clipRange[1] / rayLength;
        for (int i = 0; i < 3; i++)
          {
          p1World[i] = cameraPos[i] + tF*ray[i];
          p2World[i] = cameraPos[i] + tB*ray[i];
          }
        }
      p1World[3] = p2World[3] = 1.0;

      closedSurfacePoints->InsertNextPoint(p1World);
      closedSurfacePoints->InsertNextPoint(p2World);
      }
    }
  else
    {
    return false;
    }

  // Construct polydata
  vtkNew<vtkPolyData> closedSurfacePolyData;
  closedSurfacePolyData->SetPoints(closedSurfacePoints.GetPointer());
  closedSurfacePolyData->SetStrips(closedSurfaceStrips.GetPointer());
  closedSurfacePolyData->SetPolys(closedSurfacePolys.GetPointer());

  // Skirt
  closedSurfaceStrips->InsertNextCell(numberOfPoints * 2 + 2);
  for (int i = 0; i < numberOfPoints * 2; i++)
    {
    closedSurfaceStrips->InsertCellPoint(i);
    }
  closedSurfaceStrips->InsertCellPoint(0);
  closedSurfaceStrips->InsertCellPoint(1);
  // Front cap
  closedSurfacePolys->InsertNextCell(numberOfPoints);
  for (int i = 0; i < numberOfPoints; i++)
    {
    closedSurfacePolys->InsertCellPoint(i * 2);
    }
  // Back cap
  closedSurfacePolys->InsertNextCell(numberOfPoints);
  for (int i = 0; i < numberOfPoints; i++)
    {
    closedSurfacePolys->InsertCellPoint(i * 2 + 1);
    }

  this->BrushPolyDataNormals->SetInputData(closedSurfacePolyData.GetPointer());
  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorScissorsEffectPrivate::updateBrushStencil(qMRMLWidget* viewWidget)
{
  Q_Q(qSlicerSegmentEditorScissorsEffect);
  Q_UNUSED(viewWidget);

  if (!q->parameterSetNode())
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return false;
    }
  vtkMRMLSegmentationNode* segmentationNode = q->parameterSetNode()->GetSegmentationNode();
  if (!segmentationNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentationNode";
    return false;
    }
  vtkOrientedImageData* modifierLabelmap = q->modifierLabelmap();
  if (!modifierLabelmap)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid modifierLabelmap";
    return false;
    }

  // Brush stencil transform

  this->WorldToModifierLabelmapIjkTransform->Identity();

  vtkNew<vtkMatrix4x4> segmentationToSegmentationIjkTransformMatrix;
  modifierLabelmap->GetWorldToImageMatrix(segmentationToSegmentationIjkTransformMatrix.GetPointer());
  this->WorldToModifierLabelmapIjkTransform->Concatenate(segmentationToSegmentationIjkTransformMatrix.GetPointer());

  vtkNew<vtkMatrix4x4> worldToSegmentationTransformMatrix;
  // We don't support painting in non-linearly transformed node (it could be implemented, but would probably slow down things too much)
  // TODO: show a meaningful error message to the user if attempted
  vtkMRMLTransformNode::GetMatrixTransformBetweenNodes(NULL, segmentationNode->GetParentTransformNode(), worldToSegmentationTransformMatrix.GetPointer());
  this->WorldToModifierLabelmapIjkTransform->Concatenate(worldToSegmentationTransformMatrix.GetPointer());

  this->WorldToModifierLabelmapIjkTransformer->Update();
  if (this->operationInside())
    {
    // Clip modifier labelmap to non-null region to make labelmap modification faster later
    vtkPolyData* brushModel_ModifierLabelmapIjk = this->WorldToModifierLabelmapIjkTransformer->GetOutput();
    double* boundsIjk = brushModel_ModifierLabelmapIjk->GetBounds();
    this->BrushPolyDataToStencil->SetOutputWholeExtent(floor(boundsIjk[0]) - 1, ceil(boundsIjk[1]) + 1,
      floor(boundsIjk[2]) - 1, ceil(boundsIjk[3]) + 1, floor(boundsIjk[4]) - 1, ceil(boundsIjk[5]) + 1);
    }
  else
    {
    this->BrushPolyDataToStencil->SetOutputWholeExtent(modifierLabelmap->GetExtent());
    }
  return true;
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffectPrivate::paintApply(qMRMLWidget* viewWidget)
{
  Q_Q(qSlicerSegmentEditorScissorsEffect);

  vtkOrientedImageData* modifierLabelmap = q->defaultModifierLabelmap();
  if (!modifierLabelmap)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentationNode";
    return;
    }
  if (!q->parameterSetNode())
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node!";
    return;
    }
  vtkMRMLSegmentationNode* segmentationNode = q->parameterSetNode()->GetSegmentationNode();
  if (!segmentationNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentationNode";
    return;
    }

  if (!this->updateBrushModel(viewWidget))
    {
    return;
    }
  if (!this->updateBrushStencil(viewWidget))
    {
    return;
    }
  q->saveStateForUndo();
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  this->BrushPolyDataToStencil->Update();
  vtkImageStencilData* stencilData = this->BrushPolyDataToStencil->GetOutput();
  int stencilExtent[6] = { 0, -1, 0, -1, 0, -1 };
  stencilData->GetExtent(stencilExtent);

  vtkNew<vtkTransform> worldToModifierLabelmapIjkTransform;

  vtkNew<vtkMatrix4x4> segmentationToSegmentationIjkTransformMatrix;
  modifierLabelmap->GetImageToWorldMatrix(segmentationToSegmentationIjkTransformMatrix.GetPointer());
  segmentationToSegmentationIjkTransformMatrix->Invert();
  worldToModifierLabelmapIjkTransform->Concatenate(segmentationToSegmentationIjkTransformMatrix.GetPointer());

  vtkNew<vtkMatrix4x4> worldToSegmentationTransformMatrix;
  // We don't support painting in non-linearly transformed node (it could be implemented, but would probably slow down things too much)
  // TODO: show a meaningful error message to the user if attempted
  vtkMRMLTransformNode::GetMatrixTransformBetweenNodes(NULL, segmentationNode->GetParentTransformNode(), worldToSegmentationTransformMatrix.GetPointer());
  worldToModifierLabelmapIjkTransform->Concatenate(worldToSegmentationTransformMatrix.GetPointer());

  vtkNew<vtkImageStencilToImage> stencilToImage;
  stencilToImage->SetInputConnection(this->BrushPolyDataToStencil->GetOutputPort());
  if (this->operationInside())
    {
    stencilToImage->SetInsideValue(q->m_FillValue);
    stencilToImage->SetOutsideValue(q->m_EraseValue);
    }
  else
    {
    stencilToImage->SetInsideValue(q->m_EraseValue);
    stencilToImage->SetOutsideValue(q->m_FillValue);
    }

  stencilToImage->SetOutputScalarType(modifierLabelmap->GetScalarType());

  vtkNew<vtkImageChangeInformation> brushPositioner;
  brushPositioner->SetInputConnection(stencilToImage->GetOutputPort());
  brushPositioner->SetOutputSpacing(modifierLabelmap->GetSpacing());
  brushPositioner->SetOutputOrigin(modifierLabelmap->GetOrigin());

  brushPositioner->Update();
  vtkNew<vtkOrientedImageData> orientedBrushPositionerOutput;
  orientedBrushPositionerOutput->ShallowCopy(stencilToImage->GetOutput());
  vtkNew<vtkMatrix4x4> imageToWorld;
  modifierLabelmap->GetImageToWorldMatrix(imageToWorld.GetPointer());
  orientedBrushPositionerOutput->SetImageToWorldMatrix(imageToWorld.GetPointer());

  vtkOrientedImageDataResample::ModifyImage(modifierLabelmap, orientedBrushPositionerOutput.GetPointer(), vtkOrientedImageDataResample::OPERATION_MAXIMUM);

  // Notify editor about changes
  qSlicerSegmentEditorAbstractEffect::ModificationMode modificationMode = qSlicerSegmentEditorAbstractEffect::ModificationModeAdd;
  if (this->operationErase())
    {
    modificationMode = qSlicerSegmentEditorAbstractEffect::ModificationModeRemove;
    }
  q->modifySelectedSegmentByLabelmap(modifierLabelmap, modificationMode);

  QApplication::restoreOverrideCursor();
}

//-----------------------------------------------------------------------------
int qSlicerSegmentEditorScissorsEffectPrivate::ConvertShapeFromString(const QString& shapeStr)
{
  for (int i = 0; i < Shape_Last; i++)
    {
    if (shapeStr == ConvertShapeToString(i))
      {
      return i;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorScissorsEffectPrivate::ConvertShapeToString(int shape)
{
  switch (shape)
    {
    case ShapeFreeForm: return "FreeForm";
    case ShapeCircle: return "Circle";
    case ShapeRectangle: return "Rectangle";
    default: return "";
    }
}

//-----------------------------------------------------------------------------
int qSlicerSegmentEditorScissorsEffectPrivate::ConvertOperationFromString(const QString& operationStr)
{
  for (int i = 0; i < Shape_Last; i++)
    {
    if (operationStr == ConvertOperationToString(i))
      {
      return i;
      }
    }
  return -1;
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentEditorScissorsEffectPrivate::ConvertOperationToString(int operation)
{
  switch (operation)
    {
    case OperationEraseInside: return "EraseInside";
    case OperationEraseOutside: return "EraseOutside";
    case OperationFillInside: return "FillInside";
    case OperationFillOutside: return "FillOutside";
    default: return "";
    }
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorScissorsEffectPrivate::operationInside()
{
  Q_Q(qSlicerSegmentEditorScissorsEffect);
  int operation = this->ConvertOperationFromString(q->parameter("Operation"));
  return (operation == OperationEraseInside || operation == OperationFillInside);
}

//-----------------------------------------------------------------------------
bool qSlicerSegmentEditorScissorsEffectPrivate::operationErase()
{
  Q_Q(qSlicerSegmentEditorScissorsEffect);
  int operation = this->ConvertOperationFromString(q->parameter("Operation"));
  return (operation == OperationEraseInside || operation == OperationEraseOutside);
}

//-----------------------------------------------------------------------------
int qSlicerSegmentEditorScissorsEffectPrivate::shape()
{
  Q_Q(qSlicerSegmentEditorScissorsEffect);
  return this->ConvertShapeFromString(q->parameter("Shape"));
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorScissorsEffect::qSlicerSegmentEditorScissorsEffect(QObject* parent)
 : Superclass(parent)
 , d_ptr( new qSlicerSegmentEditorScissorsEffectPrivate(*this) )
{
  this->m_Name = QString("Scissors");
  this->m_ShowEffectCursorInThreeDView = true;
}

//----------------------------------------------------------------------------
qSlicerSegmentEditorScissorsEffect::~qSlicerSegmentEditorScissorsEffect()
{
}

//---------------------------------------------------------------------------
QIcon qSlicerSegmentEditorScissorsEffect::icon()
{
  Q_D(qSlicerSegmentEditorScissorsEffect);

  return d->ScissorsIcon;
}

//---------------------------------------------------------------------------
QString const qSlicerSegmentEditorScissorsEffect::helpText()const
{
  return QString("Cut through the entire segment from the current viewpoint.\nLeft-click and drag to sweep out an outline, release the button when ready.");
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffect::setupOptionsFrame()
{
  Q_D(qSlicerSegmentEditorScissorsEffect);

  // Setup widgets corresponding to the parent class of this effect
  Superclass::setupOptionsFrame();

  d->OperationSelectorComboBox = new QComboBox();
  d->OperationSelectorComboBox->addItem("Erase inside", d->ConvertOperationToString(qSlicerSegmentEditorScissorsEffectPrivate::OperationEraseInside));
  d->OperationSelectorComboBox->addItem("Erase outside", d->ConvertOperationToString(qSlicerSegmentEditorScissorsEffectPrivate::OperationEraseOutside));
  d->OperationSelectorComboBox->addItem("Fill inside", d->ConvertOperationToString(qSlicerSegmentEditorScissorsEffectPrivate::OperationFillInside));
  d->OperationSelectorComboBox->addItem("Fill outside", d->ConvertOperationToString(qSlicerSegmentEditorScissorsEffectPrivate::OperationFillOutside));
  this->addLabeledOptionsWidget("Operation:", d->OperationSelectorComboBox);

  d->ShapeSelectorComboBox = new QComboBox();
  d->ShapeSelectorComboBox->addItem("Free-form", d->ConvertShapeToString(qSlicerSegmentEditorScissorsEffectPrivate::ShapeFreeForm));
  d->ShapeSelectorComboBox->addItem("Circle", d->ConvertShapeToString(qSlicerSegmentEditorScissorsEffectPrivate::ShapeCircle));
  d->ShapeSelectorComboBox->addItem("Rectangle", d->ConvertShapeToString(qSlicerSegmentEditorScissorsEffectPrivate::ShapeRectangle));
  this->addLabeledOptionsWidget("Shape:", d->ShapeSelectorComboBox);

  QObject::connect(d->OperationSelectorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMRMLFromGUI()));
  QObject::connect(d->ShapeSelectorComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMRMLFromGUI()));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffect::setMRMLDefaults()
{
  Q_D(qSlicerSegmentEditorScissorsEffect);
  Superclass::setMRMLDefaults();
  this->setParameterDefault("Operation", d->ConvertOperationToString(qSlicerSegmentEditorScissorsEffectPrivate::OperationEraseInside));
  this->setParameterDefault("Shape", d->ConvertShapeToString(qSlicerSegmentEditorScissorsEffectPrivate::ShapeFreeForm));
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffect::updateGUIFromMRML()
{
  Q_D(qSlicerSegmentEditorScissorsEffect);

  Superclass::updateGUIFromMRML();

  if (!this->active())
    {
    // updateGUIFromMRML is called when the effect is activated
    return;
    }

  if (!this->scene())
    {
    return;
    }

  int operationIndex = d->OperationSelectorComboBox->findData(QString(this->parameter("Operation")));
  bool wasBlocked = d->OperationSelectorComboBox->blockSignals(true);
  d->OperationSelectorComboBox->setCurrentIndex(operationIndex);
  d->OperationSelectorComboBox->blockSignals(wasBlocked);

  int shapeIndex = d->ShapeSelectorComboBox->findData(QString(this->parameter("Shape")));
  wasBlocked = d->ShapeSelectorComboBox->blockSignals(true);
  d->ShapeSelectorComboBox->setCurrentIndex(shapeIndex);
  d->ShapeSelectorComboBox->blockSignals(wasBlocked);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffect::updateMRMLFromGUI()
{
  Q_D(qSlicerSegmentEditorScissorsEffect);

  Superclass::updateMRMLFromGUI();

  QString operation = d->OperationSelectorComboBox->itemData(d->OperationSelectorComboBox->currentIndex()).toString();
  QString shape = d->ShapeSelectorComboBox->itemData(d->ShapeSelectorComboBox->currentIndex()).toString();
  this->setParameter("Operation", operation.toLatin1().constData());
  this->setParameter("Shape", shape.toLatin1().constData());
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qSlicerSegmentEditorScissorsEffect::clone()
{
  return new qSlicerSegmentEditorScissorsEffect();
}

//---------------------------------------------------------------------------
void qSlicerSegmentEditorScissorsEffect::deactivate()
{
  Q_D(qSlicerSegmentEditorScissorsEffect);
  Superclass::deactivate();
  d->clearScissorsPipelines();
}

//---------------------------------------------------------------------------
bool qSlicerSegmentEditorScissorsEffect::processInteractionEvents(
  vtkRenderWindowInteractor* callerInteractor,
  unsigned long eid,
  qMRMLWidget* viewWidget )
{
  Q_D(qSlicerSegmentEditorScissorsEffect);
  bool abortEvent = false;

  // This effect only supports interactions in the 2D slice views currently
  qMRMLSliceWidget* sliceWidget = qobject_cast<qMRMLSliceWidget*>(viewWidget);
  qMRMLThreeDWidget* threeDWidget = qobject_cast<qMRMLThreeDWidget*>(viewWidget);
  if (!sliceWidget && !threeDWidget)
    {
    return abortEvent;
    }
  ScissorsPipeline* pipeline = d->scissorsPipelineForWidget(viewWidget);
  if (!pipeline)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to create pipeline";
    return abortEvent;
    }

  if (eid == vtkCommand::LeftButtonPressEvent)
    {
    pipeline->IsDragging = true;
    //this->cursorOff(viewWidget);
    vtkVector2i eventPosition;
    callerInteractor->GetEventPosition(eventPosition[0], eventPosition[1]);
    d->createGlyph(pipeline, eventPosition);
    abortEvent = true;
    }
  else if (eid == vtkCommand::MouseMoveEvent)
    {
    if (pipeline->IsDragging)
      {
      vtkVector2i eventPosition;
      callerInteractor->GetEventPosition(eventPosition[0], eventPosition[1]);
      d->updateGlyphWithNewPosition(pipeline, eventPosition, false);
      this->scheduleRender(viewWidget);
      abortEvent = true;
      }
    }
  else if (eid == vtkCommand::LeftButtonReleaseEvent)
    {
    pipeline->IsDragging = false;
    vtkVector2i eventPosition;
    callerInteractor->GetEventPosition(eventPosition[0], eventPosition[1]);
    d->updateGlyphWithNewPosition(pipeline, eventPosition, true);
    //this->cursorOn(viewWidget);

    // Paint on modifier labelmap
    vtkCellArray* lines = pipeline->PolyData->GetLines();
    vtkOrientedImageData* modifierLabelmap = defaultModifierLabelmap();
    if (lines->GetNumberOfCells() > 0 && modifierLabelmap)
      {
      d->paintApply(viewWidget);
      qSlicerSegmentEditorAbstractEffect::scheduleRender(viewWidget);
      }
    abortEvent = true;
    }
  return abortEvent;
}
