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

#ifndef __qMRMLScenePatientHierarchyModel_h
#define __qMRMLScenePatientHierarchyModel_h

// MRMLWidgets includes
#include "qMRMLSceneHierarchyModel.h"
class qMRMLScenePatientHierarchyModelPrivate;

class QMRML_WIDGETS_EXPORT qMRMLScenePatientHierarchyModel : public qMRMLSceneHierarchyModel
{
  Q_OBJECT

public:
  typedef qMRMLSceneHierarchyModel Superclass;
  qMRMLScenePatientHierarchyModel(QObject *parent=0);
  virtual ~qMRMLScenePatientHierarchyModel();

  ///
  virtual vtkMRMLNode* parentNode(vtkMRMLNode* node)const;

  //virtual int          nodeIndex(vtkMRMLNode* node)const;

  /// Fast function that only check the type of the node to know if it can be a child.
  virtual bool         canBeAChild(vtkMRMLNode* node)const;

  /// Fast function that only check the type of the node to know if it can be a parent.
  virtual bool         canBeAParent(vtkMRMLNode* node)const;

  /// If newParent == 0, set the node into the vtkMRMLScene
 // virtual bool         reparent(vtkMRMLNode* node, vtkMRMLNode* newParent);

private:
  Q_DECLARE_PRIVATE(qMRMLScenePatientHierarchyModel);
  Q_DISABLE_COPY(qMRMLScenePatientHierarchyModel);
};

#endif
