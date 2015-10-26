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

#ifndef __qSlicerSubjectHierarchyScriptedPlugin_h
#define __qSlicerSubjectHierarchyScriptedPlugin_h

// SubjectHierarchy includes
#include "qSlicerSubjectHierarchyAbstractPlugin.h"

#include "qSlicerSubjectHierarchyModuleWidgetsExport.h"

// Forward Declare PyObject*
#ifndef PyObject_HEAD
struct _object;
typedef _object PyObject;
#endif
class qSlicerSubjectHierarchyScriptedPluginPrivate;

class vtkObject;
class vtkMRMLNode;
class vtkMRMLSubjectHierarchyNode;
class QStandardItem;
class QAction;
class qSlicerAbstractModuleWidget;

/// \ingroup Slicer_QtModules_SubjectHierarchy_Widgets
///    In Widgets, not Plugins because the paths and libs need to be exported to extensions
/// \brief Scripted abstract plugin for handling Subject Hierarchy nodes
///
/// This class provides an interface to plugins implemented in python.
/// USAGE: Subclass AbstractScriptedSubjectHierarchyPlugin in SubjectHierarchyPlugins subfolder
///   of python scripted module, and register plugin by creating this class in module (e.g.
///   setup method of module widget) and setting python source to implemented plugin subclass.
///   Example can be found here: https://subversion.assembla.com/svn/slicerrt/trunk/VolumeClip/src
///
/// Note about confidence values (\sa canAddNodeToSubjectHierarchy \sa canReparentNodeInsideSubjectHierarchy \sa canOwnSubjectHierarchyNode):
/// The confidence value is a floating point number between 0.0 and 1.0. Meaning of some typical values:
/// 0.0 = The plugin cannot handle the node in question at all
/// 0.3 = It is likely that other plugins will be able to handle the node in question better (typical value for plugins for generic types, such as Volumes)
/// 0.5 = The plugin has equal chance to handle this node as others (an example can be color table node)
/// 0.7 = The plugin is likely be the only one that can handle the node in question, but there is a chance that other plugins can do that too
/// 1.0 = The node in question can only be handled by the plugin (by node type or identifier attribute)
///
class Q_SLICER_MODULE_SUBJECTHIERARCHY_WIDGETS_EXPORT qSlicerSubjectHierarchyScriptedPlugin
  : public qSlicerSubjectHierarchyAbstractPlugin
{
  Q_OBJECT

public:
  typedef qSlicerSubjectHierarchyAbstractPlugin Superclass;
  qSlicerSubjectHierarchyScriptedPlugin(QObject* parent = NULL);
  virtual ~qSlicerSubjectHierarchyScriptedPlugin();

  Q_INVOKABLE QString pythonSource()const;

  /// Set python source for the implemented plugin
  /// \param newPythonSource Python file path
  Q_INVOKABLE bool setPythonSource(const QString newPythonSource);

  /// Convenience method allowing to retrieve the associated scripted instance
  Q_INVOKABLE PyObject* self() const;

  /// Set the name property value.
  /// \sa name
  virtual void setName(QString name);

// Role-related virtual methods
// If the subclass plugin does not offer a role, these do not need to be overridden
public:
  /// Determines if the actual plugin can handle a subject hierarchy node. The plugin with
  /// the highest confidence number will "own" the node in the subject hierarchy (set icon, tooltip,
  /// set context menu etc.)
  /// \param node Note to handle in the subject hierarchy tree
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canOwnSubjectHierarchyNode(vtkMRMLSubjectHierarchyNode* node)const;

  /// Get role that the plugin assigns to the subject hierarchy node.
  ///   Each plugin should provide only one role.
  virtual const QString roleForPlugin()const;

  /// Get help text for plugin to be added in subject hierarchy module widget help box
  virtual const QString helpText()const;

  /// Get icon of an owned subject hierarchy node
  /// \return Icon to set, NULL if nothing to set
  virtual QIcon icon(vtkMRMLSubjectHierarchyNode* node);

  /// Get visibility icon for a visibility state
  virtual QIcon visibilityIcon(int visible);

  /// Open module belonging to node and set inputs in opened module
  virtual void editProperties(vtkMRMLSubjectHierarchyNode* node);

  /// Generate displayed name for the owned subject hierarchy node corresponding to its role.
  /// The default implementation removes the '_SubjectHierarchy' ending from the node's name.
  virtual QString displayedNodeName(vtkMRMLSubjectHierarchyNode* node)const;

  /// Generate tooltip for a owned subject hierarchy node
  virtual QString tooltip(vtkMRMLSubjectHierarchyNode* node)const;

  /// Set display visibility of a owned subject hierarchy node
  virtual void setDisplayVisibility(vtkMRMLSubjectHierarchyNode* node, int visible);

  /// Get display visibility of a owned subject hierarchy node
  /// \return Display visibility (0: hidden, 1: shown, 2: partially shown)
  virtual int getDisplayVisibility(vtkMRMLSubjectHierarchyNode* node)const;

// Function related virtual methods
public:
  /// Get node context menu item actions to add to tree view
  virtual QList<QAction*> nodeContextMenuActions()const;

  /// Get scene context menu item actions to add to tree view
  /// Separate method is needed for the scene, as its actions are set to the
  /// tree by a different method \sa nodeContextMenuActions
  virtual QList<QAction*> sceneContextMenuActions()const;

  /// Show context menu actions valid for  given subject hierarchy node.
  /// \param node Subject Hierarchy node to show the context menu items for. If NULL, then shows menu items for the scene
  virtual void showContextMenuActionsForNode(vtkMRMLSubjectHierarchyNode* node);

// Parenting related virtual methods with default implementation
public:
  /// Determines if a non subject hierarchy node can be placed in the hierarchy using the actual plugin,
  /// and gets a confidence value for a certain MRML node (usually the type and possibly attributes are checked).
  /// Most plugins do not perform steps additional to the default, so the default implementation returns a 0
  /// confidence value, which can be overridden in plugins that do handle special cases.
  /// \param node Node to be added to the hierarchy
  /// \param parent Prospective parent of the node to add.
  ///   Default value is NULL. In that case the parent will be ignored, the confidence numbers are got based on the to-be child node alone.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canAddNodeToSubjectHierarchy(vtkMRMLNode* node , vtkMRMLSubjectHierarchyNode* parent=NULL)const;

  /// Determines if a subject hierarchy node can be reparented in the hierarchy using the actual plugin,
  /// and gets a confidence value for a certain MRML node (usually the type and possibly attributes are checked).
  /// Most plugins do not perform steps additional to the default, so the default implementation returns a 0
  /// confidence value, which can be overridden in plugins that do handle special cases.
  /// \param node Node to be reparented in the hierarchy
  /// \param parent Prospective parent of the node to reparent.
  /// \return Floating point confidence number between 0 and 1, where 0 means that the plugin cannot handle the
  ///   node, and 1 means that the plugin is the only one that can handle the node (by node type or identifier attribute)
  virtual double canReparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent)const;

  /// Reparent a node that was already in the subject hierarchy under a new parent.
  /// \return True if reparented successfully, false otherwise
  virtual bool reparentNodeInsideSubjectHierarchy(vtkMRMLSubjectHierarchyNode* node, vtkMRMLSubjectHierarchyNode* parent);

protected:
  QScopedPointer<qSlicerSubjectHierarchyScriptedPluginPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSubjectHierarchyScriptedPlugin);
  Q_DISABLE_COPY(qSlicerSubjectHierarchyScriptedPlugin);
};

#endif
