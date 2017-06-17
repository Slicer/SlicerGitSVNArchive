/*==============================================================================

  Program: 3D Slicer

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
#include "qMRMLSegmentEditorWidget.h"

#include "ui_qMRMLSegmentEditorWidget.h"

#include "vtkMRMLInteractionNode.h"
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"
#include "vtkMRMLSegmentEditorNode.h"
#include "vtkSegmentation.h"
#include "vtkSegmentationHistory.h"
#include "vtkSegment.h"
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"
#include "vtkSlicerSegmentationsModuleLogic.h"

// Segment editor effects includes
#include "qSlicerSegmentEditorAbstractEffect.h"
#include "qSlicerSegmentEditorAbstractLabelEffect.h"
#include "qSlicerSegmentEditorEffectFactory.h"

// VTK includes
#include <vtkAlgorithmOutput.h>
#include <vtkCallbackCommand.h>
#include <vtkCollection.h>
#include <vtkDataArray.h>
#include <vtkGeneralTransform.h>
#include <vtkImageThreshold.h>
#include <vtkInteractorObserver.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

// Slicer includes
#include "qSlicerApplication.h"
#include "vtkSlicerApplicationLogic.h"
#include "qSlicerLayoutManager.h"
#include "vtkMRMLSliceLogic.h"
#include "qMRMLSliceWidget.h"
#include "qMRMLSliceView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLThreeDView.h"

// MRML includes
#include <vtkMRMLLabelMapVolumeNode.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLViewNode.h>

// Qt includes
#include <QDebug>
#include <QToolButton>
#include <QButtonGroup>
#include <QMainWindow>
#include <QMessageBox>
#include <QShortcut>
#include <QVBoxLayout>

// CTK includes
#include <ctkFlowLayout.h>

static const int BINARY_LABELMAP_SCALAR_TYPE = VTK_UNSIGNED_CHAR;
// static const unsigned char BINARY_LABELMAP_VOXEL_FULL = 1; // unused
static const unsigned char BINARY_LABELMAP_VOXEL_EMPTY = 0;

static const char NULL_EFFECT_NAME[] = "NULL";

//---------------------------------------------------------------------------
class vtkSegmentEditorEventCallbackCommand : public vtkCallbackCommand
{
public:
  static vtkSegmentEditorEventCallbackCommand *New()
    {
    return new vtkSegmentEditorEventCallbackCommand;
    }
  /// Segment editor widget observing the event
  QWeakPointer<qMRMLSegmentEditorWidget> EditorWidget;
  /// Slice widget or 3D widget
  QWeakPointer<qMRMLWidget> ViewWidget;
};

//-----------------------------------------------------------------------------
struct SegmentEditorEventObservation
{
  vtkSmartPointer<vtkSegmentEditorEventCallbackCommand> CallbackCommand;
  vtkWeakPointer<vtkObject> ObservedObject;
  QVector<int> ObservationTags;
};

//-----------------------------------------------------------------------------
// qMRMLSegmentEditorWidgetPrivate methods

//-----------------------------------------------------------------------------
class qMRMLSegmentEditorWidgetPrivate: public Ui_qMRMLSegmentEditorWidget
{
  Q_DECLARE_PUBLIC(qMRMLSegmentEditorWidget);

protected:
  qMRMLSegmentEditorWidget* const q_ptr;
public:
  qMRMLSegmentEditorWidgetPrivate(qMRMLSegmentEditorWidget& object);
  ~qMRMLSegmentEditorWidgetPrivate();
  void init();

  /// Update list of effect buttons
  void updateEffectList();

  /// Simple mechanism to let the effects know that default modifier labelmap has changed
  void notifyEffectsOfReferenceGeometryChange(const std::string& geometry);
  /// Simple mechanism to let the effects know that master volume has changed
  void notifyEffectsOfMasterVolumeNodeChange();
  /// Simple mechanism to let the effects know that layout has changed
  void notifyEffectsOfLayoutChange();

  /// Select first segment in table view
  void selectFirstSegment();

  /// Enable or disable effects and their options based on input selection
  void updateEffectsEnabledFromMRML();

  /// Set cursor for effect. If effect is NULL then the cursor is reset to default.
  void setEffectCursor(qSlicerSegmentEditorAbstractEffect* effect);

  /// Updates default modifier labelmap based on reference geometry (to set origin, spacing, and directions)
  /// and existing segments (to set extents). If reference geometry conversion parameter is empty
  /// then existing segments are used for determining origin, spacing, and directions and the resulting
  /// geometry is written to reference geometry conversion parameter.
  bool resetModifierLabelmapToDefault();

  /// Updates selected segment labelmap in a geometry aligned with default modifierLabelmap.
  bool updateSelectedSegmentLabelmap();

  /// Updates a resampled master volume in a geometry aligned with default modifierLabelmap.
  bool updateAlignedMasterVolume();

  /// Updates mask labelmap aligned with default modifierLabelmap.
  bool updateMaskLabelmap();

  bool updateReferenceGeometryImage();

  static std::string getReferenceImageGeometryFromSegmentation(vtkSegmentation* segmentation);
  std::string referenceImageGeometry();

public:
  /// Segment editor parameter set node containing all selections and working images
  vtkWeakPointer<vtkMRMLSegmentEditorNode> ParameterSetNode;

  vtkWeakPointer<vtkMRMLSegmentationNode> SegmentationNode;
  vtkSmartPointer<vtkSegmentationHistory> SegmentationHistory;

  vtkWeakPointer<vtkMRMLScalarVolumeNode> MasterVolumeNode;

  // Observe InteractionNode to detect when mouse mode is changed
  vtkWeakPointer<vtkMRMLInteractionNode> InteractionNode;

  /// Lock widget to make segmentation read-only.
  // In the future locked state may be read from the Segmentation node.
  bool Locked;

  /// Ordering of effects
  QStringList EffectNameOrder;
  bool UnorderedEffectsVisible;

  /// List of registered effect instances
  QList<qSlicerSegmentEditorAbstractEffect*> RegisteredEffects;

  /// Active effect
  qSlicerSegmentEditorAbstractEffect* ActiveEffect;
  /// Last active effect
  /// Stored to allow quick toggling between no effect/last active effect.
  qSlicerSegmentEditorAbstractEffect* LastActiveEffect;

  /// Structure containing necessary objects for each slice and 3D view handling interactions
  QVector<SegmentEditorEventObservation> EventObservations;

  /// Button group for the effects
  QButtonGroup EffectButtonGroup;

  /// These volumes are owned by this widget and a pointer is given to each effect
  /// so that they can access and modify it
  vtkOrientedImageData* AlignedMasterVolume;
  /// Modifier labelmap that is kept in memory to avoid memory reallocations on each editing operation.
  /// When update of this labelmap is requested its geometry is reset and its content is cleared.
  vtkOrientedImageData* ModifierLabelmap;
  vtkOrientedImageData* SelectedSegmentLabelmap;
  vtkOrientedImageData* MaskLabelmap;
  /// Image that contains reference geometry. Scalars are not allocated.
  vtkOrientedImageData* ReferenceGeometryImage;

  /// Input data that is used for computing AlignedMasterVolume.
  /// It is stored so that it can be determined that the master volume has to be updated
  vtkMRMLVolumeNode* AlignedMasterVolumeUpdateMasterVolumeNode;
  vtkMRMLTransformNode* AlignedMasterVolumeUpdateMasterVolumeNodeTransform;
  vtkMRMLTransformNode* AlignedMasterVolumeUpdateSegmentationNodeTransform;

  int MaskModeComboBoxFixedItemsCount;

  /// If reference geometry changes compared to this value then we notify effects and
  /// set this value to the current value. This allows notifying effects when there is a change.
  std::string LastNotifiedReferenceImageGeometry;

  QList< QShortcut* > KeyboardShortcuts;

  Qt::ToolButtonStyle EffectButtonStyle;
};

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::qMRMLSegmentEditorWidgetPrivate(qMRMLSegmentEditorWidget& object)
  : q_ptr(&object)
  , Locked(false)
  , ActiveEffect(NULL)
  , LastActiveEffect(NULL)
  , AlignedMasterVolume(NULL)
  , ModifierLabelmap(NULL)
  , SelectedSegmentLabelmap(NULL)
  , MaskLabelmap(NULL)
  , ReferenceGeometryImage(NULL)
  , AlignedMasterVolumeUpdateMasterVolumeNode(NULL)
  , AlignedMasterVolumeUpdateMasterVolumeNodeTransform(NULL)
  , AlignedMasterVolumeUpdateSegmentationNodeTransform(NULL)
  , MaskModeComboBoxFixedItemsCount(0)
  , EffectButtonStyle(Qt::ToolButtonTextUnderIcon)
{
  this->AlignedMasterVolume = vtkOrientedImageData::New();
  this->ModifierLabelmap = vtkOrientedImageData::New();
  this->MaskLabelmap = vtkOrientedImageData::New();
  this->SelectedSegmentLabelmap = vtkOrientedImageData::New();
  this->ReferenceGeometryImage = vtkOrientedImageData::New();
  this->SegmentationHistory = vtkSmartPointer<vtkSegmentationHistory>::New();

  // Define default effect order
  this->EffectNameOrder
    // Local painting
    << "Paint" << "Draw" << "Erase" << "Level tracing" << "Grow from seeds" << "Fill between slices"
    // Global processing
    << "Threshold" << "Margin" << "Smoothing"
    // Global splitting, merging
    << "Scissors" << "Islands" << "Logical operators";
  this->UnorderedEffectsVisible = true;
}

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidgetPrivate::~qMRMLSegmentEditorWidgetPrivate()
{
  Q_Q(qMRMLSegmentEditorWidget);
  q->removeViewObservations();

  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
    {
    delete effect;
    }
  this->RegisteredEffects.clear();
  if (this->AlignedMasterVolume)
    {
    this->AlignedMasterVolume->Delete();
    this->AlignedMasterVolume = NULL;
    }
  if (this->ModifierLabelmap)
    {
    this->ModifierLabelmap->Delete();
    this->ModifierLabelmap = NULL;
    }
  if (this->MaskLabelmap)
    {
    this->MaskLabelmap->Delete();
    this->MaskLabelmap = NULL;
    }
  if (this->SelectedSegmentLabelmap)
    {
    this->SelectedSegmentLabelmap->Delete();
    this->SelectedSegmentLabelmap = NULL;
    }
  if (this->ReferenceGeometryImage)
    {
    this->ReferenceGeometryImage->Delete();
    this->ReferenceGeometryImage = NULL;
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::init()
{
  Q_Q(qMRMLSegmentEditorWidget);
  this->setupUi(q);

  this->MaskModeComboBox->addItem(QObject::tr("Everywhere"), vtkMRMLSegmentEditorNode::PaintAllowedEverywhere);
  this->MaskModeComboBox->addItem(QObject::tr("Inside all segments"), vtkMRMLSegmentEditorNode::PaintAllowedInsideAllSegments);
  this->MaskModeComboBox->addItem(QObject::tr("Inside all visible segments"), vtkMRMLSegmentEditorNode::PaintAllowedInsideVisibleSegments);
  this->MaskModeComboBox->addItem(QObject::tr("Outside all segments"), vtkMRMLSegmentEditorNode::PaintAllowedOutsideAllSegments);
  this->MaskModeComboBox->addItem(QObject::tr("Outside all visible segments"), vtkMRMLSegmentEditorNode::PaintAllowedOutsideVisibleSegments);
  this->MaskModeComboBox->insertSeparator(this->MaskModeComboBox->count());
  this->MaskModeComboBoxFixedItemsCount = this->MaskModeComboBox->count();

  this->OverwriteModeComboBox->addItem(QObject::tr("All segments"), vtkMRMLSegmentEditorNode::OverwriteAllSegments);
  this->OverwriteModeComboBox->addItem(QObject::tr("Visible segments"), vtkMRMLSegmentEditorNode::OverwriteVisibleSegments);
  this->OverwriteModeComboBox->addItem(QObject::tr("None"), vtkMRMLSegmentEditorNode::OverwriteNone);

  // Make connections
  QObject::connect( this->SegmentationNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(onSegmentationNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->MasterVolumeNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    q, SLOT(onMasterVolumeNodeChanged(vtkMRMLNode*)) );
  QObject::connect( this->SegmentsTableView, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
    q, SLOT(onSegmentSelectionChanged(QItemSelection,QItemSelection)) );
  QObject::connect( this->SegmentsTableView, SIGNAL(segmentAboutToBeModified(QString)),
    q, SLOT(saveStateForUndo()) );
  QObject::connect( this->AddSegmentButton, SIGNAL(clicked()), q, SLOT(onAddSegment()) );
  QObject::connect( this->RemoveSegmentButton, SIGNAL(clicked()), q, SLOT(onRemoveSegment()) );
  QObject::connect( this->CreateSurfaceButton, SIGNAL(toggled(bool)), q, SLOT(onCreateSurfaceToggled(bool)) );

  QObject::connect( this->MaskModeComboBox, SIGNAL(currentIndexChanged(int)), q, SLOT(onMaskModeChanged(int)));
  QObject::connect( this->MasterVolumeIntensityMaskCheckBox, SIGNAL(toggled(bool)), q, SLOT(onMasterVolumeIntensityMaskChecked(bool)));
  QObject::connect( this->MasterVolumeIntensityMaskRangeWidget, SIGNAL(valuesChanged(double,double)), q, SLOT(onMasterVolumeIntensityMaskRangeChanged(double,double)));
  QObject::connect( this->OverwriteModeComboBox, SIGNAL(currentIndexChanged(int)), q, SLOT(onOverwriteModeChanged(int)));

  QObject::connect( this->UndoButton, SIGNAL(clicked()), q, SLOT(undo()) );
  QObject::connect( this->RedoButton, SIGNAL(clicked()), q, SLOT(redo()) );

  QObject::connect( this->EffectHelpBrowser, SIGNAL(anchorClicked(QUrl)), q, SLOT(anchorClicked(QUrl)), Qt::QueuedConnection );

  q->qvtkConnect(this->SegmentationHistory, vtkCommand::ModifiedEvent,
    q, SLOT(onSegmentationHistoryChanged()));

  // Widget properties
  this->SegmentsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
  this->SegmentsTableView->setHeaderVisible(true);
  this->SegmentsTableView->setVisibilityColumnVisible(true);
  this->SegmentsTableView->setColorColumnVisible(true);
  this->SegmentsTableView->setOpacityColumnVisible(false);
  this->AddSegmentButton->setEnabled(false);
  this->RemoveSegmentButton->setEnabled(false);
  this->CreateSurfaceButton->setEnabled(false);
  this->EffectsGroupBox->setEnabled(false);
  this->OptionsGroupBox->setEnabled(false);

  this->EffectButtonGroup.setExclusive(true);
  QObject::connect(&this->EffectButtonGroup, SIGNAL(buttonClicked(QAbstractButton*)), q, SLOT(onEffectButtonClicked(QAbstractButton*) ) );

  // Create layout for effect options
  QVBoxLayout* layout = new QVBoxLayout(this->EffectsOptionsFrame);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  // Instantiate and expose effects

  // Setup effect button group layout
  ctkFlowLayout* effectsGroupLayout = new ctkFlowLayout();
  effectsGroupLayout->setContentsMargins(4, 4, 4, 4);
  effectsGroupLayout->setSpacing(4);
  effectsGroupLayout->setAlignItems(false);
  effectsGroupLayout->setAlignment(Qt::AlignJustify);
  effectsGroupLayout->setPreferredExpandingDirections(Qt::Vertical);
  this->EffectsGroupBox->setLayout(effectsGroupLayout);

  // Update effect buttons
  this->updateEffectList();

  this->OptionsGroupBox->hide();
  this->OptionsGroupBox->setTitle("");
  this->EffectHelpBrowser->setText("");
  this->MaskingGroupBox->hide();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::updateEffectList()
{
  Q_Q(qMRMLSegmentEditorWidget);

  // Create local copy of factory effects, so that
  // - Effects can have different parameters
  // - Segment editors can have different active effects
  QList<qSlicerSegmentEditorAbstractEffect*> addedEffects = qSlicerSegmentEditorEffectFactory::instance()->copyEffects(this->RegisteredEffects);

  // Set up effect connections and options frame for all newly added effects
  foreach(qSlicerSegmentEditorAbstractEffect* effect, addedEffects)
    {
    // Connect callbacks that allow effects to send requests to the editor widget without
    // introducing a direct dependency of the effect on the widget.
    effect->setCallbackSlots(q,
      SLOT(setActiveEffectByName(QString)),
      SLOT(updateVolume(void*, bool&)),
      SLOT(saveStateForUndo()));

    effect->setVolumes(this->AlignedMasterVolume, this->ModifierLabelmap, this->MaskLabelmap, this->SelectedSegmentLabelmap, this->ReferenceGeometryImage);

    // Add effect options frame to the options widget and hide them
    effect->setupOptionsFrame();
    QFrame* effectOptionsFrame = effect->optionsFrame();
    effectOptionsFrame->setVisible(false);
    this->EffectsOptionsFrame->layout()->addWidget(effectOptionsFrame);
    }

  // Update button list

  // Deactivate possible previous buttons
  QList<QAbstractButton*> effectButtons = this->EffectButtonGroup.buttons();
  foreach (QAbstractButton* button, effectButtons)
    {
    this->EffectButtonGroup.removeButton(button);
    button->deleteLater();
    }

  // Add NULL effect (arrow button to deactivate all effects)
  QToolButton* effectButton = new QToolButton(this->EffectsGroupBox);
  effectButton->setObjectName(NULL_EFFECT_NAME);
  effectButton->setCheckable(true);
  effectButton->setIcon(QIcon(":Icons/NullEffect.png"));
  effectButton->setText("None");
  effectButton->setToolTip("No editing");
  effectButton->setToolButtonStyle(this->EffectButtonStyle);
  effectButton->setProperty("Effect", QVariant::fromValue<QObject*>(NULL));
  effectButton->setSizePolicy(QSizePolicy::MinimumExpanding, effectButton->sizePolicy().verticalPolicy());
  this->EffectButtonGroup.addButton(effectButton);
  this->EffectsGroupBox->layout()->addWidget(effectButton);


  QList<qSlicerSegmentEditorAbstractEffect*> displayedEffects; // list of effect buttons to be displayed
  QList<qSlicerSegmentEditorAbstractEffect*> unorderedEffects = this->RegisteredEffects;

  // Add effects in the requested order
  foreach(QString effectName, this->EffectNameOrder)
    {
    qSlicerSegmentEditorAbstractEffect* effect = q->effectByName(effectName);
    if (effect)
      {
      displayedEffects << effect;
      unorderedEffects.removeOne(effect);
      }
    }
  // Add unordered effects
  if (this->UnorderedEffectsVisible)
    {
    displayedEffects << unorderedEffects;
    }

  // Create buttons
  foreach(qSlicerSegmentEditorAbstractEffect* effect, displayedEffects)
    {
    QToolButton* effectButton = new QToolButton(this->EffectsGroupBox);
    effectButton->setObjectName(effect->name());
    effectButton->setCheckable(true);
    effectButton->setToolButtonStyle(this->EffectButtonStyle);
    effectButton->setIcon(effect->icon());
    effectButton->setText(effect->name());
    effectButton->setToolTip(effect->name());
    effectButton->setSizePolicy(QSizePolicy::MinimumExpanding, effectButton->sizePolicy().verticalPolicy());
    effectButton->setProperty("Effect", QVariant::fromValue<QObject*>(effect));

    this->EffectButtonGroup.addButton(effectButton);
    this->EffectsGroupBox->layout()->addWidget(effectButton);
    }

}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::notifyEffectsOfReferenceGeometryChange(const std::string& geometry)
{
  if (geometry.compare(this->LastNotifiedReferenceImageGeometry) == 0)
    {
    // no change
    return;
    }
  this->LastNotifiedReferenceImageGeometry = geometry;

  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
    {
    effect->referenceGeometryChanged();
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::notifyEffectsOfMasterVolumeNodeChange()
{
  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
    {
    effect->masterVolumeNodeChanged();
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::notifyEffectsOfLayoutChange()
{
  foreach(qSlicerSegmentEditorAbstractEffect* effect, this->RegisteredEffects)
    {
    effect->layoutChanged();
    }
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::resetModifierLabelmapToDefault()
{
  std::string referenceImageGeometry = this->referenceImageGeometry();
  if (referenceImageGeometry.empty())
    {
    qCritical() << Q_FUNC_INFO << ": Cannot determine default modifierLabelmap geometry";
    return false;
    }

  std::string modifierLabelmapReferenceImageGeometryBaseline = vtkSegmentationConverter::SerializeImageGeometry(this->ModifierLabelmap);

  // Set reference geometry to labelmap (origin, spacing, directions, extents) and allocate scalars
  vtkNew<vtkMatrix4x4> referenceGeometryMatrix;
  int referenceExtent[6] = {0,-1,0,-1,0,-1};
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, referenceGeometryMatrix.GetPointer(), referenceExtent);
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, this->ModifierLabelmap, true, BINARY_LABELMAP_SCALAR_TYPE, 1);

  vtkOrientedImageDataResample::FillImage(this->ModifierLabelmap, BINARY_LABELMAP_VOXEL_EMPTY);

  return true;
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::updateSelectedSegmentLabelmap()
{
  if (!this->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return false;
    }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  std::string referenceImageGeometry = this->referenceImageGeometry();
  if (!segmentationNode || referenceImageGeometry.empty())
    {
    return false;
    }
  const char* selectedSegmentID = this->ParameterSetNode->GetSelectedSegmentID();
  if (!selectedSegmentID)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment selection";
    return false;
    }

  vtkSegment* selectedSegment = segmentationNode->GetSegmentation()->GetSegment(selectedSegmentID);
  if (selectedSegment == NULL)
    {
    qWarning() << Q_FUNC_INFO << " failed: Segment " << selectedSegmentID << " not found in segmentation";
    return false;
    }
  vtkOrientedImageData* segmentLabelmap = vtkOrientedImageData::SafeDownCast(
    selectedSegment->GetRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()));
  if (!segmentLabelmap)
    {
    qCritical() << Q_FUNC_INFO << ": Failed to get binary labelmap representation in segmentation " << segmentationNode->GetName();
    return false;
    }
  int* extent = segmentLabelmap->GetExtent();
  if (extent[0] > extent[1] || extent[2] > extent[3] || extent[4] > extent[5])
    {
    vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, this->SelectedSegmentLabelmap, false);
    this->SelectedSegmentLabelmap->SetExtent(0, -1, 0, -1, 0, -1);
    this->SelectedSegmentLabelmap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    return true;
    }
  vtkNew<vtkOrientedImageData> referenceImage;
  vtkNew<vtkMatrix4x4> referenceImageToWorld;
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, referenceImage.GetPointer(), false);
  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(segmentLabelmap, referenceImage.GetPointer(), this->SelectedSegmentLabelmap, /*linearInterpolation=*/false);

  return true;
}


