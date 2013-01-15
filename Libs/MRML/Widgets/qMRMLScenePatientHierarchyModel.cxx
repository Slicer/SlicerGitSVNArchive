/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Qt includes

// qMRML includes
#include "qMRMLScenePatientHierarchyModel.h"
#include "qMRMLSceneHierarchyModel_p.h"

// MRMLLogic includes
#include <vtkMRMLPatientHierarchyNode.h>

// MRML includes

// VTK includes

//------------------------------------------------------------------------------
class qMRMLScenePatientHierarchyModelPrivate: public qMRMLSceneHierarchyModelPrivate
{
protected:
  Q_DECLARE_PUBLIC(qMRMLScenePatientHierarchyModel);
public:
  qMRMLScenePatientHierarchyModelPrivate(qMRMLScenePatientHierarchyModel& object);

};

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModelPrivate
::qMRMLScenePatientHierarchyModelPrivate(qMRMLScenePatientHierarchyModel& object)
: qMRMLSceneHierarchyModelPrivate(object)
{
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModel::qMRMLScenePatientHierarchyModel(QObject *vparent)
:qMRMLSceneHierarchyModel(new qMRMLScenePatientHierarchyModelPrivate(*this), vparent)
{
}

//------------------------------------------------------------------------------
qMRMLScenePatientHierarchyModel::~qMRMLScenePatientHierarchyModel()
{
}

//------------------------------------------------------------------------------
vtkMRMLNode* qMRMLScenePatientHierarchyModel::parentNode(vtkMRMLNode* node)const
{
  return vtkMRMLPatientHierarchyNode::SafeDownCast(
    this->Superclass::parentNode(node));
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::canBeAChild(vtkMRMLNode* node)const
{
  if (!node)
    {
    return false;
    }
  return node->IsA("vtkMRMLPatientHierarchyNode")/* || node->IsA("vtkMRMLNode")*/;
}

//------------------------------------------------------------------------------
bool qMRMLScenePatientHierarchyModel::canBeAParent(vtkMRMLNode* node)const
{
  vtkMRMLPatientHierarchyNode* hnode = vtkMRMLPatientHierarchyNode::SafeDownCast(node);
  if (hnode/* && hnode->GetAssociatedNodeID() == 0*/)
    {
    return true;
    }
  return false;
}
