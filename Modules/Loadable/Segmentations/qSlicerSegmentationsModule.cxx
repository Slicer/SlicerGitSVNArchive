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

// Qt includes
#include <QtPlugin>
#include <QDebug>

// Slicer includes
#include "qSlicerIOManager.h"
#include "qSlicerNodeWriter.h"
#include "vtkMRMLThreeDViewDisplayableManagerFactory.h"
#include "vtkMRMLSliceViewDisplayableManagerFactory.h"

// Segmentations includes
#include "qSlicerSegmentationsModule.h"
#include "qSlicerSegmentationsModuleWidget.h"
#include "qSlicerSegmentationsReader.h"
#include "qSlicerSubjectHierarchySegmentationsPlugin.h"
#include "qSlicerSubjectHierarchySegmentsPlugin.h"
#include "vtkSlicerSegmentationsModuleLogic.h"
#include "vtkMRMLSegmentationsDisplayableManager3D.h"
#include "vtkMRMLSegmentationsDisplayableManager2D.h"
// Segment editor effects includes
#include "qSlicerSegmentEditorEffectFactory.h"
#include "qSlicerSegmentEditorPaintEffect.h"
#include "qSlicerSegmentEditorRectangleEffect.h"
#include "qSlicerSegmentEditorEraseEffect.h"

// Subject Hierarchy includes
#include "qSlicerSubjectHierarchyPluginHandler.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLSubjectHierarchyNode.h>

// PythonQt includes
#include "PythonQt.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerSegmentationsModule, qSlicerSegmentationsModule);

