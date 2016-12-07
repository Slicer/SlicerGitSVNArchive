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

// Subject Hierarchy includes
#include "vtkMRMLSubjectHierarchyNode.h"
#include "vtkMRMLSubjectHierarchyConstants.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLDisplayableNode.h"
#include "vtkMRMLDisplayNode.h"
#include "vtkMRMLTransformNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

// STD includes
#include <sstream>
#include <set>
#include <map>
#include <algorithm>

//----------------------------------------------------------------------------
const vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID = VTK_UNSIGNED_LONG_MAX;
const std::string vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR = std::string(":");
const std::string vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR = std::string("; ");

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSubjectHierarchyNode);

//----------------------------------------------------------------------------
class vtkSubjectHierarchyItem : public vtkObject
{
public:
  static vtkSubjectHierarchyItem *New();
  vtkTypeMacro(vtkSubjectHierarchyItem, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  //TODO: Read/WriteXML

  typedef std::vector<vtkSmartPointer<vtkSubjectHierarchyItem> > ChildVector;

public:
  /// Incremental unique identifier of the subject hierarchy item.
  /// This number is used to reference to an item from outside the MRML node
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID ID;

  /// Pointer to the data node associated to this subject hierarchy node
  vtkWeakPointer<vtkMRMLNode> DataNode;
  /// Name of the item (only used if there is no data node, for example subject, study, folder)
  std::string Name;

  /// Parent
  vtkSubjectHierarchyItem* Parent;
  /// Ordered list of children
  std::vector<vtkSmartPointer<vtkSubjectHierarchyItem> > Children;

  /// Level identifier (default levels are Subject and Study)
  std::string Level;

  /// Name of the owner plugin that claimed this node
  std::string OwnerPluginName;

  /// Flag indicating whether a plugin automatic search needs to be performed when the node is modified
  /// By default it is true. It is usually only set to false when the user has manually overridden the
  /// automatic choice. In that case the manual selection is not automatically overridden.
  bool OwnerPluginAutoSearch;

  /// List of UIDs of this subject hierarchy node
  /// UIDs can be DICOM UIDs, MIDAS urls, etc.
  std::map<std::string, std::string> UIDs;

  //TODO: See later if can be removed (and store attributes in the data nodes)
  /// Attributes (metadata, referenced IDs, etc.)
  std::map<std::string, std::string> Attributes;

// Get/set functions
public:
  /// Add item to tree under parent, specifying basic properties
  /// \param parent Parent item pointer under which this item is inserted
  /// \param dataNode Associated data MRML node
  /// \param level Level string of the item (\sa vtkMRMLSubjectHierarchyConstants)
  /// \param name Name of the item. Only used if there is no data node associated
  ///   (in which case the name of that MRML node is used)
  /// \return ID of the item in the hierarchy that was assigned automatically when adding
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID AddItemToTree(
    vtkSubjectHierarchyItem* parent,
    vtkMRMLNode* dataNode=NULL,
    std::string level=vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelFolder(),
    std::string name="" );

  /// Get name of the item. If has data node associated then return name of data node, \sa Name member otherwise
  std::string GetName();

  /// Set UID to the item
  void SetUID(std::string uidName, std::string uidValue);
  /// Get a UID with a given name
  /// \return The UID value if exists, empty string if does not
  std::string GetUID(std::string uidName);
  /// Set attribute to item
  /// \parameter attributeValue Value of attribute. If empty string, then attribute is removed
  void SetAttribute(std::string attributeName, std::string attributeValue);
  /// Get an attribute with a given name
  /// \return The attribute value if exists, empty string if does not
  std::string GetAttribute(std::string attributeName);

// Child related functions
public:
  /// Determine whether this item has any children
  bool HasChildren();
  /// Find child by ID
  /// \param itemID ID to find
  /// \param recursive Flag whether to find only direct children (false) or in the whole branch (true). True by default
  /// \return Item if found, NULL otherwise
  vtkSubjectHierarchyItem* FindChildByID(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID, bool recursive=true);
  /// Find child by associated data MRML node
  /// \param dataNode Data MRML node to find
  /// \param recursive Flag whether to find only direct children (false) or in the whole branch (true). True by default
  /// \return Item if found, NULL otherwise
  vtkSubjectHierarchyItem* FindChildByDataNode(vtkMRMLNode* dataNode, bool recursive=true);
  /// Find child by UID (exact match)
  /// \param recursive Flag whether to find only direct children (false) or in the whole branch (true). True by default
  /// \return Item if found, NULL otherwise
  vtkSubjectHierarchyItem* FindChildByUID(std::string uidName, std::string uidValue, bool recursive=true);
  /// Find child by UID list (containing). For example find UID in instance UID list
  /// \param recursive Flag whether to find only direct children (false) or in the whole branch (true). True by default
  /// \return Item if found, NULL otherwise
  vtkSubjectHierarchyItem* FindChildByUIDList(std::string uidName, std::string uidValue, bool recursive=true);
  /// Find children by name
  /// \param name Name (or part of a name) to find
  /// \param foundItemIDs List of found item IDs. Needs to be empty when passing as argument!
  /// \param contains Flag whether string containment is enough to determine match. True means a substring is searched
  ///   (case insensitive), false means that the name needs to match exactly (case sensitive)
  /// \param recursive Flag whether to find only direct children (false) or in the whole branch (true). True by default
  /// \return Item if found, NULL otherwise
  void FindChildIDsByName( std::string name, std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> &foundItemIDs,
                           bool contains=false, bool recursive=true );
  /// Get data nodes (of a certain type) associated to items in the branch of this item
  void GetDataNodesInBranch(vtkCollection *children, const char* childClass=NULL);
  /// Get all children in the branch recursively
  void GetAllChildrenIDs(std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> &childIDs);
  /// Get list of IDs of all direct children of this item
  void GetDirectChildrenIDs(std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> &childIDs);

  /// Insert item as child before given item
  /// \param addedItem Item to add
  /// \param beforeItem Item to insert addedItem before. If NULL then insert to the end
  /// \return Success flag
  bool InsertChild(vtkSmartPointer<vtkSubjectHierarchyItem> addedItem, vtkSubjectHierarchyItem* beforeItem=NULL);
  /// Insert item as child before given item
  /// \param addedItem Item to add
  /// \param beforeItem ID of item to insert addedItem before. If INVALID_ITEM_ID then insert to the end
  /// \return Success flag
  bool InsertChild(vtkSmartPointer<vtkSubjectHierarchyItem> addedItem,
    vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID beforeItemID=vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID);

  //TODO:
  //bool MoveChild(vtkSubjectHierarchyItem* item, vtkSubjectHierarchyItem* beforeItem);
  //bool MoveChild(vtkSubjectHierarchyItem* item, vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID beforeItem);

  /// Remove given item from children by item pointer
  /// \return Success flag
  bool RemoveChild(vtkSubjectHierarchyItem* item);
  /// Remove given item from children by ID
  /// \return Success flag
  bool RemoveChild(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID);
  /// Remove all children
  void RemoveAllChildren();

// Utility functions
public:
  /// Get attribute value from an upper level in the subject hierarchy
  /// \param attributeName Name of the requested attribute
  /// \param level Level of the ancestor item we look for the attribute in
  ///   (e.g. value of vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelStudy()).
  ///   If empty, then look all the way up to the subject and return first attribute found with specified name
  /// \return Attribute value from the lowest level ancestor where the attribute can be found
  std::string GetAttributeFromAncestor(std::string attributeName, std::string level);
  /// Get ancestor subject hierarchy item at a certain level
  /// \param level Level of the ancestor node we start searching.
  vtkSubjectHierarchyItem* GetAncestorAtLevel(std::string level);

public:
  vtkSubjectHierarchyItem();
  ~vtkSubjectHierarchyItem();

private:
  /// Incremental ID used to uniquely identify subject hierarchy items
  static vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID NextSubjectHierarchyItemID;

  vtkSubjectHierarchyItem(const vtkSubjectHierarchyItem&); // Not implemented
  void operator=(const vtkSubjectHierarchyItem&);          // Not implemented
};

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSubjectHierarchyItem);

vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkSubjectHierarchyItem::NextSubjectHierarchyItemID = 0;

//---------------------------------------------------------------------------
// vtkSubjectHierarchyItem methods

//---------------------------------------------------------------------------
vtkSubjectHierarchyItem::vtkSubjectHierarchyItem()
  : ID(vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
  , DataNode(NULL)
  , Name("")
  , Level(vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelFolder())
  , Parent(NULL)
  , OwnerPluginName("")
  , OwnerPluginAutoSearch(true)
{
  this->Children.clear();
  this->Attributes.clear();
  this->UIDs.clear();
}

//---------------------------------------------------------------------------
vtkSubjectHierarchyItem::~vtkSubjectHierarchyItem()
{
  this->RemoveAllChildren();

  this->Attributes.clear();
  this->UIDs.clear();
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkSubjectHierarchyItem::AddItemToTree(
  vtkSubjectHierarchyItem* parent, vtkMRMLNode* dataNode/*=NULL*/, std::string level/*="Folder"*/, std::string name/*=""*/ )
{
  this->ID = vtkSubjectHierarchyItem::NextSubjectHierarchyItemID;
  vtkSubjectHierarchyItem::NextSubjectHierarchyItemID++;
  if (vtkSubjectHierarchyItem::NextSubjectHierarchyItemID == VTK_UNSIGNED_LONG_MAX)
    {
    // There is a negligible chance that it reaches maximum, report error in that case
    vtkErrorMacro("AddItemToTree: Next subject hierarchy item ID reached its maximum value! Item is not added to the tree");
    return;
    }

  this->DataNode = dataNode;
  this->Name = name;
  this->Level = level;

  this->Parent = parent;

  return this->ID;
}

//----------------------------------------------------------------------------
void vtkSubjectHierarchyItem::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  //TODO:
  //os << indent << " Level=\""
  //  << (this->Level ? this->Level : "NULL" ) << "\n";

  //os << indent << " OwnerPluginName=\""
  //  << (this->OwnerPluginName ? this->OwnerPluginName : "NULL" ) << "\n";

  //os << indent << " OwnerPluginAutoSearch=\""
  //  << (this->OwnerPluginAutoSearch ? "true" : "false") << "\n";

  //os << indent << " UIDs=\"";
  //for (std::map<std::string, std::string>::iterator uidsIt = this->UIDs.begin(); uidsIt != this->UIDs.end(); ++uidsIt)
  //  {
  //  os << uidsIt->first << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR.c_str()
  //    << uidsIt->second << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR.c_str();
  //  }
  //os << "\"";
}

//---------------------------------------------------------------------------
std::string vtkSubjectHierarchyItem::GetName()
{
  if (this->DataNode.GetPointer() && this->DataNode->GetName())
    {
    return std::string(this->DataNode->GetName());
    }
  return this->Name;
}

//---------------------------------------------------------------------------
bool vtkSubjectHierarchyItem::HasChildren()
{
  return !this->Children.empty();
}

//---------------------------------------------------------------------------
vtkSubjectHierarchyItem* vtkSubjectHierarchyItem::FindChildByID(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID, bool recursive/*=true*/)
{
  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    vtkSubjectHierarchyItem* currentItem = childIt->GetPointer();
    if (itemID == currentItem->ID)
      {
      return currentItem;
      }
    if (recursive)
      {
      vtkSubjectHierarchyItem* foundItemInBranch = currentItem->FindChildByID(itemID);
      if (foundItemInBranch)
        {
        return foundItemInBranch;
        }
      }
    }
  return NULL;
}

