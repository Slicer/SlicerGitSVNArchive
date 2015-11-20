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

// .NAME vtkSlicerSubjectHierarchyModuleLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerSubjectHierarchyModuleLogic_h
#define __vtkSlicerSubjectHierarchyModuleLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

#include "vtkSlicerSubjectHierarchyModuleLogicExport.h"

class vtkMRMLSubjectHierarchyNode;
class vtkMRMLTransformNode;

/// \ingroup Slicer_QtModules_SubjectHierarchy
class VTK_SLICER_SUBJECTHIERARCHY_LOGIC_EXPORT vtkSlicerSubjectHierarchyModuleLogic :
  public vtkSlicerModuleLogic
{
public:
  static vtkSlicerSubjectHierarchyModuleLogic *New();
  vtkTypeMacro(vtkSlicerSubjectHierarchyModuleLogic,vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

public:
  /// Place series in subject hierarchy. Create subject and study node if needed
  /// \return Series subject hierarchy node of the inserted series
  static vtkMRMLSubjectHierarchyNode* InsertDicomSeriesInHierarchy(
    vtkMRMLScene* scene, const char* subjectId, const char* studyInstanceUID, const char* seriesInstanceUID );

  /// Determine if two subject hierarchy nodes are in the same branch (share the same parent)
  /// \param node1 First node to check. Can be subject hierarchy node or a node associated with one
  /// \param node2 Second node to check
  /// \param lowestCommonLevel Lowest level on which they have to share an ancestor
  /// \return The common parent if the two nodes or their associated hierarchy nodes share a parent
  ///   on the specified level, NULL otherwise
  static vtkMRMLSubjectHierarchyNode* AreNodesInSameBranch(
    vtkMRMLNode* node1, vtkMRMLNode* node2, const char* lowestCommonLevel );

  /// Determine if a tag name is a patient tag (not attribute, but tag - without prefix!)
  static bool IsPatientTag(std::string tagName);

  /// Determine if a tag name is a study tag (not attribute, but tag - without prefix!)
  static bool IsStudyTag(std::string tagName);

  /// Apply transform node as parent transform on subject hierarchy node and on all children, recursively
  /// \param node Subject hierarchy node defining branch to apply transform on
  /// \param transformNode Transform node to apply. If NULL, then any existing transform is removed
  /// \param hardenExistingTransforms Mode of handling already transformed nodes. If true (default), then the occurrent parent transforms
  ///   of target nodes are hardened before applying the specified transform. If false, then the already applied parent transforms are
  ///   transformed with the specified transform (Note: this latter approach may result in unwanted transformations of other nodes)
  static void TransformBranch(vtkMRMLSubjectHierarchyNode* node, vtkMRMLTransformNode* transformNode, bool hardenExistingTransforms=true);

  /// Harden transform on subject hierarchy node and on all children, recursively
  static void HardenTransformOnBranch(vtkMRMLSubjectHierarchyNode* node);

protected:
  /// Called each time a new scene is set
  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Called every time the scene has been significantly changed.
  virtual void UpdateFromMRMLScene();

  /// Register custom node type
  virtual void RegisterNodes();

protected:
  vtkSlicerSubjectHierarchyModuleLogic();
  virtual ~vtkSlicerSubjectHierarchyModuleLogic();

private:
  vtkSlicerSubjectHierarchyModuleLogic(const vtkSlicerSubjectHierarchyModuleLogic&); // Not implemented
  void operator=(const vtkSlicerSubjectHierarchyModuleLogic&);               // Not implemented
};

#endif
