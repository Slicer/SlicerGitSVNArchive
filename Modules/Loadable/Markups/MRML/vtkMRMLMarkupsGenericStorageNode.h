/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Johan Andruejol, Kitware Inc.
  and was partially funded by Allen Institute.

==============================================================================*/

/// Markups Module MRML storage nodes
///
/// vtkMRMLMarkupsGenericStorageNode - MRML node for markups storage
///
/// vtkMRMLMarkupsGenericStorageNode nodes describe the markups storage
/// node that allows to read/write fiducial point data from/to JSON.

#ifndef __vtkMRMLMarkupsGenericStorageNode_h
#define __vtkMRMLMarkupsGenericStorageNode_h

// Markups includes
#include "vtkSlicerMarkupsModuleMRMLExport.h"
#include "vtkMRMLMarkupsStorageNode.h"

class vtkMRMLMarkupsNode;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_MARKUPS_MODULE_MRML_EXPORT vtkMRMLMarkupsGenericStorageNode
  : public vtkMRMLMarkupsStorageNode
{
public:
  static vtkMRMLMarkupsGenericStorageNode *New();
  vtkTypeMacro(vtkMRMLMarkupsGenericStorageNode,vtkMRMLMarkupsStorageNode);

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  ///
  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "MarkupGenericStorage";};

  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode) VTK_OVERRIDE;

protected:
  vtkMRMLMarkupsGenericStorageNode();
  ~vtkMRMLMarkupsGenericStorageNode();
  vtkMRMLMarkupsGenericStorageNode(const vtkMRMLMarkupsGenericStorageNode&);
  void operator=(const vtkMRMLMarkupsGenericStorageNode&);

  /// Initialize all the supported write file types
  virtual void InitializeSupportedReadFileTypes() VTK_OVERRIDE;

  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes() VTK_OVERRIDE;

  /// Read data and set it in the referenced node
  virtual int ReadDataInternal(vtkMRMLNode *refNode) VTK_OVERRIDE;

  /// Write data from a  referenced node.
  virtual int WriteDataInternal(vtkMRMLNode *refNode) VTK_OVERRIDE;

  /// The translation map is used read from/to JSON and filled
  /// with the markup properties.
  /// The keys follow the RapidJSON pointer usage
  /// (http://rapidjson.org/md_doc_pointer.html#BasicUsage)
  typedef std::map<std::string, vtkVariant> TranslationMap;

  /// Use Read/WriteMarkupsNodeFromTranslationMap to add properties that will
  /// be read/written for the node. For example, the MarkupLabelFormat is
  /// added here with the key "/MarkupLabelFormat".
  /// For lists, it is also expected to add the property *_Count that contains
  /// the size of the list.
  /// If the list is empty, you must add a empty vtkVariant() to it. Use a
  /// key that follows this format "/MyListKeyName/0".
  /// \sa TranslationMap()
  virtual int ReadMarkupsNodeFromTranslationMap(
    vtkMRMLMarkupsNode* markups, TranslationMap& markupsMap);

  /// Use Read/WriteNthMarkupFromTranslationMap to add properties that will
  /// be read/written for each of the node's markups. For example, the
  /// OrientationWXYZ property of each markup is added here.
  /// \sa TranslationMap()
  /// \sa ReadMarkupsNodeFromTranslationMap()
  virtual int ReadNthMarkupFromTranslationMap(
    int n, std::string key,
    vtkMRMLMarkupsNode* markups, TranslationMap& markupsMap);

  /// \sa ReadMarkupsNodeFromTranslationMap()
  virtual int WriteMarkupsNodeToTranslationMap(
    vtkMRMLMarkupsNode* markups, TranslationMap& markupsMap);

  /// \sa ReadNthMarkupFromTranslationMap()
  virtual int WriteNthMarkupToTranslationMap(
    int n, std::string key,
    vtkMRMLMarkupsNode* markups, TranslationMap& markupsMap);

  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal;
};

#endif