//---------------------------------------------------------------------------
vtkSubjectHierarchyItem* vtkSubjectHierarchyItem::FindChildByDataNode(vtkMRMLNode* dataNode, bool recursive/*=true*/)
{
  if (!dataNode)
    {
    return NULL;
    }

  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    vtkSubjectHierarchyItem* currentItem = childIt->GetPointer();
    if (dataNode == currentItem->DataNode.GetPointer())
      {
      return currentItem;
      }
    if (recursive)
      {
      vtkSubjectHierarchyItem* foundItemInBranch = currentItem->FindChildByDataNode(dataNode);
      if (foundItemInBranch)
        {
        return foundItemInBranch;
        }
      }
    }
  return NULL;
}

//---------------------------------------------------------------------------
vtkSubjectHierarchyItem* vtkSubjectHierarchyItem::FindChildByUID(std::string uidName, std::string uidValue, bool recursive/*=true*/)
{
  if (uidName.empty() || uidValue.empty())
    {
    return NULL;
    }
  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    vtkSubjectHierarchyItem* currentItem = childIt->GetPointer();
    if (!currentItem->GetUID(uidName).compare(uidValue))
      {
      return currentItem;
      }
    if (recursive)
      {
      vtkSubjectHierarchyItem* foundItemInBranch = currentItem->FindChildByUID(uidName, uidValue);
      if (foundItemInBranch)
        {
        return foundItemInBranch;
        }
      }
    }
  return NULL;
}

//---------------------------------------------------------------------------
vtkSubjectHierarchyItem* vtkSubjectHierarchyItem::FindChildByUIDList(std::string uidName, std::string uidValue, bool recursive/*=true*/)
{
  if (uidName.empty() || uidValue.empty())
    {
    return NULL;
    }
  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    vtkSubjectHierarchyItem* currentItem = childIt->GetPointer();
    if (currentItem->GetUID(uidName).find(uidValue) != std::string::npos)
      {
      return currentItem;
      }
    if (recursive)
      {
      vtkSubjectHierarchyItem* foundItemInBranch = currentItem->FindChildByUID(uidName, uidValue);
      if (foundItemInBranch)
        {
        return foundItemInBranch;
        }
      }
    }
  return NULL;
}

//---------------------------------------------------------------------------
void vtkSubjectHierarchyItem::FindChildIDsByName(
  std::string name, std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> &foundItemIDs,
  bool contains=false, bool recursive/*=true*/)
{
  if (contains)
    {
    std::transform(name.begin(), name.end(), name.begin(), ::tolower); // Make it lowercase for case-insensitive comparison
    }

  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    vtkSubjectHierarchyItem* currentItem = childIt->GetPointer();
    std::string currentName = currentItem->GetName();
    if (contains)
      {
      std::transform(currentName.begin(), currentName.end(), currentName.begin(), ::tolower); // Make it lowercase for case-insensitive comparison
      if (currentName.find(name) != std::string::npos)
        {
        foundItemIDs.push_back(currentItem->ID);
        }
      }
    else if (!currentName.compare(name))
      {
      foundItemIDs.push_back(currentItem->ID);
      }
    if (recursive)
      {
      currentItem->FindChildIDsByName(name, foundItemIDs, contains);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSubjectHierarchyItem::GetDataNodesInBranch(vtkCollection* dataNodeCollection, const char* childClass/*=NULL*/)
{
  if (dataNodeCollection == NULL)
    {
    vtkErrorMacro("GetDataNodesInBranch: Output collection must be created before calling the method");
    return;
    }
  std::string nodeClass("vtkMRMLNode");
  if (childClass)
    {
    nodeClass = childClass;
    }

  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    vtkSubjectHierarchyItem* currentItem = childIt->GetPointer();
    vtkMRMLNode* currentDataNode = currentItem->DataNode.GetPointer();
    if (currentDataNode)
      {
      if (currentDataNode->IsA(nodeClass.c_str()))
        {
        dataNodeCollection->AddItem(currentDataNode);
        }
      }

    // Find associated data nodes recursively
    currentItem->GetDataNodesInBranch(dataNodeCollection, childClass);
    }
}

//---------------------------------------------------------------------------
void vtkSubjectHierarchyItem::GetAllChildrenIDs(std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> &childIDs)
{
  childIDs.clear();
  this->FindChildIDsByName("", childIDs, true);
}

//---------------------------------------------------------------------------
void vtkSubjectHierarchyItem::GetDirectChildrenIDs(std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> &childIDs)
{
  childIDs.clear();
  for (ChildVector::iterator childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    childIDs.push_back((*childIt)->ID);
    }
}

//---------------------------------------------------------------------------
bool vtkSubjectHierarchyItem::InsertChild(vtkSmartPointer<vtkSubjectHierarchyItem> addedItem, vtkSubjectHierarchyItem* beforeItem/*=NULL*/)
{
  if (!beforeItem)
    {
    this->Children.push_back(addedItem);
    return true;
    }

  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    if (beforeItem == childIt->GetPointer())
      {
      break;
      }
    }
  if (childIt == this->Children.end())
    {
    vtkErrorMacro("InsertChild: Failed to find subject hierarchy item '" << beforeItem->GetName()
      << "' as insertion position in item '" << this->GetName() << "'");
    return false;
    }

  this->Children.insert(childIt, addedItem);
  this->Modified();
  //TODO: Invoke insert custom event?
  return true;
}

//---------------------------------------------------------------------------
bool vtkSubjectHierarchyItem::InsertChild( vtkSmartPointer<vtkSubjectHierarchyItem> addedItem,
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID beforeItemID/*=vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID*/ )
{
  if (beforeItemID == vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID)
    {
    this->Children.push_back(addedItem);
    return true;
    }

  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    if (beforeItemID == (*childIt)->ID)
      {
      break;
      }
    }

  if (childIt == this->Children.end())
    {
    vtkErrorMacro("InsertChild: Failed to find subject hierarchy item with ID " << beforeItemID
      << " as insertion position in item '" << this->GetName() << "'");
    return false;
    }

  this->Children.insert(childIt, addedItem);
  this->Modified();
  //TODO: Invoke insert custom event?
  return true;
}

