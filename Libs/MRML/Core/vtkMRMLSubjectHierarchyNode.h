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

// .NAME vtkMRMLSubjectHierarchyNode
// .SECTION Description
// MRML node to represent subject hierarchy tree
//

#ifndef __vtkMRMLSubjectHierarchyNode_h
#define __vtkMRMLSubjectHierarchyNode_h

// MRML includes
#include <vtkMRMLNode.h>

class vtkCallbackCommand;
class vtkMRMLTransformNode;

/// \ingroup Slicer_MRML_Core
/// \brief MRML node to represent a complete subject hierarchy tree
///
///   There can be only one such node in the scene, as subject hierarchy is to contain all the
///   supported data nodes in the scene, so that the user can navigate the data loaded throughout
///   the session.
///   The node entries are encapsulated in SubjectHierarchyItem classes, which contain the hierarchy
///   information for the contained nodes, and represent the non-leaf nodes of the tree.
///
class VTK_MRML_EXPORT vtkMRMLSubjectHierarchyNode : public vtkMRMLNode
{
public:
  /// Subject hierarchy item identifier
  typedef unsigned long SubjectHierarchyItemID;

  static const SubjectHierarchyItemID INVALID_ITEM_ID;

  // Separator characters for (de)serializing the UID map
  static const std::string SUBJECTHIERARCHY_UID_ITEM_SEPARATOR;
  static const std::string SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR;

