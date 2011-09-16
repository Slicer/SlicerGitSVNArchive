#ifndef __vtkMRMLEMSNode_h
#define __vtkMRMLEMSNode_h

#include "vtkMRML.h"
#include "vtkMRMLNode.h"

#include "vtkEMSegment.h"
#include "vtkMRMLScene.h"

class vtkMRMLEMSSegmenterNode;

//
// LEGACY CODE - please do not add any new functions  
//


// Description: 
// The scripted module node is simply a MRMLNode container for 
// an arbitrary keyword value pair map

class VTK_EMSEGMENT_EXPORT vtkMRMLEMSNode : 
  public vtkMRMLNode
{
public:
  static vtkMRMLEMSNode *New();
  vtkTypeMacro(vtkMRMLEMSNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual vtkMRMLNode* CreateNodeInstance();

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts);

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent);

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node);

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName()
    {return "EMS";}

  // Description:
  // Updates this node if it depends on other nodes
  // when the node is deleted in the scene
  virtual void UpdateReferences();

  // Description:
  // Update the stored reference to another node in the scene
  virtual void UpdateReferenceID(const char *oldID, const char *newID);
  
  vtkGetStringMacro (SegmenterNodeID);
  vtkMRMLEMSSegmenterNode* GetSegmenterNode();

  vtkGetStringMacro(TemplateFilename);

  vtkGetMacro(SaveTemplateAfterSegmentation, int);

  vtkGetStringMacro(TclTaskFilename);

  vtkGetStringMacro(TaskPreprocessingSetting);

  static const char* GetDefaultTclTaskFilename() { return "GenericTask.tcl"; }

protected:
  char *SegmenterNodeID;

  int   SaveTemplateAfterSegmentation;

  char* TemplateFilename;

  char* TclTaskFilename;

  char* TaskPreprocessingSetting; 

  vtkSetStringMacro (SegmenterNodeID);
  vtkSetStringMacro(TemplateFilename);
  vtkSetMacro(SaveTemplateAfterSegmentation, int);
  vtkSetStringMacro(TclTaskFilename);
  vtkSetStringMacro(TaskPreprocessingSetting);


private:
  vtkMRMLEMSNode();
  ~vtkMRMLEMSNode();
  vtkMRMLEMSNode(const vtkMRMLEMSNode&);
  void operator=(const vtkMRMLEMSNode&);



};

#endif