//---------------------------------------------------------------------------
bool vtkSubjectHierarchyItem::RemoveChild(vtkSubjectHierarchyItem* item)
{
  if (!item)
    {
    vtkErrorMacro("RemoveChild: Invalid subject hierarchy item given to remove from item '" << this->GetName() << "'");
    return false;
    }

  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    if (item == childIt->GetPointer())
      {
      break;
      }
    }
  if (childIt == this->Children.end())
    {
    vtkErrorMacro("RemoveChild: Subject hierarchy item '" << item->GetName() << "' not found in item '" << this->GetName() << "'");
    return false;
    }

  // Remove child
  this->Children.erase(childIt);
  this->Modified();
  //TODO: Invoke remove custom event?
  return true;
}

//---------------------------------------------------------------------------
bool vtkSubjectHierarchyItem::RemoveChild(vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID itemID)
{
  ChildVector::iterator childIt;
  for (childIt=this->Children.begin(); childIt!=this->Children.end(); ++childIt)
    {
    if (itemID == (*childIt)->ID)
      {
      break;
      }
    }
  if (childIt == this->Children.end())
    {
    vtkErrorMacro("RemoveChild: Subject hierarchy item with ID " << itemID << " not found in item '" << this->GetName() << "'");
    return false;
    }

  // Remove child
  this->Children.erase(childIt);
  this->Modified();
  //TODO: Invoke remove custom event?
  return true;
}

//---------------------------------------------------------------------------
void vtkSubjectHierarchyItem::RemoveAllChildren()
{
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> childIDs;
  this->GetDirectChildrenIDs(childIDs);

  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID>::iterator childIt;
  for (childIt=childIDs.begin(); childIt!=childIDs.end(); ++childIt)
    {
    this->RemoveChild(*childIt);
    }
}

//---------------------------------------------------------------------------
void vtkSubjectHierarchyItem::SetUID(std::string uidName, std::string uidValue)
{
  // Use the find function to prevent adding an empty UID to the list
  if (this->UIDs.find(uidName) != this->UIDs.end())
    {
    // Log warning if the new UID value is different than the one already set
    if (this->UIDs[uidName].compare(uidValue))
      {
      vtkWarningMacro( "SetUID: UID with name '" << uidName << "' already exists in subject hierarchy item '" << this->GetName()
        << "' with value '" << this->UIDs[uidName] << "'. Replacing it with value '" << uidValue << "'!" );
      }
    else
      {
      return; // Do nothing if the UID values match
      }
    }
  this->UIDs[uidName] = uidValue;
  //TODO:
  //this->InvokeEvent(SubjectHierarchyUIDAddedEvent, this);
  this->Modified();
}

//---------------------------------------------------------------------------
std::string vtkSubjectHierarchyItem::GetUID(std::string uidName)
{
  // Use the find function to prevent adding an empty UID to the list
  if (this->UIDs.find(uidName) != this->UIDs.end())
    {
    return this->UIDs[uidName];
    }
  return std::string();
}

//---------------------------------------------------------------------------
void vtkSubjectHierarchyItem::SetAttribute(std::string attributeName, std::string attributeValue)
{
  if (attributeName.empty())
    {
    vtkErrorMacro("SetAttribute: Name parameter is expected to have at least one character.");
    return;
    }
  // Use the find function to prevent adding an empty UID to the list
  if ( this->Attributes.find(attributeName) != this->Attributes.end()
    && !attributeValue.compare(this->Attributes[attributeName]) )
    {
    return; // Attribute to set is same as original value, nothing to do
    }
  if (attributeValue.empty())
    {
    this->Attributes.erase(attributeName);
    }
  else
    {
    this->Attributes[attributeName] = attributeValue;
    }
  this->Modified();
}

//---------------------------------------------------------------------------
std::string vtkSubjectHierarchyItem::GetAttribute(std::string attributeName)
{
  // Use the find function to prevent adding an empty UID to the list
  if (this->Attributes.find(attributeName) != this->Attributes.end())
    {
    return this->Attributes[attributeName];
    }
  return std::string();
}

//---------------------------------------------------------------------------
std::string vtkSubjectHierarchyItem::GetAttributeFromAncestor(std::string attributeName, std::string level)
{
  if (attributeName.empty())
    {
    vtkErrorMacro("GetAttributeFromAncestor: Empty attribute name!");
    return std::string();
    }

  std::string attributeValue("");
  vtkSubjectHierarchyItem* currentItem = this;
  while (currentItem && currentItem->Parent)
    {
    currentItem = currentItem->Parent;
    if (!currentItem)
      {
      break;
      }
    else if (!level.empty() && currentItem->Level.compare(level))
      {
      continue;
      }

    attributeValue = currentItem->GetAttribute(attributeName);
    if (!attributeValue.empty())
      {
      return attributeValue;
      }
    }

  return attributeValue;
}

