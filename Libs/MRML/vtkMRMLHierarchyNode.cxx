/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

Program:   3D Slicer
Module:    $RCSfile: vtkMRMLTransformNode.cxx,v $
Date:      $Date: 2006/03/17 17:01:53 $
Version:   $Revision: 1.14 $

=========================================================================auto=*/

#include <sstream>
#include <algorithm>

#include "vtkObjectFactory.h"

#include "vtkMRMLHierarchyNode.h"

#include "vtkMRMLScene.h"

vtkCxxSetReferenceStringMacro(vtkMRMLHierarchyNode, ParentNodeIDReference);
vtkCxxSetReferenceStringMacro(vtkMRMLHierarchyNode, AssociatedNodeIDReference);

typedef std::map<std::string, std::vector< vtkMRMLHierarchyNode *> > HierarchyChildrenNodesType;

std::map< vtkMRMLScene*, HierarchyChildrenNodesType> vtkMRMLHierarchyNode::SceneHierarchyChildrenNodes = std::map< vtkMRMLScene*, HierarchyChildrenNodesType>();
std::map< vtkMRMLScene*, unsigned long> vtkMRMLHierarchyNode::SceneHierarchyChildrenNodesMTime = std::map< vtkMRMLScene*, unsigned long>();

double vtkMRMLHierarchyNode::MaximumSortingValue = 0;

typedef std::map<std::string, vtkMRMLHierarchyNode *> AssociatedHierarchyNodesType;

std::map< vtkMRMLScene*, AssociatedHierarchyNodesType> vtkMRMLHierarchyNode::SceneAssociatedHierarchyNodes = std::map< vtkMRMLScene*, AssociatedHierarchyNodesType>();

std::map< vtkMRMLScene*, unsigned long> vtkMRMLHierarchyNode::SceneAssociatedHierarchyNodesMTime = std::map< vtkMRMLScene*, unsigned long>();;


typedef vtkMRMLHierarchyNode* const vtkMRMLHierarchyNodePointer; 
bool vtkMRMLHierarchyNodeSortPredicate(vtkMRMLHierarchyNodePointer d1, vtkMRMLHierarchyNodePointer d2);
bool vtkMRMLHierarchyNodeSortPredicate(vtkMRMLHierarchyNodePointer d1, vtkMRMLHierarchyNodePointer d2)
{
  return d1->GetSortingValue() < d2->GetSortingValue();
}

