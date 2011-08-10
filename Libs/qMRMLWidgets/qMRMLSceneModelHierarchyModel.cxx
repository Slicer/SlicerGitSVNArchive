/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes

// qMRML includes
#include "qMRMLSceneModelHierarchyModel.h"
#include "qMRMLSceneDisplayableModel_p.h"

// MRMLLogic includes
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLModelHierarchyLogic.h>

// MRML includes

// VTK includes

//------------------------------------------------------------------------------
class qMRMLSceneModelHierarchyModelPrivate: public qMRMLSceneDisplayableModelPrivate
{
protected:
  Q_DECLARE_PUBLIC(qMRMLSceneModelHierarchyModel);
public:
  typedef qMRMLSceneDisplayableModelPrivate Superclass;
  qMRMLSceneModelHierarchyModelPrivate(qMRMLSceneModelHierarchyModel& object);

  virtual vtkMRMLHierarchyNode* CreateHierarchyNode()const;

  vtkSmartPointer<vtkMRMLModelHierarchyLogic> ModelLogic;

};

//------------------------------------------------------------------------------
qMRMLSceneModelHierarchyModelPrivate
::qMRMLSceneModelHierarchyModelPrivate(qMRMLSceneModelHierarchyModel& object)
  : Superclass(object)
{
  this->ModelLogic = vtkSmartPointer<vtkMRMLModelHierarchyLogic>::New();
}

//------------------------------------------------------------------------------
vtkMRMLHierarchyNode* qMRMLSceneModelHierarchyModelPrivate::CreateHierarchyNode()const
{
  return vtkMRMLModelHierarchyNode::New();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
qMRMLSceneModelHierarchyModel::qMRMLSceneModelHierarchyModel(QObject *vparent)
  :Superclass(new qMRMLSceneModelHierarchyModelPrivate(*this), vparent)
{
  Q_D(qMRMLSceneModelHierarchyModel);
  d->init();
}

//------------------------------------------------------------------------------
qMRMLSceneModelHierarchyModel::~qMRMLSceneModelHierarchyModel()
{
}

//------------------------------------------------------------------------------
void qMRMLSceneModelHierarchyModel::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qMRMLSceneModelHierarchyModel);
  d->ModelLogic->SetMRMLScene(scene);
  this->qMRMLSceneModel::setMRMLScene(scene);
}

//------------------------------------------------------------------------------
vtkMRMLNode* qMRMLSceneModelHierarchyModel::parentNode(vtkMRMLNode* node)const
{
  return vtkMRMLModelHierarchyNode::SafeDownCast(
    this->Superclass::parentNode(node));
}
/*
//------------------------------------------------------------------------------
int qMRMLSceneModelHierarchyModel::nodeIndex(vtkMRMLNode* node)const
{
  const char* nodeId = node ? node->GetID() : 0;
  if (nodeId == 0)
    {
    return -1;
    }
  int index = 0;
  vtkMRMLModelHierarchyNode* parent = 
    vtkMRMLModelHierarchyNode::SafeDownCast(this->parentNode(node));
  if (!parent)
    {
    vtkCollection* sceneCollection = node->GetScene()->GetCurrentScene();
    vtkMRMLNode* n = 0;
    vtkCollectionSimpleIterator it;
    for (sceneCollection->InitTraversal(it);
         (n = static_cast<vtkMRMLNode*>(sceneCollection->GetNextItemAsObject(it))) ;)
      {
      // note: parent can be NULL, it means that the scene is the parent
      if (parent == this->parentNode(n))
        {
        const char* nId = n->GetID();
        if (nId && !strcmp(nodeId, nId))
          {
          return index;
          }
        vtkMRMLModelHierarchyNode* hierarchy = vtkMRMLModelHierarchyNode::SafeDownCast(n);
        index += (hierarchy && hierarchy->GetModelNodeID() != 0 ? 2 : 1);
        }
      }
    }
  else
    {
    vtkMRMLModelNode* mnode = vtkMRMLModelNode::SafeDownCast(node);
    vtkMRMLModelHierarchyNode* hnode = vtkMRMLModelHierarchyNode::SafeDownCast(node);
    //vtkMRMLModelHierarchyNodeList children = d->ModelLogic->GetHierarchyChildrenNodes(parent);
    std::vector< vtkMRMLHierarchyNode *> children = parent->GetChildrenNodes();


    for(std::vector< vtkMRMLHierarchyNode *>::const_iterator it = children.begin();
      it !=children.end(); ++it)
      {
        vtkMRMLModelHierarchyNode* hierarchy = vtkMRMLModelHierarchyNode::SafeDownCast(*it);
      if (mnode)
        {
        const char* nId = hierarchy->GetModelNodeID();
        if (nId && !strcmp(nodeId, nId))
          {
          return index;
          }
        }
      else if (hnode)
        {
        if (hierarchy == node)
          {
          return index;
          }
        }
      index += (hierarchy->GetModelNodeID() != 0 ? 2 : 1);
      }
    }
  return -1;
}
*/
//------------------------------------------------------------------------------
bool qMRMLSceneModelHierarchyModel::canBeAChild(vtkMRMLNode* node)const
{
  if (!node)
    {
    return false;
    }
  return node->IsA("vtkMRMLModelHierarchyNode") || node->IsA("vtkMRMLModelNode");
}