//---------------------------------------------------------------------------
vtkSubjectHierarchyItem* vtkSubjectHierarchyItem::GetAncestorAtLevel(std::string level)
{
  if (level.empty())
    {
    vtkErrorMacro("GetAncestorAtLevel: Empty subject hierarchy level");
    return NULL;
    }

  // We do not return source node even if it is at the requested level, we only look in the ancestors
  vtkSubjectHierarchyItem* currentItem = this;
  while (currentItem && currentItem->Parent)
    {
    currentItem = currentItem->Parent;
    if (currentItem && !currentItem->Level.compare(level))
      {
      // Level found
      return currentItem;
      }
    }

  vtkWarningMacro("GetAncestorAtLevel: No ancestor found for item '" << this->GetName() << "' at level '" << level);
  return NULL;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class vtkMRMLSubjectHierarchyNode::vtkInternal
{
public:
  vtkInternal(vtkMRMLSubjectHierarchyNode* external);
  ~vtkInternal();

public:
  vtkSubjectHierarchyItem* Root;
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID RootID;

private:
  vtkMRMLSubjectHierarchyNode* External;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::vtkInternal::vtkInternal(vtkMRMLSubjectHierarchyNode* external)
: External(external)
{
  this->Root = vtkSubjectHierarchyItem::New();
  this->RootID = this->Root->AddItemToTree(NULL, NULL, "Root", "Root");
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::vtkInternal::~vtkInternal()
{
  if (this->Root)
    {
    this->Root->Delete();
    this->Root = NULL;
    }
}


//----------------------------------------------------------------------------
// vtkMRMLSubjectHierarchyNode members
//----------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::vtkMRMLSubjectHierarchyNode()
{
  this->Internal = new vtkInternal(this);
}

//----------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::~vtkMRMLSubjectHierarchyNode()
{
  delete this->Internal;
  this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);

  //TODO:
  //os << indent << " UIDs=\"";
  //for (std::map<std::string, std::string>::iterator uidsIt = this->UIDs.begin(); uidsIt != this->UIDs.end(); ++uidsIt)
  //  {
  //  os << uidsIt->first << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR.c_str()
  //    << uidsIt->second << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR.c_str();
  //  }
  //os << "\"";
}

//----------------------------------------------------------------------------
const char* vtkMRMLSubjectHierarchyNode::GetNodeTagName()
{
  return "SubjectHierarchy";
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::ReadXMLAttributes( const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  //TODO:
  //const char* attName;
  //const char* attValue;
  //while (*atts != NULL)
  //  {
  //  attName = *(atts++);
  //  attValue = *(atts++);
  //  if (!strcmp(attName, "Level"))
  //    {
  //    this->SetLevel(attValue);
  //    }
  //  else if (!strcmp(attName, "OwnerPluginName"))
  //    {
  //    this->SetOwnerPluginName(attValue);
  //    }
  //  else if (!strcmp(attName, "OwnerPluginAutoSearch"))
  //    {
  //    this->OwnerPluginAutoSearch =
  //      (strcmp(attValue,"true") ? false : true);
  //    }
  //  else if (!strcmp(attName, "UIDs"))
  //    {
  //    std::stringstream ss;
  //    ss << attValue;
  //    std::string valueStr = ss.str();

  //    this->UIDs.clear();
  //    size_t itemSeparatorPosition = valueStr.find(vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR);
  //    while (itemSeparatorPosition != std::string::npos)
  //      {
  //      std::string itemStr = valueStr.substr(0, itemSeparatorPosition);
  //      size_t nameValueSeparatorPosition = itemStr.find(vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR);

  //      std::string name = itemStr.substr(0, nameValueSeparatorPosition);
  //      std::string value = itemStr.substr(nameValueSeparatorPosition + vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR.size());
  //      this->AddUID(name, value);

  //      valueStr = valueStr.substr(itemSeparatorPosition + vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR.size());
  //      itemSeparatorPosition = valueStr.find(vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR);
  //      }
  //    if (! valueStr.empty() )
  //      {
  //      std::string itemStr = valueStr.substr(0, itemSeparatorPosition);
  //      size_t tagLevelSeparatorPosition = itemStr.find(vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR);

  //      std::string name = itemStr.substr(0, tagLevelSeparatorPosition);
  //      std::string value = itemStr.substr(tagLevelSeparatorPosition + vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR.size());
  //      this->AddUID(name, value);
  //      }
  //    }
  //  }

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);

  vtkIndent indent(nIndent);

  //TODO:
  //of << indent << " Level=\""
  //  << (this->Level ? this->Level : "NULL" ) << "\"";

  //of << indent << " OwnerPluginName=\""
  //  << (this->OwnerPluginName ? this->OwnerPluginName : "NULL" ) << "\"";

  //of << indent << " OwnerPluginAutoSearch=\""
  //  << (this->OwnerPluginAutoSearch ? "true" : "false") << "\"";

  //of << indent << " UIDs=\"";
  //for (std::map<std::string, std::string>::iterator uidsIt = this->UIDs.begin(); uidsIt != this->UIDs.end(); ++uidsIt)
  //  {
  //  of << uidsIt->first << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_NAME_VALUE_SEPARATOR
  //    << uidsIt->second << vtkMRMLSubjectHierarchyNode::SUBJECTHIERARCHY_UID_ITEM_SEPARATOR;
  //  }
  //of << "\"";
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLSubjectHierarchyNode *node = (vtkMRMLSubjectHierarchyNode*)anode;

  //TODO:
  //this->SetLevel(node->Level);
  //this->SetOwnerPluginName(node->OwnerPluginName);
  //this->SetOwnerPluginAutoSearch(node->GetOwnerPluginAutoSearch());

  //this->UIDs = node->GetUIDs();
  vtkErrorMacro("Not implemented!");

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::GetRootItemID()
{
  return this->Internal->RootID;
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSubjectHierarchyNode::GetItemDataNode(SubjectHierarchyItemID itemID)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemDataNode: Failed to find subject hierarchy item by ID " << itemID);
    return NULL;
    }

  return item->DataNode.GetPointer();
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetItemName(SubjectHierarchyItemID itemID, std::string name)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("SetItemName: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  //TODO: Set data node name if there is a data node?
  item->Name = name;
  this->Modified();
}

//----------------------------------------------------------------------------
std::string vtkMRMLSubjectHierarchyNode::GetItemName(SubjectHierarchyItemID itemID)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemName: Failed to find subject hierarchy item by ID " << itemID);
    return std::string();
    }

  return item->GetName();
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetItemParent(SubjectHierarchyItemID itemID, vtkMRMLNode* parentNode)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("SetItemParent: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  vtkSubjectHierarchyItem* parentItem = this->Internal->Root->FindChildByDataNode(parentNode);
  if (!parentItem)
    {
    vtkErrorMacro("SetItemParent: Failed to find subject hierarchy item by data MRML node " << parentNode->GetName());
    return;
    }

  item->Parent = parentItem;
  this->Modified();
}

//----------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::GetItemParent(SubjectHierarchyItemID itemID)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemParent: Failed to find subject hierarchy item by ID " << itemID);
    return INVALID_ITEM_ID;
    }
  if (!item->Parent)
    {
    return INVALID_ITEM_ID;
    }

  return item->Parent->ID;
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetItemLevel(SubjectHierarchyItemID itemID, std::string level)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("SetItemLevel: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  item->Level = level;
  this->Modified();
}

//----------------------------------------------------------------------------
std::string vtkMRMLSubjectHierarchyNode::GetItemLevel(SubjectHierarchyItemID itemID)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemLevel: Failed to find subject hierarchy item by ID " << itemID);
    return std::string();
    }

  return item->Level;
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetItemOwnerPluginName(SubjectHierarchyItemID itemID, std::string owherPluginName)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("SetItemOwnerPluginName: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  item->OwnerPluginName = owherPluginName;
  this->Modified();
}

