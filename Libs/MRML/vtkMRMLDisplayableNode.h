/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLDisplayableNode.h,v $
  Date:      $Date: 2006/03/19 17:12:28 $
  Version:   $Revision: 1.6 $

=========================================================================auto=*/
///  vtkMRMLDisplayableNode - MRML node to represent a 3D surface model.
/// 
/// Model nodes describe polygonal data.  Models 
/// are assumed to have been constructed with the orientation and voxel 
/// dimensions of the original segmented volume.

#ifndef __vtkMRMLDisplayableNode_h
#define __vtkMRMLDisplayableNode_h

// MRML includes
#include "vtkMRMLStorableNode.h"
class vtkMRMLDisplayNode;

// VTK includes
class vtkPolyData;

// STD includes
#include <vector>

class VTK_MRML_EXPORT vtkMRMLDisplayableNode : public vtkMRMLStorableNode
{
public:
  static vtkMRMLDisplayableNode *New(){return NULL;};
  vtkTypeMacro(vtkMRMLDisplayableNode,vtkMRMLStorableNode);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  //--------------------------------------------------------------------------
  /// MRMLNode methods
  //--------------------------------------------------------------------------

  virtual vtkMRMLNode* CreateNodeInstance() = 0;

  virtual const char* GetNodeTagName() = 0;

  /// 
  /// Read node attributes from XML file
  virtual void ReadXMLAttributes( const char** atts);
  
  /// 
  /// Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  ///
  /// Write this node's information to a string for passing to a CLI, precede
  /// each datum with the prefix if not an empty string
  virtual void WriteCLI(std::ostringstream& vtkNotUsed(ss), std::string vtkNotUsed(prefix)) {};

  /// 
  /// Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  /// 
  /// Updates this node if it depends on other nodes 
  /// when the node is deleted in the scene
  virtual void UpdateReferences();

  /// 
  /// Clears out the list of display nodes, and updates them from teh lsit of
  /// display node ids
  virtual void UpdateScene(vtkMRMLScene *scene);

  /// 
  /// Update the stored reference to another node in the scene
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  /// 
  /// String ID of the display MRML node
  void SetAndObserveDisplayNodeID(const char *DisplayNodeID);
  void AddAndObserveDisplayNodeID(const char *DisplayNodeID);
  void SetAndObserveNthDisplayNodeID(int n, const char *DisplayNodeID);

  int GetNumberOfDisplayNodes()
    {
      return (int)this->DisplayNodeIDs.size();
    };

  const char *GetNthDisplayNodeID(int n)
  {
      if (n < 0 || n >= (int)this->DisplayNodeIDs.size())
      {
          return NULL;
      }
      return this->DisplayNodeIDs[n].c_str();
  };

  const char *GetDisplayNodeID()
    {
    return this->GetNthDisplayNodeID(0);
    };

  /// 
  /// Get associated display MRML node
  vtkMRMLDisplayNode* GetNthDisplayNode(int n);

  vtkMRMLDisplayNode* GetDisplayNode()
    {
    return this->GetNthDisplayNode(0);
    };
  std::vector<vtkMRMLDisplayNode*> GetDisplayNodes();
    
  /// 
  /// Set and observe poly data for this model
  vtkGetObjectMacro(PolyData, vtkPolyData);
  virtual void SetAndObservePolyData(vtkPolyData *PolyData);


  /// 
  /// alternative method to propagate events generated in Display nodes
  virtual void ProcessMRMLEvents ( vtkObject * /*caller*/, 
                                   unsigned long /*event*/, 
                                   void * /*callData*/ );
  
  /// DisplayModifiedEvent is generated when display node parameters is changed
  /// PolyDataModifiedEvent is generated when PloyData is changed
  enum
    {
      DisplayModifiedEvent = 17000,
      PolyDataModifiedEvent = 17001
    };

  ///
  /// Create and observe default display node(s)
  ///
  virtual void CreateDefaultDisplayNodes()
    {
      return;
    };
    
  /// Create default storage node or NULL if does not have one
  virtual vtkMRMLStorageNode* CreateDefaultStorageNode()
    {
    return Superclass::CreateDefaultStorageNode();
    };

  ///
  /// TODO: Change it to Get/SetVisibility() for consistency with
  /// vtkMRMLDisplayNode.
  /// Utility to return the visibility of all the display nodes.
  /// Return 0 if they are all hidden, 1 if all are visible and 2 if some are
  /// visible and some are hidden.
  virtual int GetDisplayVisibility();
  virtual void SetDisplayVisibility(int visible);

 protected:
  vtkMRMLDisplayableNode();
  ~vtkMRMLDisplayableNode();
  vtkMRMLDisplayableNode(const vtkMRMLDisplayableNode&);
  void operator=(const vtkMRMLDisplayableNode&);

  void SetDisplayNodeID(const char* id) ;
  void SetNthDisplayNodeID(int n, const char* id);
  void AddDisplayNodeID(const char* id);
  void AddAndObserveDisplayNode(vtkMRMLDisplayNode *dnode);

  virtual void SetPolyData(vtkPolyData* polyData);

  /// Data
  vtkPolyData *PolyData;

//BTX
  std::vector<std::string> DisplayNodeIDs;
 
  std::vector<vtkMRMLDisplayNode *> DisplayNodes;
//ETX
};

#endif