//------------------------------------------------------------------------------
bool qMRMLSceneModelHierarchyModel::canBeAParent(vtkMRMLNode* node)const
{
  vtkMRMLModelHierarchyNode* hnode = vtkMRMLModelHierarchyNode::SafeDownCast(node);
  if (hnode && hnode->GetModelNodeID() == 0)
    {
    return true;
    }
  return false;
}
/*
//------------------------------------------------------------------------------
bool qMRMLSceneModelHierarchyModel::reparent(vtkMRMLNode* node, vtkMRMLNode* newParent)
{
  Q_D(qMRMLSceneModelHierarchyModel);
  Q_ASSERT(node);
  vtkMRMLModelHierarchyNode* modelParent =
    vtkMRMLModelHierarchyNode::SafeDownCast(this->parentNode(node));
  vtkMRMLModelHierarchyNode* newModelParent =
    vtkMRMLModelHierarchyNode::SafeDownCast(newParent);    
  if (!node || modelParent == newParent)
    {
    return false;
    }
  Q_ASSERT(newParent != node);
  vtkMRMLModelNode *mnode = vtkMRMLModelNode::SafeDownCast(node);
  vtkMRMLModelHierarchyNode *hnode = vtkMRMLModelHierarchyNode::SafeDownCast(node);
    
  if (hnode)
    {
    hnode->SetParentNodeID(newModelParent ? newModelParent->GetID() : 0);
    vtkMRMLModelNode* model = hnode ? hnode->GetModelNode() : 0;
    if (model)
      {
      model->Modified();
      }
    return true;
    }
  Q_ASSERT(mnode);

  vtkMRMLModelHierarchyNode* hierarchyNode = vtkMRMLModelHierarchyNode::SafeDownCast(
    vtkMRMLDisplayableHierarchyNode::GetDisplayableHierarchyNode(node->GetScene(), node->GetID()));
  if (newModelParent == 0)
    {
    if (hierarchyNode)
      {
      vtkMRMLDisplayNode *dnode = hierarchyNode->GetDisplayNode();
      if (dnode)
        {
        this->mrmlScene()->RemoveNode(dnode);
        }
      this->mrmlScene()->RemoveNode(hierarchyNode);
      }
    }
  else 
    {
    if (!hierarchyNode)
      {
      hierarchyNode = vtkMRMLModelHierarchyNode::New();
      hierarchyNode->SetName(this->mrmlScene()->GetUniqueNameByString(
        d->MRMLScene->GetTagByClassName("vtkMRMLModelHierarchyNode")));
      hierarchyNode->SetSelectable(0);
      hierarchyNode->SetHideFromEditors(1);
      hierarchyNode->SetModelNodeID(mnode->GetID());
      this->mrmlScene()->AddNode(hierarchyNode);
      hierarchyNode->Delete();
      }
    //vtkMRMLModelHierarchyNode *oldParentNode = vtkMRMLModelHierarchyNode::SafeDownCast(modelParent->GetParentNode());
    //if (oldParentNode)
    //  {
    //  oldParentNode->SetModelNodeID(NULL);
    //  }
    hierarchyNode->SetParentNodeID(newModelParent->GetID());
    if (mnode)
      {
      mnode->Modified();
      }
    }
  return true;
}
*/