//----------------------------------------------------------------------------
std::string vtkMRMLSubjectHierarchyNode::GetItemOwnerPluginName(SubjectHierarchyItemID itemID)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemOwnerPluginName: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  return item->OwnerPluginName;
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetItemOwnerPluginAutoSearch(SubjectHierarchyItemID itemID, bool owherPluginAutoSearch)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("SetItemOwnerPluginAutoSearch: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  item->OwnerPluginAutoSearch = owherPluginAutoSearch;
  this->Modified();
}

//----------------------------------------------------------------------------
bool vtkMRMLSubjectHierarchyNode::GetItemOwnerPluginAutoSearch(SubjectHierarchyItemID itemID)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemOwnerPluginAutoSearch: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  return item->OwnerPluginAutoSearch;
}

//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetItemUID(SubjectHierarchyItemID itemID, std::string uidName, std::string uidValue)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("SetItemUID: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  item->SetUID(uidName, uidValue);
}

//----------------------------------------------------------------------------
std::string vtkMRMLSubjectHierarchyNode::GetItemUID(SubjectHierarchyItemID itemID, std::string uidName)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemUID: Failed to find subject hierarchy item by ID " << itemID);
    return std::string();
    }

  return item->GetUID(uidName);
}


//----------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetItemAttribute(SubjectHierarchyItemID itemID, std::string attributeName, std::string attributeValue)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("SetItemAttribute: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  item->SetAttribute(attributeName, attributeValue);
}