//------------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkMRMLHierarchyNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLHierarchyNode");
  if(ret)
    {
    return (vtkMRMLHierarchyNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLHierarchyNode;
}

//----------------------------------------------------------------------------
vtkMRMLHierarchyNode::vtkMRMLHierarchyNode()
{
  this->HideFromEditors = 0;

  this->ParentNodeIDReference = NULL;

  this->AssociatedNodeIDReference = NULL;

  this->SortingValue = 0;

  this->AllowMultipleChildren = 1;

}

//----------------------------------------------------------------------------
vtkMRMLHierarchyNode::~vtkMRMLHierarchyNode()
{
  if (this->ParentNodeIDReference) 
    {
    delete [] this->ParentNodeIDReference;
    this->ParentNodeIDReference = NULL;
    }
  if (this->AssociatedNodeIDReference) 
    {
    delete [] this->AssociatedNodeIDReference;
    this->AssociatedNodeIDReference = NULL;
    }
}

//-----------------------------------------------------------------------------

vtkMRMLNode* vtkMRMLHierarchyNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLHierarchyNode");
  if(ret)
    {
    return (vtkMRMLHierarchyNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLHierarchyNode;
}


//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);

  if (this->ParentNodeIDReference != NULL) 
    {
    of << indent << " parentNodeRef=\"" << this->ParentNodeIDReference << "\"";
    }
  if (this->AssociatedNodeIDReference != NULL) 
    {
    of << indent << " associatedNodeRef=\"" << this->AssociatedNodeIDReference << "\"";
    }
  of << indent << " sortingValue=\"" << this->SortingValue << "\"";
  of << indent << " allowMultipleChildren=\"" << (this->AllowMultipleChildren ? "true" : "false") << "\"";

}

//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::UpdateReferenceID(const char *oldID, const char *newID)
{
  Superclass::UpdateReferenceID(oldID, newID);
  if (this->ParentNodeIDReference && !strcmp(oldID, this->ParentNodeIDReference))
    {
    this->SetParentNodeID(newID);
    }
  else if (this->AssociatedNodeIDReference && !strcmp(oldID, this->AssociatedNodeIDReference))
    {
    this->SetAssociatedNodeID(newID);
    }
}
//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
    {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "parentNodeRef")) 
      {
      // dont reset SortingValue
      double sortingValue = this->GetSortingValue();
      this->SetParentNodeID(attValue);
      this->SetSortingValue(sortingValue);
      //this->Scene->AddReferencedNodeID(this->ParentNodeIDReference, this);
      }
    if (!strcmp(attName, "associatedNodeRef")) 
      {
      this->SetAssociatedNodeID(attValue);
      }
    else if (!strcmp(attName, "sortingValue")) 
      {
      std::stringstream ss;
      ss << attValue;
      ss >> SortingValue;
      }
    else if (!strcmp(attName, "allowMultipleChildren"))
      {
      if (!strcmp(attValue,"true")) 
        {
        this->AllowMultipleChildren = 1;
        }
      else
        {
        this->AllowMultipleChildren = 0;
        }
      }
  }

  this->EndModify(disabledModify);

}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLHierarchyNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);
  vtkMRMLHierarchyNode *node = (vtkMRMLHierarchyNode *) anode;
  this->SetParentNodeID(node->ParentNodeIDReference);
  this->SetAssociatedNodeID(node->AssociatedNodeIDReference);
  this->SetSortingValue(node->SortingValue);
  this->SetAllowMultipleChildren(node->AllowMultipleChildren);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << indent << "AssociatedNodeID: " <<
    (this->AssociatedNodeIDReference ? this->AssociatedNodeIDReference : "(none)") << "\n";
  os << indent << "ParentNodeID: " <<
    (this->ParentNodeIDReference ? this->ParentNodeIDReference : "(none)") << "\n";
  os << indent << "SortingValue:     " << this->SortingValue << "\n";
  os << indent << "AllowMultipleChildren: " << (this->AllowMultipleChildren ? "true" : "false") << "\n";

}

//----------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkMRMLHierarchyNode::GetParentNode()
{
  vtkMRMLHierarchyNode* node = NULL;
  if (this->GetScene() && this->ParentNodeIDReference != NULL )
    {
    vtkMRMLNode* snode = this->GetScene()->GetNodeByID(this->ParentNodeIDReference);
    node = vtkMRMLHierarchyNode::SafeDownCast(snode);
    }
  return node;
}

//-----------------------------------------------------------
void vtkMRMLHierarchyNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//-----------------------------------------------------------
void vtkMRMLHierarchyNode::UpdateReferences()
{
  Superclass::UpdateReferences();
  
  if (this->ParentNodeIDReference != NULL && this->Scene->GetNodeByID(this->ParentNodeIDReference) == NULL)
    {
    this->SetParentNodeID(NULL);
    }
  if (this->AssociatedNodeIDReference != NULL && this->Scene->GetNodeByID(this->AssociatedNodeIDReference) == NULL)
    {
    this->SetAssociatedNodeID(NULL);
    }
}

