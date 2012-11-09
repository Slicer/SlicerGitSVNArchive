/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLSceneViewNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.13 $

=========================================================================auto=*/

#ifndef __vtkMRMLSceneViewNode_h
#define __vtkMRMLSceneViewNode_h

#include "vtkMRMLStorableNode.h"

// VTK includes
#include <vtkStdString.h>
class vtkImageData;

class vtkMRMLStorageNode;
class VTK_MRML_EXPORT vtkMRMLSceneViewNode : public vtkMRMLStorableNode
{
  public:
  static vtkMRMLSceneViewNode *New();
  vtkTypeMacro(vtkMRMLSceneViewNode,vtkMRMLStorableNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// 
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);

  /// 
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  /// 
  /// Write this node's body to a MRML file in XML format.
  virtual void WriteNodeBodyXML(ostream& of, int indent);

  /// 
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// 
  /// Get node XML tag name (like Volume, Model)
//  virtual const char* GetNodeTagName() {return "SceneSnapshot";};
  virtual const char* GetNodeTagName() {return "SceneView";};

  /// 
  /// Updates scene nodes 
  virtual void UpdateScene(vtkMRMLScene *scene);

  /// 
  /// Updates internal nodes 
  virtual void UpdateSnapshotScene(vtkMRMLScene *);

  /// 
  /// Set dependencies between this node and a child node
  /// when parsing XML file
  virtual void ProcessChildNode(vtkMRMLNode *node);

  /// 
  /// Store content of the scene
  void StoreScene();

  /// 
  /// Restore content of the scene from the node
  void RestoreScene();

  vtkGetObjectMacro ( Nodes, vtkMRMLScene );

  void SetAbsentStorageFileNames();

  /// A description of this sceneView
  void SetSceneViewDescription(const vtkStdString& newDescription);
  vtkGetMacro(SceneViewDescription, vtkStdString);

  /// The attached screenshot of this sceneView
  virtual void SetScreenShot(vtkImageData* newScreenShot);
  vtkGetObjectMacro(ScreenShot, vtkImageData);

  /// The screenshot type of this sceneView
  /// 0: 3D View
  /// 1: Red Slice View
  /// 2: Yellow Slice View
  /// 3: Green Slice View
  /// 4: Full layout
  // TODO use an enum for the types
  virtual void SetScreenShotType(int type);
  vtkGetMacro(ScreenShotType, int);


  /// 
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode();

 /// Get vector of nodes of a specified class in the scene
  int GetNodesByClass(const char *className, std::vector<vtkMRMLNode *> &nodes);

  /// check if a node should be included in the save/restore cycle. Returns
  /// false if it's a scene view node, scene view storage node, scene view
  /// hierarchy node, snapshot clip node, true otherwise
  bool IncludeNodeInSceneView(vtkMRMLNode *node);

  void SetSceneViewRootDir( const char* name);

protected:
  vtkMRMLSceneViewNode();
  ~vtkMRMLSceneViewNode();
  vtkMRMLSceneViewNode(const vtkMRMLSceneViewNode&);
  void operator=(const vtkMRMLSceneViewNode&);


  vtkMRMLScene* Nodes;

  /// The associated Description
  vtkStdString SceneViewDescription;

  /// The vtkImageData of the screenshot
  vtkImageData* ScreenShot;

  /// The type of the screenshot
  int ScreenShotType;

};

#endif
