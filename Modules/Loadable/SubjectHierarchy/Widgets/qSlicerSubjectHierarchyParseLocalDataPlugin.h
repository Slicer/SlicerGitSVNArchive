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

#ifndef __qSlicerSubjectHierarchyParseLocalDataPlugin_h
#define __qSlicerSubjectHierarchyParseLocalDataPlugin_h

// SubjectHierarchy Plugins includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerSubjectHierarchyModuleWidgetsExport.h"

class qSlicerSubjectHierarchyParseLocalDataPluginPrivate;
class vtkMRMLNode;
class vtkMRMLSubjectHierarchyNode;

// Due to some reason the Python wrapping of this class fails, therefore
// put everything between BTX/ETX to exclude from wrapping.
// TODO investigate why the wrapping fails:
//   https://www.assembla.com/spaces/slicerrt/tickets/210-python-wrapping-error-when-starting-up-slicer-with-slicerrt
//BTX

/// \ingroup Slicer_QtModules_SubjectHierarchy_Widgets
class Q_SLICER_MODULE_SUBJECTHIERARCHY_WIDGETS_EXPORT qSlicerSubjectHierarchyParseLocalDataPlugin : public qSlicerSubjectHierarchyAbstractPlugin
{
public:
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyParseLocalDataPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyParseLocalDataPlugin();

public:
  /// Get scene context menu item actions to add to tree view
  /// Separate method is needed for the scene, as its actions are set to the
  /// tree by a different method \sa nodeContextMenuActions
  virtual QList<QAction*> sceneContextMenuActions()const;

  /// Show context menu actions valid for  given subject hierarchy node.
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node);

protected slots:
  /// Create subject hierarchy from loaded loacl directories.
  /// Adds all nodes in the potential subject hierarchy nodes list that is storable and has a valid storage node with a file
  /// name (meaning it has been loaded from local disk). Creates patient/study/series/subseries hierarchies according to the
  /// paths of the loaded files, ignoring the part that is identical (if everything has been loaded from the same directory,
  /// then only creates subject hierarchy nodes for the directories within that directory).
  void createHierarchyFromLoadedDirectoryStructure();

protected:
  QScopedPointer<qSlicerSubjectHierarchyParseLocalDataPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyParseLocalDataPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyParseLocalDataPlugin);
};

//ETX

#endif