//-----------------------------------------------------------------------------
/// \ingroup SlicerRt_QtModules_Segmentations
class qSlicerSegmentationsModulePrivate
{
public:
  qSlicerSegmentationsModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSegmentationsModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSegmentationsModulePrivate::qSlicerSegmentationsModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSegmentationsModule methods

//-----------------------------------------------------------------------------
qSlicerSegmentationsModule::qSlicerSegmentationsModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSegmentationsModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSegmentationsModule::~qSlicerSegmentationsModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentationsModule::helpText()const
{
  QString help =
    "The Segmentations module manages segmentations. "
    "For more information see <a href=\"%1/Documentation/%2.%3/Modules/Segmentations\">%1/Documentation/%2.%3/Modules/Segmentations</a><br>";
  return help.arg(this->slicerWikiUrl()).arg(Slicer_VERSION_MAJOR).arg(Slicer_VERSION_MINOR);
}

//-----------------------------------------------------------------------------
QString qSlicerSegmentationsModule::acknowledgementText()const
{
  return "This work is part of SparKit project, funded by Cancer Care Ontario (CCO)'s ACRU program and Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO).";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentationsModule::categories() const
{
  return QStringList() << "";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentationsModule::contributors() const
{
  QStringList moduleContributors;
  moduleContributors << QString("Csaba Pinter (Queen's)");
  moduleContributors << QString("Adam Rankin (Robarts)");
  moduleContributors << QString("Andras Lasso (Queen's)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSegmentationsModule::icon()const
{
  return QIcon(":/Icons/Segmentations.png");
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModule::setMRMLScene(vtkMRMLScene* scene)
{
  // Connect scene node added event to make connections enabling per-segment subject hierarchy actions
  qvtkReconnect( this->mrmlScene(), scene, vtkMRMLScene::NodeAddedEvent, this, SLOT( onNodeAdded(vtkObject*,vtkObject*) ) );
    // Connect scene node removed event so that the per-segment subject hierarchy nodes are removed too
  qvtkReconnect( this->mrmlScene(), scene, vtkMRMLScene::NodeAboutToBeRemovedEvent, this, SLOT( onNodeAboutToBeRemoved(vtkObject*,vtkObject*) ) );

  Superclass::setMRMLScene(scene);
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModule::setup()
{
  this->Superclass::setup();

  vtkSlicerSegmentationsModuleLogic* segmentationsLogic = vtkSlicerSegmentationsModuleLogic::SafeDownCast(this->logic());

  // Register subject hierarchy plugins
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchySegmentationsPlugin());
  qSlicerSubjectHierarchyPluginHandler::instance()->registerPlugin(new qSlicerSubjectHierarchySegmentsPlugin());

  // Register IOs
  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  ioManager->registerIO(new qSlicerNodeWriter("Segmentation", QString("SegmentationFile"), QStringList() << "vtkMRMLSegmentationNode", true, this));
  ioManager->registerIO(new qSlicerSegmentationsReader(segmentationsLogic, this));

  // Use the displayable manager class to make sure the the containing library is loaded
  vtkSmartPointer<vtkMRMLSegmentationsDisplayableManager3D> dm3d = vtkSmartPointer<vtkMRMLSegmentationsDisplayableManager3D>::New();
  vtkSmartPointer<vtkMRMLSegmentationsDisplayableManager2D> dm2d = vtkSmartPointer<vtkMRMLSegmentationsDisplayableManager2D>::New();
  // Register displayable managers
  vtkMRMLThreeDViewDisplayableManagerFactory::GetInstance()->RegisterDisplayableManager("vtkMRMLSegmentationsDisplayableManager3D");
  vtkMRMLSliceViewDisplayableManagerFactory::GetInstance()->RegisterDisplayableManager("vtkMRMLSegmentationsDisplayableManager2D");

  // Register default segment editor effects
  // C++ effects
  qSlicerSegmentEditorEffectFactory::instance()->registerEffect(new qSlicerSegmentEditorPaintEffect());
  qSlicerSegmentEditorEffectFactory::instance()->registerEffect(new qSlicerSegmentEditorEraseEffect());
  qSlicerSegmentEditorEffectFactory::instance()->registerEffect(new qSlicerSegmentEditorRectangleEffect());
  // Python effects
  // (otherwise it would be the responsibility of the module that embeds the segment editor widget)
  PythonQt::init();
  PythonQtObjectPtr context = PythonQt::self()->getMainModule();
  context.evalScript(QString(
    "from SegmentEditorEffects import * \n"
    "import qSlicerSegmentationsEditorEffectsPythonQt as effects \n"
    "import traceback \n"
    "import logging \n"
    "try: \n"
    "  slicer.modules.segmenteditorscriptedeffectnames \n"
    "except AttributeError: \n"
    "  slicer.modules.segmenteditorscriptedeffectnames=[] \n"
    "for effectName in slicer.modules.segmenteditorscriptedeffectnames: \n"
    "  try: \n"
    "    exec(\"{0}Instance = effects.qSlicerSegmentEditorScriptedEffect(None);{0}Instance.setPythonSource({0}.__file__.replace('\\\\\\\\','/'));{0}Instance.self().register()\".format(effectName)) \n"
    "  except Exception as e: \n"
    "    logging.error(traceback.format_exc()) \n"
    ));
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerSegmentationsModule::createWidgetRepresentation()
{
  return new qSlicerSegmentationsModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSegmentationsModule::createLogic()
{
  return vtkSlicerSegmentationsModuleLogic::New();
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModule::onNodeAdded(vtkObject* sceneObject, vtkObject* nodeObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
    {
    return;
    }

  // Connect segment added and removed events to plugin to update subject hierarchy accordingly
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(nodeObject);
  if (segmentationNode)
    {
    // Get segmentations subject hierarchy plugin
    qSlicerSubjectHierarchySegmentationsPlugin* segmentationsPlugin = qobject_cast<qSlicerSubjectHierarchySegmentationsPlugin*>(
      qSlicerSubjectHierarchyPluginHandler::instance()->pluginByName("Segmentations") );

    qvtkConnect( segmentationNode, vtkSegmentation::SegmentAdded,
      segmentationsPlugin, SLOT( onSegmentAdded(vtkObject*,void*) ) );
    qvtkConnect( segmentationNode, vtkSegmentation::SegmentRemoved,
      segmentationsPlugin, SLOT( onSegmentRemoved(vtkObject*,void*) ) );
    qvtkConnect( segmentationNode, vtkSegmentation::SegmentModified,
      segmentationsPlugin, SLOT( onSegmentModified(vtkObject*,void*) ) );

    // Workaround for auto-select new segmentation node issue
    // (although the flag is on in the MRML node combobox, it does not select newly added nodes)
    qSlicerSegmentationsModuleWidget* moduleWidget = dynamic_cast<qSlicerSegmentationsModuleWidget*>(this->widgetRepresentation());
    if (moduleWidget)
      {
      moduleWidget->selectSegmentationNode(segmentationNode);
      }
    }

  // Connect subject hierarchy modified event to handle renaming segments
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nodeObject);
  if (subjectHierarchyNode)
    {
    qvtkConnect( subjectHierarchyNode, vtkCommand::ModifiedEvent,
      this, SLOT( onSubjectHierarchyNodeModified(vtkObject*) ) );
    }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModule::onNodeAboutToBeRemoved(vtkObject* sceneObject, vtkObject* nodeObject)
{
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  if (!scene)
    {
    return;
    }

  // Do nothing if scene is closing
  if (scene->IsClosing())
    {
    return;
    }

  // Remove segment subject hierarchy nodes if segmentation node is being removed
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(nodeObject);
  if (segmentationNode)
    {
    vtkMRMLSubjectHierarchyNode* segmentationSubjectHierarchyNode =
      vtkMRMLSubjectHierarchyNode::GetAssociatedSubjectHierarchyNode(segmentationNode);
    if (segmentationSubjectHierarchyNode)
      {
      std::vector<vtkMRMLHierarchyNode*> segmentSubjectHierarchyNodes = segmentationSubjectHierarchyNode->GetChildrenNodes();
      for (std::vector<vtkMRMLHierarchyNode*>::iterator childIt = segmentSubjectHierarchyNodes.begin(); childIt != segmentSubjectHierarchyNodes.end(); ++childIt)
        {
        vtkMRMLSubjectHierarchyNode* currentSegmentShNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(*childIt);
        scene->RemoveNode(currentSegmentShNode);
        }
      }
    }

  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nodeObject);
  if (subjectHierarchyNode)
    {
    // Remove segmentation node if its associated subject hierarchy node is being removed.
    // This is necessary so that the segment virtual nodes are removed before the segmentation SH node is removed,
    // otherwise the event observers will be removed, and the segment virtual nodes will be left in the scene.
    vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(
      subjectHierarchyNode->GetAssociatedNode());
    if (segmentationNode)
      {
      scene->RemoveNode(segmentationNode);
      }
    // Remove segment from parent segmentation if its subject hierarchy node is being removed
    else if (subjectHierarchyNode->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName()))
      {
      std::string segmentId = subjectHierarchyNode->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName());

      segmentationNode = vtkSlicerSegmentationsModuleLogic::GetSegmentationNodeForSegmentSubjectHierarchyNode(subjectHierarchyNode);
      if (segmentationNode)
        {
        // Segment might have been removed first, and subject hierarchy second, in which case we should not try to remove segment again
        if (segmentationNode->GetSegmentation()->GetSegment(segmentId))
          {
          segmentationNode->GetSegmentation()->RemoveSegment(segmentId);
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerSegmentationsModule::onSubjectHierarchyNodeModified(vtkObject* nodeObject)
{
  vtkMRMLSubjectHierarchyNode* subjectHierarchyNode = vtkMRMLSubjectHierarchyNode::SafeDownCast(nodeObject);
  if (subjectHierarchyNode && subjectHierarchyNode->GetAttribute(vtkMRMLSegmentationNode::GetSegmentIDAttributeName()))
    {
    // If segment name is different than subject hierarchy node name (without postfix) then rename segment
    vtkSegment* segment = vtkSlicerSegmentationsModuleLogic::GetSegmentForSegmentSubjectHierarchyNode(subjectHierarchyNode);
    if (segment && segment->GetName())
      {
      if (strcmp(segment->GetName(), subjectHierarchyNode->GetNameWithoutPostfix().c_str()))
        {
        segment->SetName(subjectHierarchyNode->GetNameWithoutPostfix().c_str());
        }
      }
    }
}

//-----------------------------------------------------------------------------
QStringList qSlicerSegmentationsModule::associatedNodeTypes() const
{
  return QStringList() << "vtkMRMLSegmentationNode";
}
