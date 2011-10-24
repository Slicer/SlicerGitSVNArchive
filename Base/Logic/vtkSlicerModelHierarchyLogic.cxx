/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/


#include "vtkSlicerModelHierarchyLogic.h"

vtkCxxRevisionMacro(vtkSlicerModelHierarchyLogic, "$Revision$");
vtkStandardNewMacro(vtkSlicerModelHierarchyLogic);

//----------------------------------------------------------------------------
vtkSlicerModelHierarchyLogic::vtkSlicerModelHierarchyLogic()
{
  this->ModelHierarchyNodesMTime = 0;
  this->HierarchyChildrenNodesMTime = 0;
}

//----------------------------------------------------------------------------
vtkSlicerModelHierarchyLogic::~vtkSlicerModelHierarchyLogic()
{
}

//----------------------------------------------------------------------------
int vtkSlicerModelHierarchyLogic::UpdateModelToHierarchyMap()
{
  if (this->GetMRMLScene() == NULL)
    {
    this->ModelHierarchyNodes.clear();
    }
  else if (this->GetMRMLScene()->GetSceneModifiedTime() > this->ModelHierarchyNodesMTime)
  {
    this->ModelHierarchyNodes.clear();
    
    std::vector<vtkMRMLNode *> nodes;
    int nnodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLModelHierarchyNode", nodes);
  
    for (int i=0; i<nnodes; i++)
      {
      vtkMRMLModelHierarchyNode *node =  vtkMRMLModelHierarchyNode::SafeDownCast(nodes[i]);
      if (node)
        {
        vtkMRMLModelNode *mnode = node->GetModelNode();
        if (mnode)
          {
          this->ModelHierarchyNodes[std::string(mnode->GetID())] = node;
          }
        }
      }
    this->ModelHierarchyNodesMTime = this->GetMRMLScene()->GetSceneModifiedTime();
  }
  return (int)ModelHierarchyNodes.size();
}

//---------------------------------------------------------------------------
vtkMRMLModelHierarchyNode* vtkSlicerModelHierarchyLogic::GetModelHierarchyNode(const char *modelNodeID)
{
  if (modelNodeID == NULL)
    {
    return NULL;
    }
  this->UpdateModelToHierarchyMap();
  
  std::map<std::string, vtkMRMLModelHierarchyNode *>::iterator iter;
  
  iter = this->ModelHierarchyNodes.find(modelNodeID);
  if (iter != this->ModelHierarchyNodes.end())
    {
    return iter->second;
    }
  else
    {
    return NULL;
    }
  
}
//----------------------------------------------------------------------------
void vtkSlicerModelHierarchyLogic::GetHierarchyChildrenNodes(vtkMRMLModelHierarchyNode *parentNode,
                                                             std::vector< vtkMRMLModelHierarchyNode *> &childrenNodes)
{
  this->UpdateHierarchyChildrenMap();
  
  std::map<std::string, std::vector< vtkMRMLModelHierarchyNode *> >::iterator iter;
  std::map<std::string, std::vector< vtkMRMLModelHierarchyNode *> > hierarchyChildrenNodes = this->HierarchyChildrenNodes;
  iter = hierarchyChildrenNodes.find(std::string(parentNode->GetID()));
  if (iter != hierarchyChildrenNodes.end()) 
    {
    for (unsigned int i=0; i<iter->second.size(); i++)
      {
      childrenNodes.push_back(iter->second[i]);
      this->GetHierarchyChildrenNodes(iter->second[i], childrenNodes);
      }
    }
}


//----------------------------------------------------------------------------
void vtkSlicerModelHierarchyLogic::UpdateHierarchyChildrenMap()
{
  std::map<std::string, std::vector< vtkMRMLModelHierarchyNode *> >::iterator iter;
  if (this->GetMRMLScene() == NULL)
    {
    for (iter  = this->HierarchyChildrenNodes.begin();
         iter != this->HierarchyChildrenNodes.end();
         iter++)
      {
      iter->second.clear();
      }
    this->HierarchyChildrenNodes.clear();
    }
    
  if (this->GetMRMLScene()->GetSceneModifiedTime() > this->HierarchyChildrenNodesMTime)
  {
    for (iter  = this->HierarchyChildrenNodes.begin();
         iter != this->HierarchyChildrenNodes.end();
         iter++)
      {
      iter->second.clear();
      }
    this->HierarchyChildrenNodes.clear();
    
    std::vector<vtkMRMLNode *> nodes;
    int nnodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLModelHierarchyNode", nodes);
  
    for (int i=0; i<nnodes; i++)
      {
      vtkMRMLModelHierarchyNode *node =  vtkMRMLModelHierarchyNode::SafeDownCast(nodes[i]);
      if (node)
        {
        vtkMRMLModelHierarchyNode *pnode = vtkMRMLModelHierarchyNode::SafeDownCast(node->GetParentNode());
        if (pnode)
          {
          iter = this->HierarchyChildrenNodes.find(std::string(pnode->GetID()));
          if (iter == this->HierarchyChildrenNodes.end())
            {
            std::vector< vtkMRMLModelHierarchyNode *> children;
            children.push_back(node);
            this->HierarchyChildrenNodes[std::string(pnode->GetID())] = children;
            }
          else
            {
            iter->second.push_back(node);
            }
          }
        }
      }
    this->HierarchyChildrenNodesMTime = this->GetMRMLScene()->GetSceneModifiedTime();
  }
}