//----------------------------------------------------------------------------
std::string vtkMRMLSubjectHierarchyNode::GetItemAttribute(SubjectHierarchyItemID itemID, std::string attributeName)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemAttribute: Failed to find subject hierarchy item by ID " << itemID);
    return std::string();
    }

  return item->GetAttribute(attributeName);
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyItemByUID(const char* uidName, const char* uidValue)
{
  if (!uidName || !uidValue)
    {
    vtkErrorMacro("GetSubjectHierarchyNodeByUID: Invalid UID name or value!");
    return INVALID_ITEM_ID;
    }
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByUID(uidName, uidValue);
  return (item ? item->ID : INVALID_ITEM_ID);
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyItemByUIDList(const char* uidName, const char* uidValue)
{
  if (!uidName || !uidValue)
    {
    vtkErrorMacro("GetSubjectHierarchyItemByUIDList: Invalid UID name or value!");
    return INVALID_ITEM_ID;
    }
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByUIDList(uidName, uidValue);
  return (item ? item->ID : INVALID_ITEM_ID);
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyItemByDataNode(vtkMRMLNode* dataNode)
{
  if (!dataNode)
    {
    vtkErrorMacro("GetSubjectHierarchyItemByDataNode: Invalid data node to find!");
    return INVALID_ITEM_ID;
    }

  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByDataNode(dataNode);
  return (item ? item->ID : INVALID_ITEM_ID);
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::CreateSubjectHierarchyItem(
  vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID parentItemID,
  vtkMRMLNode* dataNode/*=NULL*/,
  std::string level/*=vtkMRMLSubjectHierarchyConstants::GetSubjectHierarchyLevelFolder()*/,
  std::string name/*=""*/ )
{
  // Use existing subject hierarchy item if found (only one subject hierarchy item can be associated with a data node)
  SubjectHierarchyItemID itemID = INVALID_ITEM_ID;
  if (dataNode)
    {
    itemID = this->GetSubjectHierarchyItemByDataNode(dataNode);
    }
  if (itemID != INVALID_ITEM_ID)
    {
    // Set properties if item already existed for data node
    // This should be the case every time data node is not NULL, because subject hierarchy items are added automatically
    // for supported data MRML nodes (supported = there is a plugin that can "own" it)
    vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
    item->Name = ""; // No matter what the user gave, the name of the data node is used (because it exists)
    if ( (item->Parent && item->Parent->ID != parentItemID) || item->Level.compare(level))
      {
      item->Parent = this->Internal->Root->FindChildByID(parentItemID);
      item->Level;
      item->Modified();
      }
    }
  // No data node was specified, or no subject hierarchy item was found for the given data node
  else
    {
    vtkSubjectHierarchyItem* parentItem = NULL;
    if (parentItemID == this->Internal->RootID)
      {
      parentItem = this->Internal->Root;
      }
    // Find parent item if not the root item was given
    if (!parentItem)
      {
      parentItem = this->Internal->Root->FindChildByID(parentItemID);
      }
    if (!parentItem)
      {
      vtkErrorMacro("CreateSubjectHierarchyItem: Failed to find parent subject hierarchy item by ID " << parentItemID);
      return INVALID_ITEM_ID;
      }

    // Create subject hierarchy item and add it to the tree
    vtkSubjectHierarchyItem* item = vtkSubjectHierarchyItem::New();
    itemID = item->AddItemToTree( parentItem, dataNode, level, (dataNode ? "" : name) ); // Name is not used if valid data node is given
    }

  return itemID;
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::GetChildWithName(SubjectHierarchyItemID parentItemID, std::string name)
{
  vtkSubjectHierarchyItem* parentItem = this->Internal->Root->FindChildByID(parentItemID);
  if (!parentItem)
    {
    vtkErrorMacro("GetChildWithName: Failed to find subject hierarchy item by ID " << parentItemID);
    return;
    }

  // Search only one level (not recursive)
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> foundItemIDs;
  parentItem->FindChildIDsByName(name, foundItemIDs, false, false);
  if (foundItemIDs.size() == 0)
    {
    vtkErrorMacro("GetChildWithName: Failed to find subject hierarchy item with name '" << name
      << "' under item with ID " << parentItemID);
    return INVALID_ITEM_ID;
    }
  else if (foundItemIDs.size() > 1)
    {
    vtkWarningMacro("GetChildWithName: Multiple subject hierarchy item found with name '" << name
      << "' under item with ID " << parentItemID << ". Returning first");
    }
  return foundItemIDs[0];
}

//---------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::GetDataNodesInBranch(SubjectHierarchyItemID itemID, vtkCollection* dataNodeCollection, const char* childClass/*=NULL*/)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetDataNodesInBranch: Failed to find subject hierarchy item by ID " << itemID);
    return;
    }

  item->GetDataNodesInBranch(dataNodeCollection, childClass);
}

//---------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::SetDisplayVisibilityForBranch(SubjectHierarchyItemID itemID, int visible)
{
  if (visible != 0 && visible != 1)
    {
    vtkErrorMacro( "SetDisplayVisibilityForBranch: Invalid visibility value to set: " << visible
      << ". Needs to be one of the following: 0:Hidden, 1:Visible, 2:PartiallyVisible" );
    return;
    }
  if (this->Scene->IsBatchProcessing())
    {
    //vtkDebugMacro("SetDisplayVisibilityForBranch: Batch processing is on, returning");
    return;
    }

  // Get all child displayable nodes for branch
  vtkNew<vtkCollection> childDisplayableNodes;
  this->GetDataNodesInBranch(itemID, childDisplayableNodes.GetPointer(), "vtkMRMLDisplayableNode");

  childDisplayableNodes->InitTraversal();
  std::set<vtkMRMLSubjectHierarchyNode*> parentNodes;
  for (int childNodeIndex = 0;
       childNodeIndex < childDisplayableNodes->GetNumberOfItems();
       ++childNodeIndex)
    {
    vtkMRMLDisplayableNode* displayableNode =
        vtkMRMLDisplayableNode::SafeDownCast(childDisplayableNodes->GetItemAsObject(childNodeIndex));
    if (displayableNode)
      {
      // Create default display node is there is no display node associated
      vtkMRMLDisplayNode* displayNode = displayableNode->GetDisplayNode();
      if (!displayNode)
        {
        displayableNode->CreateDefaultDisplayNodes();
        }

      // Set display visibility
      displayableNode->SetDisplayVisibility(visible);

      // Set slice intersection visibility through display node
      displayNode = displayableNode->GetDisplayNode();
      if (displayNode)
        {
        displayNode->SetSliceIntersectionVisibility(visible);
        }
      displayableNode->Modified();
      }
    }

  this->Modified();
}

//---------------------------------------------------------------------------
int vtkMRMLSubjectHierarchyNode::GetDisplayVisibilityForBranch(SubjectHierarchyItemID itemID)
{
  int visible = -1;

  // Get all child displayable nodes for branch
  vtkNew<vtkCollection> childDisplayableNodes;
  this->GetDataNodesInBranch(itemID, childDisplayableNodes.GetPointer(), "vtkMRMLDisplayableNode");

  // Add associated displayable node for the parent item too
  vtkMRMLDisplayableNode* associatedDisplayableDataNode = vtkMRMLDisplayableNode::SafeDownCast(
    this->GetItemDataNode(itemID));
  if (associatedDisplayableDataNode)
    {
    childDisplayableNodes->AddItem(associatedDisplayableDataNode);
    }

  // Determine visibility state based on all displayable nodes involved
  childDisplayableNodes->InitTraversal();
  for (int childNodeIndex=0; childNodeIndex<childDisplayableNodes->GetNumberOfItems(); ++childNodeIndex)
    {
    vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(
      childDisplayableNodes->GetItemAsObject(childNodeIndex) );
    // Omit volume nodes from the process (they are displayed differently than every other type)
    // TODO: This is not very elegant or safe, it would be better to distinguish between visibility modes, or overhaul the visibility features completely
    if ( displayableNode
      && ( !displayableNode->IsA("vtkMRMLVolumeNode")
        || !strcmp(displayableNode->GetClassName(), "vtkMRMLSegmentationNode") ) )
      {
      // If we set visibility
      if (visible == -1)
        {
        visible = displayableNode->GetDisplayVisibility();

        // We expect only 0 or 1 from leaf nodes
        if (visible == 2)
          {
          vtkWarningMacro("GetDisplayVisibilityForBranch: Unexpected visibility value for node " << displayableNode->GetName());
          }
        }
      // If the current node visibility does not match the found visibility, then set partial visibility
      else if (displayableNode->GetDisplayVisibility() != visible)
        {
        return 2;
        }
      }
    }

  return visible;
}

//---------------------------------------------------------------------------
bool vtkMRMLSubjectHierarchyNode::IsItemLevel(SubjectHierarchyItemID itemID, std::string level)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("IsItemLevel: Failed to find subject hierarchy item by ID " << itemID);
    return false;
    }

  return !item->Level.compare(level);
}

//---------------------------------------------------------------------------
std::string vtkMRMLSubjectHierarchyNode::GetAttributeFromItemAncestor(SubjectHierarchyItemID itemID, std::string attributeName, std::string level)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetAttributeFromAncestor: Failed to find subject hierarchy item by ID " << itemID);
    return std::string();
    }

  return item->GetAttributeFromAncestor(attributeName, level);
}

