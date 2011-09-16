#ifndef __vtkMRMLEMSSegmenterNode_h
#define __vtkMRMLEMSSegmenterNode_h

#include "vtkMRML.h"
#include "vtkMRMLNode.h"
#include "vtkEMSegment.h"
class vtkMRMLEMSTemplateNode;
class vtkMRMLEMSWorkingDataNode;
class vtkMRMLScalarVolumeNode;


//
// LEGACY CODE - please do not add any new functions 
//
class VTK_EMSEGMENT_EXPORT vtkMRMLEMSSegmenterNode : 
  public vtkMRMLNode
{
public:
  static vtkMRMLEMSSegmenterNode *New();
  vtkTypeMacro(vtkMRMLEMSSegmenterNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes(const char** atts);

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() {return "EMSSegmenter";}

  // Description:
  // Updates this node if it depends on other nodes
  // when the node is deleted in the scene
  virtual void UpdateReferences();

  // Description:
  // Update the stored reference to another node in the scene
  virtual void UpdateReferenceID(const char *oldID, const char *newID);

  // associated nodes
  vtkGetStringMacro(TemplateNodeID);
  vtkMRMLEMSTemplateNode* GetTemplateNode();

  vtkGetStringMacro(OutputVolumeNodeID);
  vtkMRMLScalarVolumeNode* GetOutputVolumeNode();

  vtkGetStringMacro         (WorkingDataNodeID);
  vtkMRMLEMSWorkingDataNode* GetWorkingDataNode();

  vtkGetStringMacro(WorkingDirectory);

protected:
  vtkMRMLEMSSegmenterNode();
  ~vtkMRMLEMSSegmenterNode();
  vtkMRMLEMSSegmenterNode(const vtkMRMLEMSSegmenterNode&);
  void operator=(const vtkMRMLEMSSegmenterNode&);

  char*                               TemplateNodeID;
  char*                               OutputVolumeNodeID;
  char*                               WorkingDataNodeID;

  char*                               WorkingDirectory;

  vtkSetStringMacro(TemplateNodeID);
  vtkSetStringMacro(OutputVolumeNodeID);
  vtkSetStringMacro(WorkingDataNodeID);
  vtkSetStringMacro(WorkingDirectory);
};

#endif