//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::updateAlignedMasterVolume()
{
  if (!this->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return false;
    }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  vtkMRMLScalarVolumeNode* masterVolumeNode = this->ParameterSetNode->GetMasterVolumeNode();
  std::string referenceImageGeometry = this->referenceImageGeometry();
  if (!segmentationNode || !masterVolumeNode || referenceImageGeometry.empty())
    {
    return false;
    }

  vtkNew<vtkOrientedImageData> referenceImage;
  vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, referenceImage.GetPointer(), false);

  int* referenceImageExtent = referenceImage->GetExtent();
  int* alignedMasterVolumeExtent = this->AlignedMasterVolume->GetExtent();
  // If master volume node and transform nodes did not change and the aligned master volume covers the entire reference geometry
  // then we don't need to update the aligned master volume.
  if (vtkOrientedImageDataResample::DoGeometriesMatch(referenceImage.GetPointer(), this->AlignedMasterVolume)
    && alignedMasterVolumeExtent[0] <= referenceImageExtent[0] && alignedMasterVolumeExtent[1] >= referenceImageExtent[1]
    && alignedMasterVolumeExtent[2] <= referenceImageExtent[2] && alignedMasterVolumeExtent[3] >= referenceImageExtent[3]
    && alignedMasterVolumeExtent[4] <= referenceImageExtent[4] && alignedMasterVolumeExtent[5] >= referenceImageExtent[5]
    && vtkOrientedImageDataResample::DoExtentsMatch(referenceImage.GetPointer(), this->AlignedMasterVolume)
    && this->AlignedMasterVolumeUpdateMasterVolumeNode == masterVolumeNode
    && this->AlignedMasterVolumeUpdateMasterVolumeNodeTransform == masterVolumeNode->GetParentTransformNode()
    && this->AlignedMasterVolumeUpdateSegmentationNodeTransform == segmentationNode->GetParentTransformNode() )
    {
    // Extents and nodes are matching, check if they have not been modified since the aligned master
    // volume generation.
    bool updateAlignedMasterVolumeRequired = false;
    if (masterVolumeNode->GetMTime() > this->AlignedMasterVolume->GetMTime())
      {
      updateAlignedMasterVolumeRequired = true;
      }
    else if (masterVolumeNode->GetParentTransformNode() && masterVolumeNode->GetParentTransformNode()->GetMTime() > this->AlignedMasterVolume->GetMTime())
      {
      updateAlignedMasterVolumeRequired = true;
      }
    else if (segmentationNode->GetParentTransformNode() && segmentationNode->GetParentTransformNode()->GetMTime() > this->AlignedMasterVolume->GetMTime())
      {
      updateAlignedMasterVolumeRequired = true;
      }
    if (!updateAlignedMasterVolumeRequired)
      {
      return true;
      }
    }

  // Get a read-only version of masterVolume as a vtkOrientedImageData
  vtkNew<vtkOrientedImageData> masterVolume;
  masterVolume->vtkImageData::ShallowCopy(masterVolumeNode->GetImageData());
  vtkSmartPointer<vtkMatrix4x4> ijkToRasMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
  masterVolumeNode->GetIJKToRASMatrix(ijkToRasMatrix);
  masterVolume->SetGeometryFromImageToWorldMatrix(ijkToRasMatrix);

  vtkNew<vtkGeneralTransform> masterVolumeToSegmentationTransform;
  vtkMRMLTransformNode::GetTransformBetweenNodes(masterVolumeNode->GetParentTransformNode(), segmentationNode->GetParentTransformNode(), masterVolumeToSegmentationTransform.GetPointer());

  vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(masterVolume.GetPointer(), referenceImage.GetPointer(), this->AlignedMasterVolume,
    /*linearInterpolation=*/true, /*padImage=*/false, masterVolumeToSegmentationTransform.GetPointer());

  this->AlignedMasterVolumeUpdateMasterVolumeNode = masterVolumeNode;
  this->AlignedMasterVolumeUpdateMasterVolumeNodeTransform = masterVolumeNode->GetParentTransformNode();
  this->AlignedMasterVolumeUpdateSegmentationNodeTransform = segmentationNode->GetParentTransformNode();

  return true;
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::updateMaskLabelmap()
{
  if (!this->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return false;
    }

  vtkOrientedImageData* maskImage = this->MaskLabelmap;
  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  if (!this->ModifierLabelmap || !segmentationNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment selection!";
    return false;
    }
  if (!this->ParameterSetNode->GetSelectedSegmentID())
    {
    return false;
    }

  std::vector<std::string> allSegmentIDs;
  segmentationNode->GetSegmentation()->GetSegmentIDs(allSegmentIDs);

  std::vector<std::string> visibleSegmentIDs;
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (displayNode)
    {
    for (std::vector<std::string>::iterator segmentIDIt = allSegmentIDs.begin(); segmentIDIt != allSegmentIDs.end(); ++segmentIDIt)
      {
      if (displayNode->GetSegmentVisibility(*segmentIDIt))
        {
        visibleSegmentIDs.push_back(*segmentIDIt);
        }
      }
    }

  std::string editedSegmentID(this->ParameterSetNode->GetSelectedSegmentID());

  std::vector<std::string> maskSegmentIDs;
  bool paintInsideSegments = false;
  switch (this->ParameterSetNode->GetMaskMode())
    {
  case vtkMRMLSegmentEditorNode::PaintAllowedEverywhere:
    paintInsideSegments = false;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedInsideAllSegments:
    paintInsideSegments = true;
    maskSegmentIDs = allSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedInsideVisibleSegments:
    paintInsideSegments = true;
    maskSegmentIDs = visibleSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedOutsideAllSegments:
    paintInsideSegments = false;
    maskSegmentIDs = allSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedOutsideVisibleSegments:
    paintInsideSegments = false;
    maskSegmentIDs = visibleSegmentIDs;
    break;
  case vtkMRMLSegmentEditorNode::PaintAllowedInsideSingleSegment:
    paintInsideSegments = true;
    if (this->ParameterSetNode->GetMaskSegmentID())
      {
      maskSegmentIDs.push_back(this->ParameterSetNode->GetMaskSegmentID());
      }
    else
      {
      qWarning() << Q_FUNC_INFO << ": PaintAllowedInsideSingleSegment selected but no mask segment is specified";
      }
    break;
  default:
    qWarning() << Q_FUNC_INFO << ": unknown mask mode";
    }

  // Always allow paint inside edited segment
  if (paintInsideSegments)
    {
    // include edited segment in "inside" mask
    if (std::find(maskSegmentIDs.begin(), maskSegmentIDs.end(), editedSegmentID) == maskSegmentIDs.end())
      {
      // add it if it's not in the segment list already
      maskSegmentIDs.push_back(editedSegmentID);
      }
    }
  else
    {
    // exclude edited segment from "outside" mask
    maskSegmentIDs.erase(std::remove(maskSegmentIDs.begin(), maskSegmentIDs.end(), editedSegmentID), maskSegmentIDs.end());
    }

  // Update mask if modifier labelmap is valid
  int modifierLabelmapExtent[6] = { 0, -1, 0, -1, 0, -1 };
  this->ModifierLabelmap->GetExtent(modifierLabelmapExtent);
  if (modifierLabelmapExtent[0] <= modifierLabelmapExtent[1]
    && modifierLabelmapExtent[2] <= modifierLabelmapExtent[3]
    && modifierLabelmapExtent[4] <= modifierLabelmapExtent[5])
    {
    maskImage->SetExtent(modifierLabelmapExtent);
    maskImage->AllocateScalars(VTK_SHORT, 1); // Change scalar type from unsigned int back to short for merged labelmap generation

    segmentationNode->GenerateMergedLabelmap(maskImage, vtkSegmentation::EXTENT_UNION_OF_SEGMENTS, this->ModifierLabelmap, maskSegmentIDs);
    vtkSmartPointer<vtkMatrix4x4> mergedImageToWorldMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    maskImage->GetImageToWorldMatrix(mergedImageToWorldMatrix);

    vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
    threshold->SetInputData(maskImage);
    threshold->SetInValue(paintInsideSegments ? 1 : 0);
    threshold->SetOutValue(paintInsideSegments ? 0 : 1);
    threshold->ReplaceInOn();
    threshold->ThresholdByLower(0);
    threshold->SetOutputScalarType(VTK_UNSIGNED_CHAR);
    threshold->Update();

    maskImage->DeepCopy(threshold->GetOutput());
    maskImage->SetImageToWorldMatrix(mergedImageToWorldMatrix);
    }
  return true;
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidgetPrivate::updateReferenceGeometryImage()
{
  std::string geometry = this->referenceImageGeometry();
  if (geometry.empty())
    {
    return false;
    }
  return vtkSegmentationConverter::DeserializeImageGeometry(geometry, this->ReferenceGeometryImage, false /* don't allocate scalars */);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::selectFirstSegment()
{
  if (!this->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  if ( segmentationNode
    && segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0 )
    {
    std::vector<std::string> segmentIDs;
    segmentationNode->GetSegmentation()->GetSegmentIDs(segmentIDs);

    QStringList firstSegmentID;
    firstSegmentID << QString(segmentIDs[0].c_str());
    this->SegmentsTableView->setSelectedSegmentIDs(firstSegmentID);
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidgetPrivate::setEffectCursor(qSlicerSegmentEditorAbstractEffect* effect)
{
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
    {
    // application is closing
    return;
    }

  foreach(QString sliceViewName, layoutManager->sliceViewNames())
    {
    qMRMLSliceWidget* sliceWidget = layoutManager->sliceWidget(sliceViewName);
    if (effect && effect->showEffectCursorInSliceView())
      {
      sliceWidget->sliceView()->setCursor(effect->createCursor(sliceWidget));
      }
    else
      {
      sliceWidget->sliceView()->unsetCursor();
      }
    }
  for (int threeDViewId = 0; threeDViewId < layoutManager->threeDViewCount(); ++threeDViewId)
    {
    qMRMLThreeDWidget* threeDWidget = layoutManager->threeDWidget(threeDViewId);
    if (effect && effect->showEffectCursorInThreeDView())
      {
      threeDWidget->threeDView()->setCursor(effect->createCursor(threeDWidget));
      }
    else
      {
      threeDWidget->threeDView()->unsetCursor();
      }
    }
}

//-----------------------------------------------------------------------------
std::string qMRMLSegmentEditorWidgetPrivate::getReferenceImageGeometryFromSegmentation(vtkSegmentation* segmentation)
{
  if (!segmentation)
    {
    return "";
    }

  // If "reference image geometry" conversion parameter is set then use that
  std::string referenceImageGeometry = segmentation->GetConversionParameter(vtkSegmentationConverter::GetReferenceImageGeometryParameterName());
  if (!referenceImageGeometry.empty())
    {
    // Extend reference image geometry to contain all segments (needed for example for properly handling imported segments
    // that do not fit into the reference image geometry)
    vtkSmartPointer<vtkOrientedImageData> commonGeometryImage = vtkSmartPointer<vtkOrientedImageData>::New();
    vtkSegmentationConverter::DeserializeImageGeometry(referenceImageGeometry, commonGeometryImage, false);
    // Determine extent that contains all segments
    int commonSegmentExtent[6] = { 0, -1, 0, -1, 0, -1 };
    segmentation->DetermineCommonLabelmapExtent(commonSegmentExtent, commonGeometryImage);
    if (commonSegmentExtent[0] <= commonSegmentExtent[1]
      && commonSegmentExtent[2] <= commonSegmentExtent[3]
      && commonSegmentExtent[4] <= commonSegmentExtent[5])
      {
      // Expand commonGeometryExtent as needed to contain commonSegmentExtent
      int commonGeometryExtent[6] = { 0, -1, 0, -1, 0, -1 };
      commonGeometryImage->GetExtent(commonGeometryExtent);
      for (int i = 0; i < 3; i++)
        {
        commonGeometryExtent[i * 2] = std::min(commonSegmentExtent[i * 2], commonGeometryExtent[i * 2]);
        commonGeometryExtent[i * 2 + 1] = std::max(commonSegmentExtent[i * 2 + 1], commonGeometryExtent[i * 2 + 1]);
        }
      commonGeometryImage->SetExtent(commonGeometryExtent);
      referenceImageGeometry = vtkSegmentationConverter::SerializeImageGeometry(commonGeometryImage);
      }

    // TODO: Use oversampling (if it's 'A' then ignore and changed to 1)
    return referenceImageGeometry;
    }
  if (segmentation->ContainsRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
    // If no reference image geometry is specified but there are labels already then determine geometry from that
    referenceImageGeometry = segmentation->DetermineCommonLabelmapGeometry();
    return referenceImageGeometry;
    }
  return "";
}

//-----------------------------------------------------------------------------
std::string qMRMLSegmentEditorWidgetPrivate::referenceImageGeometry()
{
  if (!this->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    notifyEffectsOfReferenceGeometryChange("");
    return "";
    }

  vtkMRMLSegmentationNode* segmentationNode = this->ParameterSetNode->GetSegmentationNode();
  vtkSegmentation* segmentation = segmentationNode ? segmentationNode->GetSegmentation() : NULL;
  if (!segmentationNode || !segmentation)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segmentation";
    notifyEffectsOfReferenceGeometryChange("");
    return "";
    }

  std::string referenceImageGeometry;
  referenceImageGeometry = this->getReferenceImageGeometryFromSegmentation(segmentation);
  if (referenceImageGeometry.empty())
    {
    // If no reference image geometry could be determined then use the master volume's geometry
    vtkMRMLScalarVolumeNode* masterVolumeNode = this->ParameterSetNode->GetMasterVolumeNode();
    if (!masterVolumeNode)
      {
      // cannot determine reference geometry
      return "";
      }
    // Update the referenceImageGeometry parameter for next time
    segmentationNode->SetReferenceImageGeometryParameterFromVolumeNode(masterVolumeNode);
    // Update extents to include all existing segments
    referenceImageGeometry = this->getReferenceImageGeometryFromSegmentation(segmentation);
    }
  notifyEffectsOfReferenceGeometryChange(referenceImageGeometry);
  return referenceImageGeometry;
}

//-----------------------------------------------------------------------------
// qMRMLSegmentEditorWidget methods

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidget::qMRMLSegmentEditorWidget(QWidget* _parent)
  : qMRMLWidget(_parent)
  , d_ptr(new qMRMLSegmentEditorWidgetPrivate(*this))
{
  Q_D(qMRMLSegmentEditorWidget);
  d->init();
}

//-----------------------------------------------------------------------------
qMRMLSegmentEditorWidget::~qMRMLSegmentEditorWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!this->mrmlScene() || this->mrmlScene()->IsClosing())
    {
    return;
    }

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  int wasModified = d->ParameterSetNode->StartModify();

  updateWidgetFromSegmentationNode();
  updateWidgetFromMasterVolumeNode();

  d->EffectsGroupBox->setEnabled(d->SegmentationNode != NULL);
  d->MaskingGroupBox->setEnabled(d->SegmentationNode != NULL);
  d->EffectsOptionsFrame->setEnabled(d->SegmentationNode != NULL);
  d->MasterVolumeNodeComboBox->setEnabled(d->SegmentationNode != NULL);

  QString selectedSegmentID;
  if (d->ParameterSetNode->GetSelectedSegmentID())
    {
    selectedSegmentID = QString(d->ParameterSetNode->GetSelectedSegmentID());
    }

  // Disable adding new segments until master volume is set (or reference geometry is specified for the segmentation).
  // This forces the user to select a master volume before start adding segments.
  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  bool enableAddSegments = (segmentationNode != NULL) && (!d->Locked) && ((d->MasterVolumeNode != NULL) || (!d->referenceImageGeometry().empty()));
  d->AddSegmentButton->setEnabled(enableAddSegments);

  // Only enable remove button if a segment is selected
  d->RemoveSegmentButton->setEnabled(!selectedSegmentID.isEmpty() && (!d->Locked));

  d->CreateSurfaceButton->setEnabled(!d->Locked);

  // Segments list section
  if (!selectedSegmentID.isEmpty())
    {
    if (segmentationNode)
      {
      QStringList segmentID;
      segmentID << QString(selectedSegmentID);
      d->SegmentsTableView->setSelectedSegmentIDs(segmentID);
      }
    }
  else
    {
    d->SegmentsTableView->clearSelection();
    }
  d->SegmentsTableView->setReadOnly(d->Locked);

  // Effects section (list and options)
  updateEffectsSectionFromMRML();

  // Undo/redo section

  d->UndoButton->setEnabled(!d->Locked);
  d->RedoButton->setEnabled(!d->Locked);

  // Masking section

  bool wasBlocked = d->MaskModeComboBox->blockSignals(true);
  int maskModeIndex = -1;
  if (d->ParameterSetNode->GetMaskMode() == vtkMRMLSegmentEditorNode::PaintAllowedInsideSingleSegment)
    {
    // segment item
    maskModeIndex = d->MaskModeComboBox->findData(d->ParameterSetNode->GetMaskSegmentID());
    }
  else
    {
    // fixed item, identified by overwrite mode id
    maskModeIndex = d->MaskModeComboBox->findData(d->ParameterSetNode->GetMaskMode());
    }
  d->MaskModeComboBox->setCurrentIndex(maskModeIndex);
  d->MaskModeComboBox->blockSignals(wasBlocked);

  wasBlocked = d->MasterVolumeIntensityMaskCheckBox->blockSignals(true);
  d->MasterVolumeIntensityMaskCheckBox->setChecked(d->ParameterSetNode->GetMasterVolumeIntensityMask());
  d->MasterVolumeIntensityMaskCheckBox->blockSignals(wasBlocked);

  // Initialize mask range if it has never set and intensity masking es enabled
  if (d->ParameterSetNode->GetMasterVolumeIntensityMask()
    && d->ParameterSetNode->GetMasterVolumeIntensityMaskRange()[0] == d->ParameterSetNode->GetMasterVolumeIntensityMaskRange()[1])
    {
    // threshold was uninitialized, set some default
    double range[2] = { 0.0 };
    d->MasterVolumeNode->GetImageData()->GetScalarRange(range);
    d->ParameterSetNode->SetMasterVolumeIntensityMaskRange(range[0] + 0.25*(range[1] - range[0]), range[0] + 0.75*(range[1] - range[0]));
    }

  wasBlocked = d->MasterVolumeIntensityMaskRangeWidget->blockSignals(true);
  d->MasterVolumeIntensityMaskRangeWidget->setVisible(d->ParameterSetNode->GetMasterVolumeIntensityMask());
  d->MasterVolumeIntensityMaskRangeWidget->setMinimumValue(d->ParameterSetNode->GetMasterVolumeIntensityMaskRange()[0]);
  d->MasterVolumeIntensityMaskRangeWidget->setMaximumValue(d->ParameterSetNode->GetMasterVolumeIntensityMaskRange()[1]);
  d->MasterVolumeIntensityMaskRangeWidget->blockSignals(wasBlocked);

  wasBlocked = d->OverwriteModeComboBox->blockSignals(true);
  int overwriteModeIndex = d->OverwriteModeComboBox->findData(d->ParameterSetNode->GetOverwriteMode());
  d->OverwriteModeComboBox->setCurrentIndex(overwriteModeIndex);
  d->OverwriteModeComboBox->blockSignals(wasBlocked);

  // Segmentation object might have been replaced, update selected segment
  onSegmentAddedRemoved();

  d->ParameterSetNode->EndModify(wasModified);
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::setMasterRepresentationToBinaryLabelmap()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (d->SegmentationNode == NULL)
    {
    qDebug() << Q_FUNC_INFO << " failed: segmentation node is invalid.";
    return false;
    }

  if (d->SegmentationNode->GetSegmentation()->GetNumberOfSegments() < 1)
    {
    // If segmentation contains no segments, then set binary labelmap as master by default
    d->SegmentationNode->GetSegmentation()->SetMasterRepresentationName(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
    return true;
    }

  if (d->SegmentationNode->GetSegmentation()->GetMasterRepresentationName() == vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName())
    {
    // Current master representation is already binary labelmap
    return true;
    }

  // Editing is only possible if binary labelmap is the master representation
  // If master is not binary labelmap, then ask the user if they wants to make it master
  QString message = QString("Editing requires binary labelmap master representation, but currently the master representation is %1. "
    "Changing the master representation requires conversion. Some details may be lost during conversion process.\n\n"
    "Change master representation to binary labelmap?").
    arg(d->SegmentationNode->GetSegmentation()->GetMasterRepresentationName().c_str());
  QMessageBox::StandardButton answer =
    QMessageBox::question(NULL, tr("Change master representation to binary labelmap?"), message,
    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
  if (answer != QMessageBox::Yes)
    {
    // User rejected the conversion
    qDebug() << Q_FUNC_INFO << " failed: user rejected changing of master representation.";
    return false;
    }

  // All other representations are invalidated when changing to binary labelmap.
  // Re-creating closed surface if it was present before, so that changes can be seen.
  bool closedSurfacePresent = d->SegmentationNode->GetSegmentation()->ContainsRepresentation(
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());

  // Make sure binary labelmap representation exists
  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
  bool createBinaryLabelmapRepresentationSuccess = d->SegmentationNode->GetSegmentation()->CreateRepresentation(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
  QApplication::restoreOverrideCursor();
  if (!createBinaryLabelmapRepresentationSuccess)
    {
    QString message = QString("Failed to create binary labelmap representation in segmentation %1 for editing!\nPlease see Segmentations module for details.").
      arg(d->SegmentationNode->GetName());
    QMessageBox::critical(NULL, tr("Failed to create binary labelmap for editing"), message);
    qCritical() << Q_FUNC_INFO << ": " << message;
    return false;
    }

  QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));

  d->SegmentationNode->GetSegmentation()->SetMasterRepresentationName(
    vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());

  if (closedSurfacePresent)
    {
    d->SegmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
    }

  // Show binary labelmap in 2D
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(d->SegmentationNode->GetDisplayNode());
  if (displayNode)
    {
    displayNode->SetPreferredDisplayRepresentationName2D(
      vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
    }

  QApplication::restoreOverrideCursor();

  return true;
}


//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateWidgetFromSegmentationNode()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  // Save segmentation node selection
  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (segmentationNode != d->SegmentationNode)
    {
    // Connect events needed to update closed surface button
    qvtkReconnect(d->SegmentationNode, segmentationNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));
    qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::ContainedRepresentationNamesModified, this, SLOT(onSegmentAddedRemoved()));
    qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentAdded, this, SLOT(onSegmentAddedRemoved()));
    qvtkReconnect(d->SegmentationNode, segmentationNode, vtkSegmentation::SegmentRemoved, this, SLOT(onSegmentAddedRemoved()));
    d->SegmentationNode = segmentationNode;

    bool wasBlocked = d->SegmentationNodeComboBox->blockSignals(true);
    d->SegmentationNodeComboBox->setCurrentNode(d->SegmentationNode);
    d->SegmentationNodeComboBox->blockSignals(wasBlocked);

    wasBlocked = d->SegmentsTableView->blockSignals(true);
    d->SegmentsTableView->setSegmentationNode(d->SegmentationNode);
    d->SegmentsTableView->blockSignals(wasBlocked);

    if (segmentationNode)
      {
      // If a geometry reference volume was defined for this segmentation then select it as master volumeSelect master volume node
      vtkMRMLNode* referenceVolumeNode = segmentationNode->GetNodeReference(
        vtkMRMLSegmentationNode::GetReferenceImageGeometryReferenceRole().c_str());
      // Make sure the master volume selection is performed fully before proceeding
      d->MasterVolumeNodeComboBox->setCurrentNode(referenceVolumeNode);

      // Make sure there is a display node and get it
      segmentationNode->CreateDefaultDisplayNodes();
      vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());

      // Remember whether closed surface is present so that it can be re-converted later if necessary
      bool closedSurfacePresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
        vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
      bool binaryLabelmapPresent = d->SegmentationNode->GetSegmentation()->ContainsRepresentation(
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
      // Show closed surface in 3D if present
      if (displayNode && closedSurfacePresent)
        {
        displayNode->SetPreferredDisplayRepresentationName3D(
          vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
        }

      // Show binary labelmap in 2D
      if (displayNode && binaryLabelmapPresent)
        {
        displayNode->SetPreferredDisplayRepresentationName2D(
          vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
        }

      if (segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
        {
        // Select first segment to enable all effects (including per-segment ones)
        d->selectFirstSegment();
        }

      // Set label layer to empty, because edit actor will be shown in the slice views during editing
      vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
      if (selectionNode)
        {
        selectionNode->SetActiveLabelVolumeID(NULL);
        qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection(vtkMRMLApplicationLogic::LabelLayer, 0);
        }
      else
        {
        qCritical() << Q_FUNC_INFO << ": Unable to get selection node to show segmentation node " << segmentationNode->GetName();
        }
      }

    emit segmentationNodeChanged(d->SegmentationNode);
    }

  d->SegmentationHistory->SetSegmentation(d->SegmentationNode ? d->SegmentationNode->GetSegmentation() : NULL);

  // Update closed surface button with new segmentation
  this->onSegmentAddedRemoved();

}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateWidgetFromMasterVolumeNode()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
    {
    return;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  if (!segmentation)
    {
    return;
    }
  vtkMRMLScalarVolumeNode* masterVolumeNode = d->ParameterSetNode->GetMasterVolumeNode();
  if (masterVolumeNode == d->MasterVolumeNode)
    {
    // no change
    return;
    }

  qvtkReconnect(d->MasterVolumeNode, masterVolumeNode, vtkMRMLVolumeNode::ImageDataModifiedEvent, this, SLOT(onMasterVolumeImageDataModified()));
  d->MasterVolumeNode = masterVolumeNode;

  bool wasBlocked = d->MasterVolumeNodeComboBox->blockSignals(true);
  d->MasterVolumeNodeComboBox->setCurrentNode(d->MasterVolumeNode);
  d->MasterVolumeNodeComboBox->blockSignals(wasBlocked);

  this->onMasterVolumeImageDataModified();

  emit masterVolumeNodeChanged(d->MasterVolumeNode);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeImageDataModified()
{
  Q_D(qMRMLSegmentEditorWidget);

  // Update intensity range slider widget
  if (d->MasterVolumeNode != NULL && d->MasterVolumeNode->GetImageData() != NULL)
    {
    double range[2] = { 0.0, 0.0 };
    d->MasterVolumeNode->GetImageData()->GetScalarRange(range);
    d->MasterVolumeIntensityMaskRangeWidget->setMinimum(range[0]);
    d->MasterVolumeIntensityMaskRangeWidget->setMaximum(range[1]);
    d->MasterVolumeIntensityMaskRangeWidget->setEnabled(true);
    }
  else
    {
    d->MasterVolumeIntensityMaskRangeWidget->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qMRMLSegmentEditorWidget::activeEffect()const
{
  Q_D(const qMRMLSegmentEditorWidget);

  return d->ActiveEffect;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setActiveEffect(qSlicerSegmentEditorAbstractEffect* effect)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    if (effect != NULL)
      {
      qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
      }
    return;
    }

  d->ParameterSetNode->SetActiveEffectName(effect ? effect->name().toLatin1() : "");
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateEffectsSectionFromMRML()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  // Disable editing if no master volume node is set:
  // master volume determines the extent of editing, so even though the segmentation is valid
  // without a master volume, editing is not possible until it is selected.

  // Disable effect selection and options altogether if no master volume is selected
  bool effectsOverallEnabled = (d->ParameterSetNode->GetMasterVolumeNode() != NULL) && (!d->Locked);
  d->EffectsGroupBox->setEnabled(effectsOverallEnabled);
  d->OptionsGroupBox->setEnabled(effectsOverallEnabled);

  // Enable only non-per-segment effects if no segment is selected, otherwise enable all effects
  if (effectsOverallEnabled)
    {
    vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
    bool segmentAvailable = segmentationNode && (segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0);
    QString selectedSegmentID(d->ParameterSetNode->GetSelectedSegmentID());
    bool segmentSelected = !selectedSegmentID.isEmpty();
    QList<QAbstractButton*> effectButtons = d->EffectButtonGroup.buttons();
    foreach(QAbstractButton* effectButton, effectButtons)
      {
      qSlicerSegmentEditorAbstractEffect* effect = qobject_cast<qSlicerSegmentEditorAbstractEffect*>(
        effectButton->property("Effect").value<QObject*>());
      if (!effect)
        {
        // NULL effect
        continue;
        }
      effectButton->setEnabled(segmentAvailable && (segmentSelected || !effect->perSegment()));
      }
    }

  // Update effect options
  const char* activeEffectName = d->ParameterSetNode->GetActiveEffectName();
  qSlicerSegmentEditorAbstractEffect* activeEffect = this->effectByName(activeEffectName); // newly selected effect
  if (activeEffect == d->ActiveEffect)
    {
    return;
    }

  // Deactivate previously selected effect
  if (d->ActiveEffect)
    {
    d->ActiveEffect->deactivate();
    }

  if (activeEffect && !this->setMasterRepresentationToBinaryLabelmap())
    {
    // effect cannot be activated because master representation has to be binary labelmap
    qDebug() << Q_FUNC_INFO << ": Cannot activate effect, failed to set binary labelmap as master representation.";
    activeEffect = NULL;
    }

  if (activeEffect)
    {
    // Create observations between view interactors and the editor widget.
    // The captured events are propagated to the active effect if any.
    this->setupViewObservations();

    // Deactivate markup/ruler/ROI placement
    if (d->InteractionNode && d->InteractionNode->GetCurrentInteractionMode() != vtkMRMLInteractionNode::ViewTransform)
      {
      d->InteractionNode->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
      }

    // Activate newly selected effect
    activeEffect->activate();
    d->OptionsGroupBox->show();
    d->OptionsGroupBox->setTitle(activeEffect->name());
    d->EffectHelpBrowser->setCollapsibleText(activeEffect->helpText());
    d->MaskingGroupBox->show();

    // Perform updates to prevent layout collapse
    d->EffectHelpBrowser->setMinimumHeight(d->EffectHelpBrowser->sizeHint().height());
    d->EffectHelpBrowser->layout()->update();
    activeEffect->optionsFrame()->setMinimumHeight(activeEffect->optionsFrame()->sizeHint().height());
    activeEffect->optionsLayout()->activate();
    this->setMinimumHeight(this->sizeHint().height());
    }
  else
    {
    d->OptionsGroupBox->hide();
    d->OptionsGroupBox->setTitle("");
    d->EffectHelpBrowser->setText("");
    d->MaskingGroupBox->hide();

    this->removeViewObservations();
    }

  // Update button checked states
  QString effectName(NULL_EFFECT_NAME);
  if (activeEffect)
    {
    effectName = activeEffect->name();
    }
  QList<QAbstractButton*> effectButtons = d->EffectButtonGroup.buttons();
  foreach(QAbstractButton* effectButton, effectButtons)
    {
    bool checked = effectButton->isChecked();
    bool needToBeChecked = (effectButton->objectName().compare(effectName) == 0);
    if (checked != needToBeChecked)
      {
      bool wasBlocked = effectButton->blockSignals(true);
      effectButton->setChecked(needToBeChecked);
      effectButton->blockSignals(wasBlocked);
      }
    }

  // Set cursor for active effect
  d->setEffectCursor(activeEffect);

  // Set active effect
  d->LastActiveEffect = d->ActiveEffect;
  d->ActiveEffect = activeEffect;
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (newScene == this->mrmlScene())
    {
    return;
    }

  Superclass::setMRMLScene(newScene);

  // Make connections that depend on the Slicer application
  QObject::connect( qSlicerApplication::application()->layoutManager(), SIGNAL(layoutChanged(int)), this, SLOT(onLayoutChanged(int)) );

  vtkMRMLInteractionNode *interactionNode = NULL;
  if (newScene)
    {
    interactionNode = vtkMRMLInteractionNode::SafeDownCast(newScene->GetNodeByID("vtkMRMLInteractionNodeSingleton"));
    }
  this->qvtkReconnect(d->InteractionNode, interactionNode, vtkCommand::ModifiedEvent, this, SLOT(onInteractionNodeModified()));
  d->InteractionNode = interactionNode;

  // Update UI
  this->updateWidgetFromMRML();

  // observe close event so can re-add a parameters node if necessary
  this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::EndCloseEvent, this, SLOT(onMRMLSceneEndCloseEvent()));
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMRMLSceneEndCloseEvent()
{
  this->initializeParameterSetNode();
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onInteractionNodeModified()
{
  Q_D(const qMRMLSegmentEditorWidget);
  if (!d->InteractionNode || !d->ActiveEffect)
    {
    return;
    }
  // Only notify the active effect about interaction node changes
  // (inactive effects should not interact with the user)
  d->ActiveEffect->interactionNodeModified(d->InteractionNode);
}

//------------------------------------------------------------------------------
vtkMRMLSegmentEditorNode* qMRMLSegmentEditorWidget::mrmlSegmentEditorNode()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->ParameterSetNode;
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMRMLSegmentEditorNode(vtkMRMLSegmentEditorNode* newSegmentEditorNode)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (d->ParameterSetNode == newSegmentEditorNode)
    {
    return;
    }

  // Connect modified event on ParameterSetNode to updating the widget
  qvtkReconnect(d->ParameterSetNode, newSegmentEditorNode, vtkCommand::ModifiedEvent, this, SLOT(updateWidgetFromMRML()));

  // Set parameter set node
  d->ParameterSetNode = newSegmentEditorNode;

  if (!d->ParameterSetNode)
    {
    return;
    }

  this->initializeParameterSetNode();

  // Update UI
  this->updateWidgetFromMRML();
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::initializeParameterSetNode()
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
    {
    return;
    }
  // Set parameter set node to all effects
  foreach(qSlicerSegmentEditorAbstractEffect* effect, d->RegisteredEffects)
    {
    effect->setParameterSetNode(d->ParameterSetNode);
    effect->setMRMLDefaults();

    // Connect parameter modified event to update effect options widget
    qvtkReconnect(d->ParameterSetNode, vtkMRMLSegmentEditorNode::EffectParameterModified, effect, SLOT(updateGUIFromMRML()));
    }
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setSegmentationNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->SegmentationNodeComboBox->setCurrentNode(node);
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLSegmentEditorWidget::segmentationNode()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->SegmentationNodeComboBox->currentNode();
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setSegmentationNodeID(const QString& nodeID)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->SegmentationNodeComboBox->setCurrentNodeID(nodeID);
}

//------------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::segmentationNodeID()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->SegmentationNodeComboBox->currentNodeID();
}

//-----------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::currentSegmentID()const
{
  Q_D(const qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return QString();
    }

  const char* selectedSegmentID = d->ParameterSetNode->GetSelectedSegmentID();
  return QString(selectedSegmentID);
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMasterVolumeNode(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (node && !d->MasterVolumeNodeComboBox->isEnabled())
    {
    qCritical() << Q_FUNC_INFO << ": Cannot set master volume until segmentation is selected!";
    return;
    }
  d->MasterVolumeNodeComboBox->setCurrentNode(node);
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qMRMLSegmentEditorWidget::masterVolumeNode()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MasterVolumeNodeComboBox->currentNode();
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMasterVolumeNodeID(const QString& nodeID)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->MasterVolumeNodeComboBox->isEnabled())
    {
    qCritical() << Q_FUNC_INFO << ": Cannot set master volume until segmentation is selected!";
    return;
    }
  d->MasterVolumeNodeComboBox->setCurrentNodeID(nodeID);
}

//------------------------------------------------------------------------------
QString qMRMLSegmentEditorWidget::masterVolumeNodeID()const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MasterVolumeNodeComboBox->currentNodeID();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSegmentationNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    d->SegmentationNodeComboBox->blockSignals(true);
    d->SegmentationNodeComboBox->setCurrentNode(NULL);
    d->SegmentationNodeComboBox->blockSignals(false);

    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  setActiveEffect(NULL); // deactivate current effect when we switch to a different segmentation
  d->ParameterSetNode->SetAndObserveSegmentationNode(vtkMRMLSegmentationNode::SafeDownCast(node));
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSegmentSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
  Q_UNUSED(selected);
  Q_UNUSED(deselected);
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  QStringList selectedSegmentIDs = d->SegmentsTableView->selectedSegmentIDs();

  // If selection did not change, then return
  QString currentSegmentID(d->ParameterSetNode->GetSelectedSegmentID());
  // Only the first selected segment is used (if multiple segments are selected then the others
  // are ignored; multi-select may be added in the future).
  QString selectedSegmentID(selectedSegmentIDs.isEmpty() ? QString() : selectedSegmentIDs[0]);
  if (!currentSegmentID.compare(selectedSegmentID))
    {
    return;
    }

  // Set segment ID if changed
  if (selectedSegmentIDs.isEmpty())
    {
    d->ParameterSetNode->SetSelectedSegmentID(NULL);
    // Also de-select current effect if per-segment
    if (d->ActiveEffect && d->ActiveEffect->perSegment())
      {
      this->setActiveEffect(NULL);
      }
    }
  else
    {
    d->ParameterSetNode->SetSelectedSegmentID(selectedSegmentID.toLatin1().constData());
    }

  // Disable editing if no segment is selected
  this->updateWidgetFromMRML();

  emit currentSegmentIDChanged(selectedSegmentID);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setCurrentSegmentID(const QString segmentID)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (d->ParameterSetNode)
    {
    d->ParameterSetNode->SetSelectedSegmentID(segmentID.toLatin1().constData());
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeNodeChanged(vtkMRMLNode* node)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    d->MasterVolumeNodeComboBox->blockSignals(true);
    d->MasterVolumeNodeComboBox->setCurrentNode(NULL);
    d->MasterVolumeNodeComboBox->blockSignals(false);

    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  // Cannot set master volume if no segmentation node is selected
  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
    {
    return;
    }

  // Set master volume to parameter set node
  vtkMRMLScalarVolumeNode* volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(node);
  if (d->ParameterSetNode->GetMasterVolumeNode() != volumeNode)
    {
    int wasModified = d->ParameterSetNode->StartModify();
    d->ParameterSetNode->SetAndObserveMasterVolumeNode(volumeNode);
    d->ParameterSetNode->EndModify(wasModified);

    if (volumeNode)
      {
      // Show reference volume in background layer of slice viewers
      vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
      if (!selectionNode)
        {
        qCritical() << Q_FUNC_INFO << ": Unable to get selection node to show volume node " << volumeNode->GetName();
        return;
        }
      selectionNode->SetActiveVolumeID(volumeNode->GetID());
      selectionNode->SetSecondaryVolumeID(NULL); // Hide foreground volume
      selectionNode->SetActiveLabelVolumeID(NULL); // Hide label volume
      qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
      }

    // Notify effects about change
    d->notifyEffectsOfMasterVolumeNodeChange();
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onEffectButtonClicked(QAbstractButton* button)
{
  // Get effect that was just clicked
  qSlicerSegmentEditorAbstractEffect* clickedEffect = qobject_cast<qSlicerSegmentEditorAbstractEffect*>(
    button->property("Effect").value<QObject*>() );

  this->setActiveEffect(clickedEffect);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onAddSegment()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
    {
    return;
    }

  d->SegmentationHistory->SaveState();

  // Create empty segment in current segmentation
  std::string addedSegmentID = segmentationNode->GetSegmentation()->AddEmptySegment();

  // Select the new segment
  if (!addedSegmentID.empty())
    {
    QStringList segmentIDList;
    segmentIDList << QString(addedSegmentID.c_str());
    d->SegmentsTableView->setSelectedSegmentIDs(segmentIDList);
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onRemoveSegment()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  std::string selectedSegmentID = (d->ParameterSetNode->GetSelectedSegmentID() ? d->ParameterSetNode->GetSelectedSegmentID() : "");
  if (!segmentationNode || selectedSegmentID.empty())
    {
    return;
    }

  d->SegmentationHistory->SaveState();

  // Switch to a new valid segment now (to avoid transient state when no segments are selected
  // as it could inactivate current effect).
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
  std::vector< std::string > segmentIDs;
  segmentation->GetSegmentIDs(segmentIDs);
  if (segmentIDs.size() > 1)
    {
    std::string newSelectedSegmentID;
    std::string previousSegmentID = segmentIDs.front();
    for (std::vector< std::string >::const_iterator segmentIdIt = segmentIDs.begin(); segmentIdIt != segmentIDs.end(); ++segmentIdIt)
      {
      if (*segmentIdIt == selectedSegmentID)
        {
        // found the currently selected segment
        // use the next segment (if at the end, use the previous)
        ++segmentIdIt;
        if (segmentIdIt != segmentIDs.end())
          {
          newSelectedSegmentID = *segmentIdIt;
          }
        else
          {
          newSelectedSegmentID = previousSegmentID;
          }
        break;
        }
      previousSegmentID = *segmentIdIt;
      }
    QStringList newSelectedSegmentIdList;
    newSelectedSegmentIdList << QString(newSelectedSegmentID.c_str());
    d->SegmentsTableView->setSelectedSegmentIDs(newSelectedSegmentIdList);
    }

  // Remove segment
  segmentationNode->GetSegmentation()->RemoveSegment(selectedSegmentID);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onCreateSurfaceToggled(bool on)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  vtkMRMLSegmentationNode* segmentationNode = d->ParameterSetNode->GetSegmentationNode();
  if (!segmentationNode)
    {
    return;
    }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(
    segmentationNode->GetDisplayNode());
  if (!displayNode)
    {
    return;
    }

  // If just have been checked, then create closed surface representation and show it
  if (on)
    {
    // Make sure closed surface representation exists
    if (segmentationNode->GetSegmentation()->CreateRepresentation(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() ))
      {
      // Set closed surface as displayed poly data representation
      displayNode->SetPreferredDisplayRepresentationName3D(
        vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );
      // But keep binary labelmap for 2D
      bool binaryLabelmapPresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
        vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());
      if (binaryLabelmapPresent)
        {
        displayNode->SetPreferredDisplayRepresentationName2D(
          vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() );
        }
      }
    }
  // If unchecked, then remove representation (but only if it's not the master representation)
  else if (segmentationNode->GetSegmentation()->GetMasterRepresentationName() !=
    vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
    {
    segmentationNode->GetSegmentation()->RemoveRepresentation(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());
    }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSegmentAddedRemoved()
{
  Q_D(qMRMLSegmentEditorWidget);

  vtkMRMLSegmentationNode* segmentationNode = NULL;
  if (d->ParameterSetNode)
    {
    segmentationNode = d->ParameterSetNode->GetSegmentationNode();
    }
  else
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    }

  // Update state of CreateSurfaceButton
  if (segmentationNode)
    {
    // Enable button if there is at least one segment in the segmentation
    d->CreateSurfaceButton->setEnabled(!d->Locked
      && segmentationNode->GetSegmentation()->GetNumberOfSegments()>0
      && segmentationNode->GetSegmentation()->GetMasterRepresentationName() !=
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName());

    // Change button state based on whether it contains closed surface representation
    bool closedSurfacePresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
      vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName() );
    d->CreateSurfaceButton->blockSignals(true);
    d->CreateSurfaceButton->setChecked(closedSurfacePresent);
    d->CreateSurfaceButton->blockSignals(false);
    }
  else
    {
    d->CreateSurfaceButton->setEnabled(false);
    }

  // Update mask mode combo box with current segment names

  bool wasBlocked = d->MaskModeComboBox->blockSignals(true);

  // save selection (if it's a non-fixed item)
  QString selectedSegmentId;
  if (d->MaskModeComboBox->currentIndex() >= d->MaskModeComboBoxFixedItemsCount)
    {
    selectedSegmentId = d->MaskModeComboBox->itemData(d->MaskModeComboBox->currentIndex()).toString();
    }

  // Remove segment names, keep only fixed items
  while (d->MaskModeComboBox->count() > d->MaskModeComboBoxFixedItemsCount)
    {
    d->MaskModeComboBox->removeItem(d->MaskModeComboBox->count()-1);
    }

  if (segmentationNode)
    {
    //bool labelmapPresent = segmentationNode->GetSegmentation()->ContainsRepresentation(
    //  vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName() );

    vtkSegmentation* segmentation = segmentationNode->GetSegmentation();
    std::vector< std::string > segmentIDs;
    segmentation->GetSegmentIDs(segmentIDs);
    for (std::vector< std::string >::const_iterator segmentIdIt = segmentIDs.begin(); segmentIdIt != segmentIDs.end(); ++segmentIdIt)
      {
      QString segmentName = segmentation->GetSegment(*segmentIdIt)->GetName();
      d->MaskModeComboBox->addItem(tr("Inside ") + segmentName, QString::fromLocal8Bit(segmentIdIt->c_str()));
      }

    // restore selection
    if (!selectedSegmentId.isEmpty())
      {
      int maskModeIndex = d->MaskModeComboBox->findData(selectedSegmentId);
      d->MaskModeComboBox->setCurrentIndex(maskModeIndex);
      }
    }
  d->MaskModeComboBox->blockSignals(wasBlocked);

  if (segmentationNode && d->MaskModeComboBox->currentIndex()<0)
    {
    // probably the currently selected mask segment was deleted,
    // switch to the first masking option (no mask).
    d->MaskModeComboBox->setCurrentIndex(0);
    }

}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onLayoutChanged(int layoutIndex)
{
  Q_D(qMRMLSegmentEditorWidget);
  Q_UNUSED(layoutIndex);

  // Refresh view observations with the new layout
  this->setupViewObservations();

  // Set volume selection to all slice viewers in new layout
  vtkMRMLSelectionNode* selectionNode = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  if (selectionNode && d->ParameterSetNode)
    {
    selectionNode->SetActiveVolumeID(d->ParameterSetNode->GetMasterVolumeNode() ? d->ParameterSetNode->GetMasterVolumeNode()->GetID() : NULL);
    selectionNode->SetSecondaryVolumeID(NULL);
    selectionNode->SetActiveLabelVolumeID(NULL);
    qSlicerCoreApplication::application()->applicationLogic()->PropagateVolumeSelection();
    }

  // Let effects know about the updated layout
  d->notifyEffectsOfLayoutChange();
}

//---------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qMRMLSegmentEditorWidget::effectByName(QString name)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (name.isEmpty())
    {
    return NULL;
    }

  // Find effect with name
  qSlicerSegmentEditorAbstractEffect* currentEffect = NULL;
  foreach(currentEffect, d->RegisteredEffects)
    {
    if (currentEffect->name().compare(name) == 0)
      {
      return currentEffect;
      }
    }

  return NULL;
}

//------------------------------------------------------------------------------
QStringList qMRMLSegmentEditorWidget::effectNameOrder() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->EffectNameOrder;
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setEffectNameOrder(const QStringList& effectNames)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (effectNames == d->EffectNameOrder)
    {
    // no change
    return;
    }
  d->EffectNameOrder = effectNames;
  d->updateEffectList();
}

//------------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::unorderedEffectsVisible() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->UnorderedEffectsVisible;
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setUnorderedEffectsVisible(bool visible)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (visible == d->UnorderedEffectsVisible)
    {
    // no change
    return;
    }
  d->UnorderedEffectsVisible = visible;
  d->updateEffectList();
}

//---------------------------------------------------------------------------
QStringList qMRMLSegmentEditorWidget::availableEffectNames()
{
  Q_D(qMRMLSegmentEditorWidget);
  QStringList availableEffectNames;
  foreach(qSlicerSegmentEditorAbstractEffect* effect, d->RegisteredEffects)
    {
    availableEffectNames << effect->name();
    }
  return availableEffectNames;
}

//---------------------------------------------------------------------------
int qMRMLSegmentEditorWidget::effectCount()
{
  Q_D(qMRMLSegmentEditorWidget);
  return d->EffectButtonGroup.buttons().count();
}

//---------------------------------------------------------------------------
qSlicerSegmentEditorAbstractEffect* qMRMLSegmentEditorWidget::effectByIndex(int index)
{
  Q_D(qMRMLSegmentEditorWidget);
  QList<QAbstractButton *> effectButtons = d->EffectButtonGroup.buttons();
  if (index < 0 || index >= effectButtons.count())
    {
    return NULL;
    }
  QAbstractButton* effectButton = effectButtons[index];
  qSlicerSegmentEditorAbstractEffect* effect = qobject_cast<qSlicerSegmentEditorAbstractEffect*>(
    effectButton->property("Effect").value<QObject*>());
  return effect;
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setupViewObservations()
{
  Q_D(qMRMLSegmentEditorWidget);

  // Make sure previous observations are cleared before setting up the new ones
  this->removeViewObservations();

  // Set up interactor observations
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();

  // Slice views
  foreach (QString sliceViewName, layoutManager->sliceViewNames())
    {
    // Create command for slice view
    qMRMLSliceWidget* sliceWidget = layoutManager->sliceWidget(sliceViewName);
    qMRMLSliceView* sliceView = sliceWidget->sliceView();
    vtkNew<vtkSegmentEditorEventCallbackCommand> interactionCallbackCommand;
    interactionCallbackCommand->EditorWidget = this;
    interactionCallbackCommand->ViewWidget = sliceWidget;
    interactionCallbackCommand->SetClientData( reinterpret_cast<void*>(interactionCallbackCommand.GetPointer()) );
    interactionCallbackCommand->SetCallback( qMRMLSegmentEditorWidget::processEvents );

    // Connect interactor events
    vtkRenderWindowInteractor* interactor = sliceView->interactorStyle()->GetInteractor();
    SegmentEditorEventObservation interactorObservation;
    interactorObservation.CallbackCommand = interactionCallbackCommand.GetPointer();
    interactorObservation.ObservedObject = interactor;
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeftButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::RightButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MiddleButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MiddleButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MouseMoveEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::KeyPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::KeyReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::EnterEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeaveEvent, interactorObservation.CallbackCommand, 1.0);
    d->EventObservations << interactorObservation;

    // Slice node observations
    vtkMRMLSliceNode* sliceNode = sliceWidget->sliceLogic()->GetSliceNode();
    SegmentEditorEventObservation sliceNodeObservation;
    sliceNodeObservation.CallbackCommand = interactionCallbackCommand.GetPointer();
    sliceNodeObservation.ObservedObject = sliceNode;
    sliceNodeObservation.ObservationTags << sliceNode->AddObserver(vtkCommand::ModifiedEvent, sliceNodeObservation.CallbackCommand, 1.0);
    d->EventObservations << sliceNodeObservation;
    }

  // 3D views
  for (int threeDViewId=0; threeDViewId<layoutManager->threeDViewCount(); ++threeDViewId)
    {
    // Create command for 3D view
    qMRMLThreeDWidget* threeDWidget = layoutManager->threeDWidget(threeDViewId);
    qMRMLThreeDView* threeDView = threeDWidget->threeDView();
    vtkNew<vtkSegmentEditorEventCallbackCommand> interactionCallbackCommand;
    interactionCallbackCommand->EditorWidget = this;
    interactionCallbackCommand->ViewWidget = threeDWidget;
    interactionCallbackCommand->SetClientData( reinterpret_cast<void*>(interactionCallbackCommand.GetPointer()) );
    interactionCallbackCommand->SetCallback( qMRMLSegmentEditorWidget::processEvents );

    // Connect interactor events
    vtkRenderWindowInteractor* interactor = threeDView->interactor();
    SegmentEditorEventObservation interactorObservation;
    interactorObservation.CallbackCommand = interactionCallbackCommand.GetPointer();
    interactorObservation.ObservedObject = interactor;
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeftButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeftButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::RightButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::RightButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MiddleButtonPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MiddleButtonReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MouseMoveEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MouseWheelForwardEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::MouseWheelBackwardEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::KeyPressEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::KeyReleaseEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::EnterEvent, interactorObservation.CallbackCommand, 1.0);
    interactorObservation.ObservationTags << interactor->AddObserver(vtkCommand::LeaveEvent, interactorObservation.CallbackCommand, 1.0);
    d->EventObservations << interactorObservation;

    // 3D view node observations
    vtkMRMLViewNode* viewNode = threeDWidget->mrmlViewNode();
    SegmentEditorEventObservation viewNodeObservation;
    viewNodeObservation.CallbackCommand = interactionCallbackCommand.GetPointer();
    viewNodeObservation.ObservedObject = viewNode;
    viewNodeObservation.ObservationTags << viewNode->AddObserver(vtkCommand::ModifiedEvent, viewNodeObservation.CallbackCommand, 1.0);
    d->EventObservations << viewNodeObservation;
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::removeViewObservations()
{
  Q_D(qMRMLSegmentEditorWidget);
  foreach (SegmentEditorEventObservation eventObservation, d->EventObservations)
    {
    if (eventObservation.ObservedObject)
      {
      foreach (int observationTag, eventObservation.ObservationTags)
        {
        eventObservation.ObservedObject->RemoveObserver(observationTag);
        }
      }
    }
  d->EventObservations.clear();
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setActiveEffectByName(QString effectName)
{
  qSlicerSegmentEditorAbstractEffect* effect = this->effectByName(effectName);
  this->setActiveEffect(effect);
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::saveStateForUndo()
{
  Q_D(qMRMLSegmentEditorWidget);
  if (d->SegmentationHistory->GetMaximumNumberOfStates() > 0)
    {
    d->SegmentationHistory->SaveState();
    }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateVolume(void* volumeToUpdate, bool& success)
{
  Q_D(qMRMLSegmentEditorWidget);

  if (volumeToUpdate == d->AlignedMasterVolume)
    {
    success = d->updateAlignedMasterVolume();
    }
  else if (volumeToUpdate == d->ModifierLabelmap)
    {
    success = d->resetModifierLabelmapToDefault();
    }
  else if (volumeToUpdate == d->MaskLabelmap)
    {
    success = d->updateMaskLabelmap();
    }
  else if (volumeToUpdate == d->SelectedSegmentLabelmap)
    {
    success = d->updateSelectedSegmentLabelmap();
    }
  else if (volumeToUpdate == d->ReferenceGeometryImage)
    {
    success = d->updateReferenceGeometryImage();
    }
  else
    {
    qCritical() << Q_FUNC_INFO << ": Failed to update volume";
    success = false;
    }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::processEvents(vtkObject* caller,
                                        unsigned long eid,
                                        void* clientData,
                                        void* vtkNotUsed(callData))
{
  // Get and parse client data
  vtkSegmentEditorEventCallbackCommand* callbackCommand = reinterpret_cast<vtkSegmentEditorEventCallbackCommand*>(clientData);
  qMRMLSegmentEditorWidget* self = callbackCommand->EditorWidget.data();
  qMRMLWidget* viewWidget = callbackCommand->ViewWidget.data();
  if (!self || !viewWidget)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid event data!";
    return;
    }
  // Do nothing if scene is closing
  if (!self->mrmlScene() || self->mrmlScene()->IsClosing())
    {
    return;
    }

  // Get active effect
  qSlicerSegmentEditorAbstractEffect* activeEffect = self->activeEffect();
  if (!activeEffect)
    {
    return;
    }

  // Call processing function of active effect. Handle both interactor and view node events
  vtkRenderWindowInteractor* callerInteractor = vtkRenderWindowInteractor::SafeDownCast(caller);
  vtkMRMLAbstractViewNode* callerViewNode = vtkMRMLAbstractViewNode::SafeDownCast(caller);
  if (callerInteractor)
    {
    bool abortEvent = activeEffect->processInteractionEvents(callerInteractor, eid, viewWidget);
    if (abortEvent)
      {
      /// Set the AbortFlag on the vtkCommand associated with the event.
      /// It causes other observers of the interactor not to receive the events.
      callbackCommand->SetAbortFlag(1);
      }
    }
  else if (callerViewNode)
    {
    activeEffect->processViewNodeEvents(callerViewNode, eid, viewWidget);
    }
  else
    {
    qCritical() << Q_FUNC_INFO << ": Unsupported caller object!";
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeIntensityMaskChecked(bool checked)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }
  d->ParameterSetNode->SetMasterVolumeIntensityMask(checked);
  /*
  this->ThresholdRangeWidget->blockSignals(true);
  this->ThresholdRangeWidget->setVisible(checked);
  this->ThresholdRangeWidget->blockSignals(false);
  */
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMasterVolumeIntensityMaskRangeChanged(double min, double max)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }
  d->ParameterSetNode->SetMasterVolumeIntensityMaskRange(min, max);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onMaskModeChanged(int index)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }

  QString selectedSegmentId;
  if (index >= d->MaskModeComboBoxFixedItemsCount)
    {
    // specific index is selected
    d->ParameterSetNode->SetMaskSegmentID(d->MaskModeComboBox->itemData(index).toString().toLatin1());
    d->ParameterSetNode->SetMaskMode(vtkMRMLSegmentEditorNode::PaintAllowedInsideSingleSegment);
    }
  else
    {
    // inside/outside all/visible segments
    d->ParameterSetNode->SetMaskMode(d->MaskModeComboBox->itemData(index).toInt());
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onOverwriteModeChanged(int index)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (!d->ParameterSetNode)
    {
    qCritical() << Q_FUNC_INFO << ": Invalid segment editor parameter set node";
    return;
    }
  d->ParameterSetNode->SetOverwriteMode(d->OverwriteModeComboBox->itemData(index).toInt());
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::segmentationNodeSelectorVisible() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->SegmentationNodeComboBox->isVisible();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setSegmentationNodeSelectorVisible(bool visible)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->SegmentationNodeComboBox->setVisible(visible);
  d->SegmentationNodeLabel->setVisible(visible);
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::masterVolumeNodeSelectorVisible() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->MasterVolumeNodeComboBox->isVisible();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMasterVolumeNodeSelectorVisible(bool visible)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MasterVolumeNodeComboBox->setVisible(visible);
  d->MasterVolumeNodeLabel->setVisible(visible);
}

//-----------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::undoEnabled() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return (d->UndoButton->isVisible() || d->RedoButton->isVisible());
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setUndoEnabled(bool enabled)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (enabled)
    {
    d->SegmentationHistory->RemoveAllStates();
    }
  d->UndoButton->setVisible(enabled);
  d->RedoButton->setVisible(enabled);
}

//-----------------------------------------------------------------------------
int qMRMLSegmentEditorWidget::maximumNumberOfUndoStates() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->SegmentationHistory->GetMaximumNumberOfStates();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setMaximumNumberOfUndoStates(int maxNumberOfStates)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->SegmentationHistory->SetMaximumNumberOfStates(maxNumberOfStates);
}

//------------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::readOnly() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->Locked;
}

//------------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setReadOnly(bool aReadOnly)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->Locked = aReadOnly;
  if (aReadOnly)
    {
    setActiveEffect(NULL);
    }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::undo()
{
  Q_D(qMRMLSegmentEditorWidget);
  if (d->SegmentationNode == NULL)
    {
    return;
    }
  d->SegmentationHistory->RestorePreviousState();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::redo()
{
  Q_D(qMRMLSegmentEditorWidget);
  d->SegmentationHistory->RestoreNextState();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSegmentationHistoryChanged()
{
  Q_D(qMRMLSegmentEditorWidget);
  d->UndoButton->setEnabled(d->SegmentationHistory->IsRestorePreviousStateAvailable());
  d->RedoButton->setEnabled(d->SegmentationHistory->IsRestoreNextStateAvailable());
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::installKeyboardShortcuts(QWidget* parent /*=NULL*/)
{
  Q_D(qMRMLSegmentEditorWidget);
  this->uninstallKeyboardShortcuts();

  if (parent == NULL)
    {
    parent = qSlicerApplication::application()->mainWindow();
    }

  // Keys 1, 2, ..., 9, 0 => toggle activation of effect 1..10
  for (int effectIndex = 1; effectIndex <= 10; effectIndex++)
    {
    QShortcut* s = new QShortcut(QKeySequence(QString::number(effectIndex % 10)), parent);
    d->KeyboardShortcuts.push_back(s);
    s->setProperty("effectIndex", effectIndex);
    QObject::connect(s, SIGNAL(activated()), this, SLOT(onSelectEffectShortcut()));
    }

  // Keys Shift + 1, 2, ..., 9, 0 => toggle activation of effect 1..10
  for (int effectIndex = 1; effectIndex <= 10; effectIndex++)
    {
    QShortcut* s = new QShortcut(QKeySequence("Shift+"+QString::number(effectIndex % 10)), parent);
    d->KeyboardShortcuts.push_back(s);
    s->setProperty("effectIndex", effectIndex+10);
    QObject::connect(s, SIGNAL(activated()), this, SLOT(onSelectEffectShortcut()));
    }

  // Escape => deactivate active effect
  QShortcut* deactivateEffectShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), parent);
  d->KeyboardShortcuts.push_back(deactivateEffectShortcut);
  deactivateEffectShortcut->setProperty("effectIndex", 0);
  QObject::connect(deactivateEffectShortcut, SIGNAL(activated()), this, SLOT(onSelectEffectShortcut()));

  // Space => activate/deactivate last effect
  QShortcut* toggleActiveEffectShortcut = new QShortcut(QKeySequence(Qt::Key_Space), parent);
  d->KeyboardShortcuts.push_back(toggleActiveEffectShortcut);
  toggleActiveEffectShortcut->setProperty("effectIndex", -1);
  QObject::connect(toggleActiveEffectShortcut, SIGNAL(activated()), this, SLOT(onSelectEffectShortcut()));

  // z, y => undo, redo
  QShortcut* undoShortcut = new QShortcut(QKeySequence(Qt::Key_Z), parent);
  d->KeyboardShortcuts.push_back(undoShortcut);
  QObject::connect(undoShortcut, SIGNAL(activated()), this, SLOT(undo()));
  QShortcut* undoShortcut2 = new QShortcut(QKeySequence::Undo, parent);
  d->KeyboardShortcuts.push_back(undoShortcut2);
  QObject::connect(undoShortcut2, SIGNAL(activated()), this, SLOT(undo()));

  QShortcut* redoShortcut = new QShortcut(QKeySequence(Qt::Key_Y), parent);
  d->KeyboardShortcuts.push_back(redoShortcut);
  QObject::connect(redoShortcut, SIGNAL(activated()), this, SLOT(redo()));
  QShortcut* redoShortcut2 = new QShortcut(QKeySequence::Redo, parent);
  d->KeyboardShortcuts.push_back(redoShortcut2);
  QObject::connect(redoShortcut2, SIGNAL(activated()), this, SLOT(redo()));

  // Keys qw/*,.<> => select previous, next segment
  Qt::Key prevNexSegmentKeys[] =
    {
    Qt::Key_Q, Qt::Key_W, // near effect selector numbers on a regular keyboard
    Qt::Key_Slash, Qt::Key_Asterisk, // available on the numpad
    Qt::Key_Comma, Qt::Key_Period, // commonly used in other applications
    Qt::Key_Greater, Qt::Key_Less, // commonly used in other applications
    Qt::Key_unknown // add shortcuts above, this must be the last line
    };
  for (int keyIndex = 0; prevNexSegmentKeys[keyIndex] != Qt::Key_unknown; keyIndex++)
    {
    QShortcut* prevShortcut = new QShortcut(QKeySequence(prevNexSegmentKeys[keyIndex]), parent);
    d->KeyboardShortcuts.push_back(prevShortcut);
    prevShortcut->setProperty("segmentIndexOffset", -1);
    QObject::connect(prevShortcut, SIGNAL(activated()), this, SLOT(onSelectSegmentShortcut()));
    keyIndex++;
    QShortcut* nextShortcut = new QShortcut(QKeySequence(prevNexSegmentKeys[keyIndex]), parent);
    d->KeyboardShortcuts.push_back(nextShortcut);
    nextShortcut->setProperty("segmentIndexOffset", +1);
    QObject::connect(nextShortcut, SIGNAL(activated()), this, SLOT(onSelectSegmentShortcut()));
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::uninstallKeyboardShortcuts()
{
  Q_D(qMRMLSegmentEditorWidget);
  foreach(QShortcut* shortcut, d->KeyboardShortcuts)
    {
    shortcut->disconnect(SIGNAL(activated()));
    shortcut->setParent(NULL);
    delete shortcut;
    }
  d->KeyboardShortcuts.clear();
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSelectEffectShortcut()
{
  Q_D(qMRMLSegmentEditorWidget);
  QShortcut* shortcut = qobject_cast<QShortcut*>(sender());
  if (shortcut == NULL || d->Locked)
    {
    return;
    }
  qSlicerSegmentEditorAbstractEffect* activeEffect = this->activeEffect();
  int selectedEffectIndex = shortcut->property("effectIndex").toInt();
  if (selectedEffectIndex >= 0)
    {
    qSlicerSegmentEditorAbstractEffect* selectedEffect = this->effectByIndex(selectedEffectIndex);
    if (selectedEffect == activeEffect)
      {
      // effect is already active => deactivate it
      selectedEffect = NULL;
      }
    this->setActiveEffect(selectedEffect);
    }
  else
    {
    this->setActiveEffect(d->LastActiveEffect);
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::onSelectSegmentShortcut()
{
  QShortcut* shortcut = qobject_cast<QShortcut*>(sender());
  if (shortcut == NULL)
    {
    return;
    }
  int segmentIndexOffset = shortcut->property("segmentIndexOffset").toInt();

  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(this->segmentationNode());
  if (segmentationNode == NULL || segmentationNode->GetDisplayNode() == NULL)
    {
    return;
    }
  vtkMRMLSegmentationDisplayNode* displayNode = vtkMRMLSegmentationDisplayNode::SafeDownCast(segmentationNode->GetDisplayNode());
  if (displayNode == NULL)
    {
    return;
    }
  std::vector<std::string> segmentIDs;
  displayNode->GetVisibleSegmentIDs(segmentIDs);
  QString currentSegmentID = this->currentSegmentID();
  for (unsigned int segmentIndex = 0; segmentIndex < segmentIDs.size(); segmentIndex++)
    {
    std::string segmentID = segmentIDs[segmentIndex];
    if (currentSegmentID == segmentID.c_str())
      {
      // this is the current segment, determine which one is the previous/next and select that
      int newSegmentIndex = (segmentIndex + segmentIndexOffset) % segmentIDs.size(); // wrap around
      this->setCurrentSegmentID(segmentIDs[newSegmentIndex].c_str());
      return;
      }
    }
}

//---------------------------------------------------------------------------
bool qMRMLSegmentEditorWidget::turnOffLightboxes()
{
  qSlicerLayoutManager* layoutManager = qSlicerApplication::application()->layoutManager();
  if (!layoutManager)
    {
    // application is closing
    return false;
    }
  vtkCollection* sliceLogics = layoutManager->mrmlSliceLogics();
  if (!sliceLogics)
    {
    return false;
    }

  bool lightboxFound = false;
  vtkObject* object = NULL;
  vtkCollectionSimpleIterator it;
  for (sliceLogics->InitTraversal(it); (object = sliceLogics->GetNextItemAsObject(it));)
    {
    vtkMRMLSliceLogic* sliceLogic = vtkMRMLSliceLogic::SafeDownCast(object);
    if (!sliceLogic)
      {
      continue;
      }
    vtkMRMLSliceNode* sliceNode = sliceLogic->GetSliceNode();
    if (!sliceNode)
      {
      continue;
      }
    if (sliceNode->GetLayoutGridRows() != 1 || sliceNode->GetLayoutGridColumns() != 1)
      {
      lightboxFound = true;
      sliceNode->SetLayoutGrid(1, 1);
      }
    }

  return lightboxFound;
}

//---------------------------------------------------------------------------
Qt::ToolButtonStyle qMRMLSegmentEditorWidget::effectButtonStyle() const
{
  Q_D(const qMRMLSegmentEditorWidget);
  return d->EffectButtonStyle;
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::setEffectButtonStyle(Qt::ToolButtonStyle toolButtonStyle)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (d->EffectButtonStyle == toolButtonStyle)
    {
    return;
    }
  d->EffectButtonStyle = toolButtonStyle;
  QList<QAbstractButton*> effectButtons = d->EffectButtonGroup.buttons();
  foreach(QAbstractButton* button, effectButtons)
    {
    QToolButton* toolButton = dynamic_cast<QToolButton*>(button);
    toolButton->setToolButtonStyle(d->EffectButtonStyle);
    }
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::segmentationNodeSelectorAddAttribute(const QString& nodeType,
  const QString& attributeName, const QVariant& attributeValue/*=QVariant()*/)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->SegmentationNodeComboBox->addAttribute(nodeType, attributeName, attributeValue);
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::segmentationNodeSelectorRemoveAttribute(const QString& nodeType,
  const QString& attributeName)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->SegmentationNodeComboBox->removeAttribute(nodeType, attributeName);
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::masterVolumeNodeSelectorAddAttribute(const QString& nodeType,
  const QString& attributeName, const QVariant& attributeValue/*=QVariant()*/)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MasterVolumeNodeComboBox->addAttribute(nodeType, attributeName, attributeValue);
}

//---------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::masterVolumeNodeSelectorRemoveAttribute(const QString& nodeType,
  const QString& attributeName)
{
  Q_D(qMRMLSegmentEditorWidget);
  d->MasterVolumeNodeComboBox->removeAttribute(nodeType, attributeName);
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::anchorClicked(const QUrl &url)
{
  Q_D(qMRMLSegmentEditorWidget);
  if (url.path().isEmpty())
    {
    this->updateEffectLayouts();
    }
}

//-----------------------------------------------------------------------------
void qMRMLSegmentEditorWidget::updateEffectLayouts()
{
  Q_D(qMRMLSegmentEditorWidget);

  if (d->ActiveEffect)
    {
    d->EffectHelpBrowser->setMinimumHeight(d->EffectHelpBrowser->sizeHint().height());
    d->EffectHelpBrowser->layout()->update();
    d->ActiveEffect->optionsFrame()->setMinimumHeight(d->ActiveEffect->optionsFrame()->sizeHint().height());
    d->ActiveEffect->optionsLayout()->activate();
    }
  else
    {
    d->OptionsGroupBox->setMinimumHeight(d->OptionsGroupBox->sizeHint().height());
    d->OptionsGroupBox->layout()->activate();
    }

  this->setMinimumHeight(this->sizeHint().height());
}