//---------------------------------------------------------------------------
vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID vtkMRMLSubjectHierarchyNode::GetItemAncestorAtLevel(SubjectHierarchyItemID itemID, std::string level)
{
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetItemAncestorAtLevel: Failed to find subject hierarchy item by ID " << itemID);
    return INVALID_ITEM_ID;
    }

  vtkSubjectHierarchyItem* ancestorItem = item->GetAncestorAtLevel(level);
  if (ancestorItem)
    {
    return ancestorItem->ID;
    }
  return INVALID_ITEM_ID;
}

//---------------------------------------------------------------------------
bool vtkMRMLSubjectHierarchyNode::IsAnyNodeInBranchTransformed(SubjectHierarchyItemID itemID, vtkMRMLTransformNode* exceptionNode/*=NULL*/)
{
  // Check transformable node from the item itself if any
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("IsAnyNodeInBranchTransformed: Failed to find subject hierarchy item by ID " << itemID);
    return false;
    }
  if (item->DataNode)
    {
    vtkMRMLTransformableNode* transformableDataNode = vtkMRMLTransformableNode::SafeDownCast(item->DataNode);
    if ( transformableDataNode && transformableDataNode->GetParentTransformNode()
      && transformableDataNode->GetParentTransformNode() != exceptionNode)
      {
      return true;
      }
    }

  // Check all child transformable nodes for branch
  vtkNew<vtkCollection> childTransformableNodes;
  this->GetDataNodesInBranch(itemID, childTransformableNodes.GetPointer(), "vtkMRMLTransformableNode");
  childTransformableNodes->InitTraversal();

  for (int childNodeIndex=0; childNodeIndex<childTransformableNodes->GetNumberOfItems(); ++childNodeIndex)
    {
    vtkMRMLTransformableNode* transformableNode = vtkMRMLTransformableNode::SafeDownCast(
      childTransformableNodes->GetItemAsObject(childNodeIndex) );
    vtkMRMLTransformNode* parentTransformNode = NULL;
    if (transformableNode && (parentTransformNode = transformableNode->GetParentTransformNode()))
      {
      if (parentTransformNode != exceptionNode)
        {
        return true;
        }
      }
    }

  return false;
}

//---------------------------------------------------------------------------
void vtkMRMLSubjectHierarchyNode::DeserializeUIDList(std::string uidListString, std::vector<std::string>& deserializedUIDList)
{
  deserializedUIDList.clear();
  char separatorCharacter = ' ';
  size_t separatorPosition = uidListString.find( separatorCharacter );
  while (separatorPosition != std::string::npos)
    {
    std::string uid = uidListString.substr(0, separatorPosition);
    deserializedUIDList.push_back(uid);
    uidListString = uidListString.substr( separatorPosition+1 );
    separatorPosition = uidListString.find( separatorCharacter );
    }
  // Add last UID in case there was no space at the end (which is default behavior)
  if (!uidListString.empty() && uidListString.find(separatorCharacter) == std::string::npos)
    {
    deserializedUIDList.push_back(uidListString);
    }
}

//---------------------------------------------------------------------------
std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyItemsReferencedFromItemByDICOM(
  SubjectHierarchyItemID itemID )
{
  std::vector<vtkMRMLSubjectHierarchyNode::SubjectHierarchyItemID> referencedItemIDs;
  vtkSubjectHierarchyItem* item = this->Internal->Root->FindChildByID(itemID);
  if (!item)
    {
    vtkErrorMacro("GetSubjectHierarchyItemsReferencedFromItemByDICOM: Failed to find subject hierarchy item by ID " << itemID);
    return referencedItemIDs;
    }

  // Get referenced SOP instance UIDs
  std::string referencedInstanceUIDsAttribute = item->GetAttribute(
    vtkMRMLSubjectHierarchyConstants::GetDICOMReferencedInstanceUIDsAttributeName() );
  if (referencedInstanceUIDsAttribute.empty())
    {
    return referencedItemIDs;
    }

  // De-serialize SOP instance UID list
  std::vector<vtkSubjectHierarchyItem*> referencedItems;
  std::vector<std::string> referencedSopInstanceUids;
  this->DeserializeUIDList(referencedInstanceUIDsAttribute, referencedSopInstanceUids);

  // Find subject hierarchy items by SOP instance UIDs
  for (std::vector<std::string>::iterator uidIt=referencedSopInstanceUids.begin(); uidIt!=referencedSopInstanceUids.end(); ++uidIt)
    {
    // Find first referenced item in the subject hierarchy tree
    if (referencedItems.empty())
      {
      vtkSubjectHierarchyItem* referencedItem = this->Internal->Root->FindChildByUIDList(
        vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName(), (*uidIt) );
      if (referencedItem)
        {
        referencedItems.push_back(referencedItem);
        }
      }
    else
      {
      // If we found a referenced node, check the other instances in those nodes first to save time
      bool foundUidInFoundReferencedItems = false;
      for (std::vector<vtkSubjectHierarchyItem*>::iterator itemIt=referencedItems.begin(); itemIt!=referencedItems.end(); ++itemIt)
        {
        // Get instance UIDs of the referenced item
        std::string uids = (*itemIt)->GetUID(vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName());
        if (uids.find(*uidIt) != std::string::npos)
          {
          // If we found the UID in the already found referenced items, then we don't need to do anything
          foundUidInFoundReferencedItems = true;
          break;
          }
        }
      // If the referenced SOP instance UID is not contained in the already found referenced items, then we look in the tree
      if (!foundUidInFoundReferencedItems)
        {
        vtkSubjectHierarchyItem* referencedItem = this->Internal->Root->FindChildByUIDList(
          vtkMRMLSubjectHierarchyConstants::GetDICOMInstanceUIDName(), (*uidIt) );
        if (referencedItem)
          {
          referencedItems.push_back(referencedItem);
          }
        }
      }
    }

  // Copy item IDs into output vector
  for (std::vector<vtkSubjectHierarchyItem*>::iterator itemIt=referencedItems.begin(); itemIt!=referencedItems.end(); ++itemIt)
    {
    referencedItemIDs.push_back( (*itemIt)->ID );
    }

  return referencedItemIDs;
}