//-----------------------------------------------------------
void vtkMRMLHierarchyNode::SetParentNodeID(const char* ref) 
{
  if ((this->ParentNodeIDReference && ref && strcmp(ref, this->ParentNodeIDReference)) ||
      (this->ParentNodeIDReference != ref))
    {

    vtkMRMLHierarchyNode *oldParentNode = this->GetParentNode();

    int disableModify = this->StartModify();

    this->SetParentNodeIDReference(ref);
    this->SetSortingValue(++MaximumSortingValue);

    this->HierarchyIsModified(this->GetScene());

    this->EndModify(disableModify);

    vtkMRMLHierarchyNode *parentNode = this->GetParentNode();
    if (oldParentNode)
      {
      oldParentNode->InvokeEvent(vtkMRMLHierarchyNode::ChildNodeRemovedEvent);
      oldParentNode->Modified();
      }
    if (parentNode)
      {
      parentNode->InvokeEvent(vtkMRMLHierarchyNode::ChildNodeAddedEvent);
      parentNode->Modified();
      }

    }
}

//----------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkMRMLHierarchyNode::GetTopParentNode()
{
  vtkMRMLHierarchyNode *node = NULL;
  vtkMRMLHierarchyNode *parent = vtkMRMLHierarchyNode::SafeDownCast(this->GetParentNode());
  if (parent == NULL) 
    {
    node = this;
    }
  else 
    {
    node =  parent->GetTopParentNode();
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::GetAllChildrenNodes(std::vector< vtkMRMLHierarchyNode *> &childrenNodes)
{
  if (this->GetScene() == NULL)
    {
    return;
    }
  
  this->UpdateChildrenMap();

  std::map< vtkMRMLScene*, HierarchyChildrenNodesType>::iterator siter = 
        SceneHierarchyChildrenNodes.find(this->GetScene());
  
  HierarchyChildrenNodesType::iterator iter = 
    siter->second.find(std::string(this->GetID()));
  if (iter == siter->second.end()) 
    {
    return;
    }
  for (unsigned int i=0; i<iter->second.size(); i++)
    {
    childrenNodes.push_back(iter->second[i]);
    iter->second[i]->GetAllChildrenNodes(childrenNodes);
    }
}

//----------------------------------------------------------------------------
std::vector< vtkMRMLHierarchyNode *> vtkMRMLHierarchyNode::GetChildrenNodes()
{
  std::vector< vtkMRMLHierarchyNode *> childrenNodes;
  if (this->GetScene() == NULL)
    {
    return childrenNodes;
    }
  
  this->UpdateChildrenMap();

  std::map< vtkMRMLScene*, HierarchyChildrenNodesType>::iterator siter = 
        SceneHierarchyChildrenNodes.find(this->GetScene());

  HierarchyChildrenNodesType::iterator iter =
    siter->second.find(std::string(this->GetID()));
  if (iter == siter->second.end()) 
    {
    return childrenNodes;
    }
  for (unsigned int i=0; i<iter->second.size(); i++)
    {
    childrenNodes.push_back(iter->second[i]);
    }

  // Sort the vector using predicate and std::sort
  std::sort(childrenNodes.begin(), childrenNodes.end(), vtkMRMLHierarchyNodeSortPredicate);

  return childrenNodes;
}

//----------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkMRMLHierarchyNode::GetNthChildNode(int index)
{
  std::vector< vtkMRMLHierarchyNode *> childrenNodes = this->GetChildrenNodes();
  if (index < 0 || index > (int)(childrenNodes.size()-1))
    {
    vtkErrorMacro("vtkMRMLHierarchyNode::GetNthChildNode() index " << index << " outside the range 0-" << childrenNodes.size()-1 );
    return NULL;
    }
  else
    {
    return childrenNodes[index];
    }
}

//----------------------------------------------------------------------------

int vtkMRMLHierarchyNode::GetIndexInParent()
{
  vtkMRMLHierarchyNode *pnode = this->GetParentNode();
  if (pnode == NULL)
    {
    vtkErrorMacro("vtkMRMLHierarchyNode::GetIndexInParent() no parent");
    return -1;
    }
  else
    {
    std::vector< vtkMRMLHierarchyNode *> childrenNodes = pnode->GetChildrenNodes();
    for (unsigned int i=0; i<childrenNodes.size(); i++)
      {
      if (childrenNodes[i] == this)
        {
        return i;
        }
      }
    return -1;
    }
}

//----------------------------------------------------------------------------

void vtkMRMLHierarchyNode::SetIndexInParent(int index)
{
  vtkMRMLHierarchyNode *pnode = this->GetParentNode();
  if (pnode == NULL)
    {
    vtkErrorMacro("vtkMRMLHierarchyNode::SetIndexInParent() no parent");
    return;
    }
  else
    {
    std::vector< vtkMRMLHierarchyNode *> childrenNodes = pnode->GetChildrenNodes();
    if (index < 0 || index >= (int)childrenNodes.size())
      {
      vtkErrorMacro("vtkMRMLHierarchyNode::SetIndexInParent() index " << index << ", outside the range 0-" << childrenNodes.size()-1);
      return;
      }
    double sortValue = childrenNodes[index]->GetSortingValue();
    int oldIndex = this->GetIndexInParent();
    if (index == 0) 
      {
      sortValue -= 1;
      }
    else if (index == (int)childrenNodes.size()-1)
      {
      sortValue += 1;
      }
    else if (index > oldIndex)
      {
      sortValue = 0.5*(sortValue + childrenNodes[index+1]->GetSortingValue());
      }
    else if (index < oldIndex)
      {
      sortValue = 0.5*(sortValue + childrenNodes[index-1]->GetSortingValue());
      }
    this->SetSortingValue(sortValue);
    }

  return;
}

//----------------------------------------------------------------------------

void vtkMRMLHierarchyNode::RemoveHierarchyChildrenNodes()
{
  if (this->GetScene() == NULL)
    {
    return;
    }

  char *parentID = this->GetParentNodeID();
  vtkMRMLHierarchyNode *parentNode = this->GetParentNode();

  std::vector< vtkMRMLHierarchyNode *> children = this->GetChildrenNodes();
  for (unsigned int i=0; i<children.size(); i++)
    {
    vtkMRMLHierarchyNode *child = children[i];
    std::vector< vtkMRMLHierarchyNode *> childChildern = child->GetChildrenNodes();
    for (unsigned int j=0; i<childChildern.size(); j++)
      {
      childChildern[j]->SetParentNodeID(parentID);
      if (parentNode)
        {
        parentNode->InvokeEvent(vtkMRMLHierarchyNode::ChildNodeRemovedEvent);
        }
      }
    this->GetScene()->RemoveNode(child);
    }
}
//----------------------------------------------------------------------------

void vtkMRMLHierarchyNode::RemoveAllHierarchyChildrenNodes()
{
  if (this->GetScene() == NULL)
    {
    return;
    }

  std::vector< vtkMRMLHierarchyNode *> children = this->GetChildrenNodes();
  for (unsigned int i=0; i<children.size(); i++)
    {
    vtkMRMLHierarchyNode *child = children[i];
    std::vector< vtkMRMLHierarchyNode *> childChildern = child->GetChildrenNodes();
    for (unsigned int j=0; i<childChildern.size(); j++)
      {
      childChildern[j]->RemoveAllHierarchyChildrenNodes();
      }
    this->GetScene()->RemoveNode(child);
    }
}

//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::UpdateChildrenMap()
{
  if (this->GetScene() == NULL)
    {
    this->SceneHierarchyChildrenNodes.clear();
    this->SceneHierarchyChildrenNodesMTime.clear();
    return;
    }

  std::map< vtkMRMLScene*, HierarchyChildrenNodesType>::iterator siter = 
        SceneHierarchyChildrenNodes.find(this->GetScene());
  if (siter == SceneHierarchyChildrenNodes.end())
    {
    HierarchyChildrenNodesType h;
    SceneHierarchyChildrenNodes[this->GetScene()] = h;
    siter = SceneHierarchyChildrenNodes.find(this->GetScene());
    SceneHierarchyChildrenNodesMTime[this->GetScene()] = 0;
    }

  std::map< vtkMRMLScene*, unsigned long>::iterator titer = 
        SceneHierarchyChildrenNodesMTime.find(this->GetScene());

  std::map<std::string, std::vector< vtkMRMLHierarchyNode *> >::iterator iter;
  if (this->GetScene() == 0)
    {
    for (iter  = siter->second.begin();
         iter != siter->second.end();
         iter++)
      {
      iter->second.clear();
      }
    siter->second.clear();
    }

    
  if (this->GetScene()->GetSceneModifiedTime() > titer->second)
  {
    for (iter  = siter->second.begin();
         iter != siter->second.end();
         iter++)
      {
      iter->second.clear();
      }
    siter->second.clear();
    
    std::vector<vtkMRMLNode *> nodes;
    int nnodes = this->GetScene()->GetNodesByClass("vtkMRMLHierarchyNode", nodes);

    double maxSortingValue = this->MaximumSortingValue;

    for (int i=0; i<nnodes; i++)
      {
      vtkMRMLHierarchyNode *node =  vtkMRMLHierarchyNode::SafeDownCast(nodes[i]);
      if (node)
        {
        vtkMRMLHierarchyNode *pnode = vtkMRMLHierarchyNode::SafeDownCast(node->GetParentNode());
        if (pnode)
          {
          if (pnode->GetSortingValue() > maxSortingValue)
            {
            maxSortingValue = pnode->GetSortingValue();
            }
          iter = siter->second.find(std::string(pnode->GetID()));
          if (iter == siter->second.end())
            {
            std::vector< vtkMRMLHierarchyNode *> children;
            children.push_back(node);
            siter->second[std::string(pnode->GetID())] = children;
            }
          else
            {
            iter->second.push_back(node);
            }
          }
        }
      }
    titer->second = this->GetScene()->GetSceneModifiedTime();
    this->MaximumSortingValue = maxSortingValue;
  }
}

void vtkMRMLHierarchyNode::HierarchyIsModified(vtkMRMLScene *scene)
{
  if (scene == NULL)
    {
    return;
    }

  SceneHierarchyChildrenNodesMTime[scene] = 0;
}


void vtkMRMLHierarchyNode::GetAssociateChildrendNodes(vtkCollection *children, 
                                                      const char* childClass)
{
  if (children == NULL)
    {
    return;
    }
  vtkMRMLScene *scene = this->GetScene();
  if (scene == NULL)
    {
    //vtkErrorMacro("GetChildrenAssociatedNodes: scene is null, cannot find children of this node");
    return;
    }
  vtkMRMLNode *mnode = NULL;
  vtkMRMLHierarchyNode *hnode = NULL;
  std::string nodeClass("vtkMRMLNode");
  if (childClass)
    {
    nodeClass = childClass;
    }

  int numNodes = scene->GetNumberOfNodesByClass(nodeClass.c_str());
  for (int n=0; n < numNodes; n++) 
    {
    mnode = scene->GetNthNodeByClass(n, nodeClass.c_str());
    // check for a hierarchy node for this displayble node
    hnode = this->GetAssociatedHierarchyNode(this->GetScene(), mnode->GetID());
    while (hnode)
      {
      // hnode == this
      if (hnode->GetID() && this->GetID() &&
          strcmp(hnode->GetID(), this->GetID()) == 0) 
        {
        children->AddItem(mnode);
        break;
        }
      // the hierarchy node for this node may not be the one we're checking
      // against, go up the tree
      hnode = vtkMRMLHierarchyNode::SafeDownCast(hnode->GetParentNode());

      }// end while
    }// end for
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(vtkMRMLScene *scene,
                                                                       const char *associatedNodeID)
{
  if (associatedNodeID == 0)
    {
    std::cerr << "GetAssociatedHierarchyNode: associated node id is null" << std::endl;
    return NULL;
    }
  if (scene == 0)
    {
    std::cerr << "GetAssociatedHierarchyNode: scene is null" << std::endl;
    return NULL;
    }

  vtkMRMLHierarchyNode::UpdateAssociatedToHierarchyMap(scene);

  std::map< vtkMRMLScene*, AssociatedHierarchyNodesType>::iterator siter = 
        SceneAssociatedHierarchyNodes.find(scene);
  if (siter == SceneAssociatedHierarchyNodes.end())
    {
    std::cerr << "GetAssociatedHierarchyNode: didn't find an associated hierarchy node type associated with the scene" << std::endl;
    return NULL;
    }

  std::map<std::string, vtkMRMLHierarchyNode *>::iterator iter;
  
  iter = siter->second.find(associatedNodeID);
  if (iter != siter->second.end())
    {
    return iter->second;
    }
  else
    {
    return NULL;
    }
  
}

//----------------------------------------------------------------------------
int vtkMRMLHierarchyNode::UpdateAssociatedToHierarchyMap(vtkMRMLScene *scene)
{
  if (scene == 0)
    {
    SceneAssociatedHierarchyNodes.clear();
    SceneAssociatedHierarchyNodesMTime.clear();
    return 0;
    }

  std::map< vtkMRMLScene*, AssociatedHierarchyNodesType>::iterator siter = 
        SceneAssociatedHierarchyNodes.find(scene);
  if (siter == SceneAssociatedHierarchyNodes.end())
    {
    AssociatedHierarchyNodesType h;
    SceneAssociatedHierarchyNodes[scene] = h;
    siter = SceneAssociatedHierarchyNodes.find(scene);
    SceneAssociatedHierarchyNodesMTime[scene] = 0;
    }

  std::map< vtkMRMLScene*, unsigned long>::iterator titer = 
        SceneAssociatedHierarchyNodesMTime.find(scene);


  if (scene->GetSceneModifiedTime() > titer->second)
  {
    siter->second.clear();
    
    std::vector<vtkMRMLNode *> nodes;
    int nnodes = scene->GetNodesByClass("vtkMRMLHierarchyNode", nodes);
  
    for (int i=0; i<nnodes; i++)
      {
      vtkMRMLHierarchyNode *node =  vtkMRMLHierarchyNode::SafeDownCast(nodes[i]);
      if (node)
        {
        vtkMRMLNode *mnode = node->GetAssociatedNode();
        if (mnode)
          {
          siter->second[std::string(mnode->GetID())] = node;
          }
        }
      }
    titer->second = scene->GetSceneModifiedTime();
  }
  return static_cast<int>(siter->second.size());
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLHierarchyNode::GetAssociatedNode()
{
  vtkMRMLNode* node = NULL;
  if (this->GetScene() && this->GetAssociatedNodeID() )
    {
    node = this->GetScene()->GetNodeByID(this->AssociatedNodeIDReference);
    }
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::SetAssociatedNodeID(const char* ref) 
{
  if ((this->AssociatedNodeIDReference && ref && strcmp(ref, this->AssociatedNodeIDReference)) ||
      (this->AssociatedNodeIDReference != ref))
    {
    this->SetAssociatedNodeIDReference(ref);
    this->AssociatedHierarchyIsModified(this->GetScene());
    vtkMRMLNode* node = this->GetAssociatedNode();
    if (node)
      {
      node->Modified();
      }
    }
}
  
//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::AssociatedHierarchyIsModified(vtkMRMLScene *scene)
{
  if (scene == NULL)
    {
    return;
    }

  SceneAssociatedHierarchyNodesMTime[scene] = 0;
}

//----------------------------------------------------------------------------
void vtkMRMLHierarchyNode::SetSortingValue(double value)
{
  // reimplemented from vtkSet macro
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting SortingValue to " << value);
  if (this->SortingValue != value) 
    { 
    this->SortingValue = value; 
    this->Modified(); 

    // if there is an associated node, call modified on it to trigger updates in
    // q widgets
    vtkMRMLNode *associatedNode = this->GetAssociatedNode();
    if (associatedNode)
      {
      associatedNode->Modified();
      }
    }
}

// End