  enum
  {
    SubjectHierarchyItemAddedEvent = 62000,
    SubjectHierarchyItemAboutToBeRemovedEvent,
    SubjectHierarchyItemRemovedEvent,
    SubjectHierarchyItemModifiedEvent,
    /// Event fired when UID is added to subject hierarchy item. Useful when using UIDs
    /// to find related nodes, and the nodes are loaded sequentially in unspecified order.
    SubjectHierarchyItemUIDAddedEvent
  };

public:
  static vtkMRMLSubjectHierarchyNode *New();
  vtkTypeMacro(vtkMRMLSubjectHierarchyNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// Read node attributes from XML file
  virtual void ReadXMLAttributes(const char** atts);

  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// Get node XML tag name (like Volume, Contour)
  virtual const char* GetNodeTagName();

// Get/Set methods
public:
  /// Get ID of root subject hierarchy item
  SubjectHierarchyItemID GetRootItemID();
  /// Get data node for a subject hierarchy item
  vtkMRMLNode* GetItemDataNode(SubjectHierarchyItemID itemID);
  /// Set name for a subject hierarchy item
  void SetItemName(SubjectHierarchyItemID itemID, std::string name);
  /// Get name for a subject hierarchy item
  /// \return Name of the associated data node if any, otherwise the name of the item
  std::string GetItemName(SubjectHierarchyItemID itemID);
  /// Set level for a subject hierarchy item
  void SetItemLevel(SubjectHierarchyItemID itemID, std::string level);
  /// Get level for a subject hierarchy item
  std::string GetItemLevel(SubjectHierarchyItemID itemID);
  /// Set owner plugin name (role) for a subject hierarchy item
  void SetItemOwnerPluginName(SubjectHierarchyItemID itemID, std::string owherPluginName);
  /// Get owner plugin name (role) for a subject hierarchy item
  std::string GetItemOwnerPluginName(SubjectHierarchyItemID itemID);
  /// Set owner plugin auto search flag for a subject hierarchy item
  void SetItemOwnerPluginAutoSearch(SubjectHierarchyItemID itemID, bool owherPluginAutoSearch);
  /// Get owner plugin auto search flag for a subject hierarchy item
  bool GetItemOwnerPluginAutoSearch(SubjectHierarchyItemID itemID);

  /// Get ID of the parent of a subject hierarchy item
  /// \return Parent item ID, INVALID_ITEM_ID if there is no parent
  SubjectHierarchyItemID GetItemParent(SubjectHierarchyItemID itemID);
  /// Get IDs of the children of a subject hierarchy item
  /// \param recursive If false then collect direct children, if true then the whole branch. False by default
  void GetItemChildren(SubjectHierarchyItemID itemID, std::vector<SubjectHierarchyItemID>& childIDs, bool recursive=false);
  /// Set new parent to a subject hierarchy item
  /// \return Success flag
  bool ReparentItem(SubjectHierarchyItemID itemID, vtkMRMLNode* newParentNode);
  /// Move item within the same branch before given item
  /// \param beforeItemID Item to move given item before. If INVALID_ITEM_ID then insert to the end
  /// \return Success flag
  bool MoveItem(SubjectHierarchyItemID itemID, SubjectHierarchyItemID beforeItemID);
  /// Get position of item under its parent
  /// \return Position of item under its parent. -1 on failure.
  int GetItemPositionUnderParent(SubjectHierarchyItemID itemID);

  /// Set UID to the subject hierarchy item
  void SetItemUID(SubjectHierarchyItemID itemID, std::string uidName, std::string uidValue);
  /// Get a UID with a given name
  /// \return The UID value if exists, empty string if does not
  std::string GetItemUID(SubjectHierarchyItemID itemID, std::string uidName);

  /// Add attribute to the subject hierarchy item
  void SetItemAttribute(SubjectHierarchyItemID itemID, std::string attributeName, std::string attributeValue);
  /// Get an attribute with a given name
  /// \return The attribute value if exists, empty string if does not
  std::string GetItemAttribute(SubjectHierarchyItemID itemID, std::string attributeName);

// Item finder methods
public:
  /// Find subject hierarchy item according to a UID (by exact match)
  /// \param uidName UID string to lookup
  /// \param uidValue UID string that needs to _exactly match_ the UID string of the subject hierarchy item
  /// \sa GetUID()
  SubjectHierarchyItemID GetSubjectHierarchyItemByUID(const char* uidName, const char* uidValue);

  /// Find subject hierarchy item according to a UID (by containing). For example find UID in instance UID list
  /// \param uidName UID string to lookup
  /// \param uidValue UID string that needs to be _contained_ in the UID string of the subject hierarchy item
  /// \return First match
  /// \sa GetUID()
  SubjectHierarchyItemID GetSubjectHierarchyItemByUIDList(const char* uidName, const char* uidValue);

  /// Get subject hierarchy item associated to a data MRML node
  /// \param dataNode The node for which we want the associated hierarchy node
  /// \return The first subject hierarchy item ID to which the given node is associated to.
  SubjectHierarchyItemID GetSubjectHierarchyItemByDataNode(vtkMRMLNode* dataNode);

  /// Create subject hierarchy item in the hierarchy under a specified parent. If the item existed then use that and
  /// set it up with the supplied parameters. Used mostly for creating hierarchy items (folder, patient, study, etc.)
  /// \param parentItemID Parent item under which the created item is inserted. If top-level then use \sa GetRootItemID
  /// \param dataNode Associated data MRML node
  /// \param level Level string of the created item (\sa vtkMRMLSubjectHierarchyConstants)
  /// \param name Name of the item. Only used if there is no data node associated (in which case the name of that MRML node is used)
  /// \return ID of the item in the hierarchy that was assigned automatically when adding
  SubjectHierarchyItemID CreateSubjectHierarchyItem( SubjectHierarchyItemID parentItemID,
                                                     vtkMRMLNode* dataNode=NULL,
                                                     std::string level=vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelFolder(),
                                                     std::string name="" );

  /// Get child subject hierarchy item with specific name
  /// \param parent Parent subject hierarchy item to start from
  /// \param name Name to find
  /// \return Child node whose name without postfix is the same as the given attribute
  SubjectHierarchyItemID GetChildWithName(SubjectHierarchyItemID parentItemID, std::string name);

  /// Find all associated data nodes of a specified class in a branch of the hierarchy.
  /// Re-implemented to handle nested associations \sa GetAssociatedNode
  /// \param dataNodeCollection Collection updated with the list of data nodes
  /// \param childClass Name of the class we are looking for. NULL returns associated data nodes of any kind
  void GetDataNodesInBranch(SubjectHierarchyItemID itemID, vtkCollection* dataNodeCollection, const char* childClass=NULL);

// Utility functions
public:
  /// Set subject hierarchy branch visibility
  void SetDisplayVisibilityForBranch(SubjectHierarchyItemID itemID, int visible);

  /// Get subject hierarchy branch visibility
  /// \return Visibility value (0:Hidden, 1:Visible, 2:PartiallyVisible)
  int GetDisplayVisibilityForBranch(SubjectHierarchyItemID itemID);

  /// Determine if an item is of a certain level
  /// \param level Level to check
  /// \return True if the item is of the specified level, false otherwise
  bool IsItemLevel(SubjectHierarchyItemID itemID, std::string level);

  /// Get attribute value for a item from an upper level in the subject hierarchy
  /// \param attributeName Name of the requested attribute
  /// \param level Level of the ancestor item we look for the attribute in
  ///   (e.g. value of vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelStudy()).
  ///   If empty, then look all the way up to the subject and return first attribute found with specified name
  /// \return Attribute value from the lowest level ancestor where the attribute can be found
  std::string GetAttributeFromItemAncestor(SubjectHierarchyItemID itemID, std::string attributeName, std::string level="");

  /// Get ancestor subject hierarchy item at a certain level
  /// \param level Level of the ancestor item we start searching.
  SubjectHierarchyItemID GetItemAncestorAtLevel(SubjectHierarchyItemID itemID, std::string level);

  /// Determine if any of the children of this item is transformed (has a parent transform applied), except for an optionally given node
  /// \param exceptionNode The function still returns true if the only applied transform found is this specified node
  bool IsAnyNodeInBranchTransformed(SubjectHierarchyItemID itemID, vtkMRMLTransformNode* exceptionNode=NULL);

  /// Deserialize a UID list string (stored in the UID map) into a vector of UID strings
  static void DeserializeUIDList(std::string uidListString, std::vector<std::string>& deserializedUIDList);

  /// Get subject hierarchy items that are referenced from a given item by DICOM.
  /// Finds the series items that contain the SOP instance UIDs that are listed in
  /// the attribute of this item containing the referenced SOP instance UIDs
  /// \sa vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName()
  std::vector<SubjectHierarchyItemID> GetSubjectHierarchyItemsReferencedFromItemByDICOM(SubjectHierarchyItemID itemID);

protected:
  /// Callback function for all events from the subject hierarchy items
  static void ItemEventCallback(vtkObject* caller, unsigned long eid, void* clientData, void* callData);

protected:
  vtkMRMLSubjectHierarchyNode();
  ~vtkMRMLSubjectHierarchyNode();
  vtkMRMLSubjectHierarchyNode(const vtkMRMLSubjectHierarchyNode&);
  void operator=(const vtkMRMLSubjectHierarchyNode&);

  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal;

  /// Command handling events from subject hierarchy items
  vtkSmartPointer<vtkCallbackCommand> ItemEventCallbackCommand;
};

#endif
