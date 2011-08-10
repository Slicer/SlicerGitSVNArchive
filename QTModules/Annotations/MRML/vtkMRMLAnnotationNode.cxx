// MRML includes
#include "vtkMRMLAnnotationNode.h"
#include "vtkMRMLAnnotationStorageNode.h"
#include "vtkMRMLAnnotationTextDisplayNode.h"
#include "vtkMRMLCameraNode.h"
#include "vtkMRMLSliceNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkCommand.h>
#include <vtkBitArray.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkStringArray.h>

// STD includes
#include <sstream>

// KPs Todos 
// - create specific event for node modification
// - talk to Steve if we have to do anything when UpdatingScene 
// - NumberingScheme should not be in annotation node - should be in fiducial nodes - just put it here right now 


//------------------------------------------------------------------------------
vtkMRMLAnnotationNode* vtkMRMLAnnotationNode::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLAnnotationNode");
  if(ret)
    {
    return (vtkMRMLAnnotationNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLAnnotationNode;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLAnnotationNode::CreateNodeInstance()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkMRMLAnnotationNode");
  if(ret)
    {
    return (vtkMRMLAnnotationNode*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkMRMLAnnotationNode;
}


//----------------------------------------------------------------------------
vtkMRMLAnnotationNode::vtkMRMLAnnotationNode()
{
  this->TextList = vtkStringArray::New();
  this->ReferenceNodeID = NULL;
  this->Visible=1;  
  this->Locked = 0;
  this->m_Backup = 0;

  this->m_RedSliceNode = 0;
  this->m_YellowSliceNode = 0;
  this->m_GreenSliceNode = 0;
  this->m_CameraNode = 0;
}

//----------------------------------------------------------------------------
vtkMRMLAnnotationNode::~vtkMRMLAnnotationNode()
{
  this->TextList->Delete();
  if (this->ReferenceNodeID) 
    {
      delete [] this->ReferenceNodeID;
    }

  if (this->m_RedSliceNode)
    {
    this->m_RedSliceNode->Delete();
    this->m_RedSliceNode = 0;
    }

  if (this->m_YellowSliceNode)
    {
    this->m_YellowSliceNode->Delete();
    this->m_YellowSliceNode = 0;
    }

  if (this->m_GreenSliceNode)
    {
    this->m_GreenSliceNode->Delete();
    this->m_GreenSliceNode = 0;
    }

  if (this->m_CameraNode)
    {
    this->m_CameraNode->Delete();
    this->m_CameraNode = 0;
    }

  if (this->m_Backup)
    {
    this->m_Backup->Delete();
    this->m_Backup = 0;
    }
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationNode::WriteXML(ostream& of, int nIndent)
{
  // cout << "vtkMRMLAnnotationNode::WriteXML(ostream& of, int nIndent) start" << endl;
  // vtkMRMLDisplayableNode::WriteXML(of,nIndent);
  Superclass::WriteXML(of,nIndent);
 
  vtkIndent indent(nIndent);
 
  of << indent << " referenceNodeID=\"" << (this->ReferenceNodeID ? this->GetReferenceNodeID() : "None") << "\"";
  of << indent << " visible=\"" << this->Visible << "\"";
  of << indent << " locked=\"" << this->Locked << "\"";
   
  int textLength = this->TextList->GetNumberOfValues();
  of << indent << " textList=\"";

  if (textLength)
    {
      for (int i = 0 ; i < textLength - 1; i++) {
    of << this->TextList->GetValue(i) << "|"; 
      }
      of <<  this->TextList->GetValue(textLength -1);
    }
  of << "\"";
 
  for (int j = 0 ; j < NUM_TEXT_ATTRIBUTE_TYPES; j ++) 
    {
      of << indent << " " << this->GetAttributeTypesEnumAsString(j) << "=\"";
      if (textLength && this->PolyData && this->PolyData->GetPointData()) 
    {
      for (int i = 0 ; i < textLength - 1; i++) 
        {
          of << this->GetAnnotationAttribute(i,j) << " " ;
        }
      of << this->GetAnnotationAttribute(textLength - 1,j);
    }    
      of << "\"";
    }
}


//----------------------------------------------------------------------------
void vtkMRMLAnnotationNode::ReadXMLAttributes(const char** atts)
{
  // cout << "vtkMRMLAnnotationNode::ReadXMLAttributes start"<< endl;

  int disabledModify = this->StartModify();
  this->ResetAnnotations();

  // vtkMRMLDisplayableNode::ReadXMLAttributes(atts);
  Superclass::ReadXMLAttributes(atts);
  
  while (*atts != NULL) 
    {
    const char* attName = *(atts++);
    std::string attValue(*(atts++));

    if (!strcmp(attName, "textList") && attValue.size())
      {
    std::string tmpStr;
    size_t  startPos = 0;
    size_t  endPos =attValue.find("|",startPos);
    while (endPos != std::string::npos) {
      if (endPos == startPos) this->AddText(0,1,1);
      else {
        this->AddText(attValue.substr(startPos,endPos-startPos).c_str(),1,1);
      }
      startPos = endPos +1;
      endPos =attValue.find("|",startPos);
    }
      this->AddText(attValue.substr(startPos,endPos).c_str(),1,1);
      }  
    else if (!strcmp(attName, "referenceNodeID"))
      {
      this->SetReferenceNodeID(attValue.c_str());
      }
    else if (!strcmp(attName, "visible"))
      {
      this->SetVisible(atof(attValue.c_str()));
      }
    // for backwards compatibility
    else if (!strcmp(attName, "visibility"))
      {
      if (!strcmp(attValue.c_str(),"true"))
        {
        // visiblity = true
        this->SetVisible(1);
        }
      else
        {
        // visibility = false
        this->SetVisible(0);
        }
      }
    // end of backwards compatibility
    else if (!strcmp(attName, "locked"))
      {
      this->SetLocked(atof(attValue.c_str()));
      }
    else 
      {
      int j = 0;
      while (j < NUM_TEXT_ATTRIBUTE_TYPES)
        {
          if (!strcmp(attName, this->GetAttributeTypesEnumAsString(j)))
            {
          std::stringstream ss;
          ss << attValue;
          double value;
          vtkIdType id = 0;
          while (ss >> value)
            {
              this->SetAnnotationAttribute(id,j, value);
              id ++;
            }
          j = NUM_TEXT_ATTRIBUTE_TYPES;
            }
          j++;
        }
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);

  vtkMRMLAnnotationNode *node = (vtkMRMLAnnotationNode *) anode;
  if (!node) return; 
  
  this->SetReferenceNodeID(node->GetReferenceNodeID());
  this->SetVisible(node->GetVisible());
  this->SetLocked(node->GetLocked());
  this->TextList->DeepCopy(node->TextList);
}


//-----------------------------------------------------------
void vtkMRMLAnnotationNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);

  // Nothing to do at this point  bc vtkMRMLAnnotationTextDisplayNode is subclass of vtkMRMLModelDisplayNode 
  // => will be taken care of by vtkMRMLModelDisplayNode  

}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event, 
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);

  // Not necessary bc vtkMRMLAnnotationTextDisplayNode is subclass of vtkMRMLModelDisplayNode 
  // => will be taken care of  in vtkMRMLModelNode
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << endl;
  this->PrintAnnotationInfo(os,indent,0);
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationNode::PrintAnnotationInfo(ostream& os, vtkIndent indent, int titleFlag)
{
  if (titleFlag) 
    {
      
    os <<indent << "vtkMRMLAnnotationNode: Annotation Summary";
    if (this->GetName()) 
      {
      os << " of " << this->GetName();
      }
    os << endl;
    }


  os << indent << "ReferenceNodeID: " << ( (this->ReferenceNodeID) ? this->ReferenceNodeID : "None" ) << "\n";
  os << indent << "Selected: " << this->Selected << "\n";
  os << indent << "Visible: " << this->Visible << "\n";
  os << indent << "Locked: " << this->Locked << "\n";
  os << indent << "textList: "; 
  if  (!this->TextList || !this->GetNumberOfTexts()) 
    {
    os << indent << "None"  << endl;
    }
  else 
    {
    os << endl;
    for (int i = 0 ; i < this->GetNumberOfTexts() ; i++) 
      {
      os << indent << "  " << i <<": " <<  (TextList->GetValue(i) ? TextList->GetValue(i) : "(none)") << endl;
    }
    }

  for (int j = 0 ; j < NUM_TEXT_ATTRIBUTE_TYPES; j ++) 
    {
      os << indent << this->GetAttributeTypesEnumAsString(j) <<": ";
      if (this->PolyData &&  this->PolyData->GetPointData())
    {
      for (int i = 0; i <  this->GetNumberOfTexts(); i++ ) 
        {
          os << this->GetAnnotationAttribute(i,j) << " " ;
        }
      os << endl;
    }
      else 
    {
      os << " None" << endl; 
    }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLAnnotationNode::ResetAnnotations()
{
  if (!this->TextList) 
    {
      this->TextList = vtkStringArray::New();
    }
  else 
    {
      this->TextList->Initialize(); 
    }

  this->ResetTextAttributesAll();
} 

//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::CreatePolyData()
{
  if (!this->PolyData) 
    {
      vtkPolyData *poly = vtkPolyData::New();
      this->SetAndObservePolyData(poly);
      // Releasing data for pipeline parallism.
      // Filters will know it is empty. 
      poly->ReleaseData();
      poly->Delete();
      
      // This assumes I want to display the poly data , which I do not want to as it is displayed by widgets 
      //if (this->GetAnnotationTextDisplayNode())
      //    {
      //      this->GetAnnotationTextDisplayNode()->SetPolyData(poly); 
      //    }
    } 

}


//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::ResetTextAttributesAll() {
  this->CreatePolyData();

  for (int j = 0 ; j < NUM_TEXT_ATTRIBUTE_TYPES; j ++) {
    this->ResetAttributes(j);
  }
} 

//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::ResetAttributes(int id) {
  if (!this->PolyData || !this->PolyData->GetPointData())
    {
      vtkErrorMacro("Annotation: "<< this->GetName() << " PolyData or  this->PolyData->GetPointData() is NULL" ); 
      return;
    }

  if ((id < 0 ))
    {
      vtkErrorMacro("Annotation: "<< this->GetName() << " ID is out of range"); 
      return;
    }


  vtkBitArray *attArray = dynamic_cast <  vtkBitArray *> (this->GetAnnotationAttributes(id));
  if (!attArray) {
    attArray =  vtkBitArray::New();
    attArray->SetName(this->GetAttributeTypesEnumAsString(id));
    this->PolyData->GetPointData()->AddArray(attArray);
    attArray->Delete();
  } 
  attArray->Initialize();
} 



//---------------------------------------------------------------------------
vtkDataArray* vtkMRMLAnnotationNode::GetAnnotationAttributes(int att) 
{
  if (!this->PolyData || !this->PolyData->GetPointData()) 
    {
      vtkErrorMacro("Annotation: " << this->GetName() << " PolyData or  PolyData->GetPointData() is NULL" ); 
      return 0;
    }

  return this->PolyData->GetPointData()->GetScalars(this->GetAttributeTypesEnumAsString(att));
}
 
//---------------------------------------------------------------------------
int vtkMRMLAnnotationNode::GetAnnotationAttribute(vtkIdType id, int att)
{
  vtkBitArray *attArray = dynamic_cast <  vtkBitArray *> (this->GetAnnotationAttributes(att));
  if (attArray) 
    {
      return attArray->GetValue(id);
    }
  vtkErrorMacro("Annotation: " << this->GetName() << " Attributes for " << att << " are not defined"); 
  return  -1;
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::SetAnnotationAttribute(vtkIdType id, int att, double value)
{
  vtkBitArray *attArray = dynamic_cast <  vtkBitArray *> (this->GetAnnotationAttributes(att));
  if (!attArray) 
    {
      return;
    }
  if (id < 0 || id >= attArray->GetSize())
    {
      vtkErrorMacro("SetAnnotationAttribute: id is out of range: id " << id << " Size: " <<  attArray->GetSize())
      return ;
    }
  attArray->SetValue(id,value);
}

//---------------------------------------------------------------------------
int vtkMRMLAnnotationNode::DeleteAttribute(vtkIdType idEntry, vtkIdType idAtt) 
{
  vtkBitArray *dataArray = dynamic_cast <vtkBitArray *> (this->GetAnnotationAttributes(idAtt));
  if (!dataArray) 
    {
      vtkErrorMacro("Annotation " << this->GetName() << " Attribute " << idAtt << " does not exist")
      return 0;
    } 
  int n = dataArray->GetSize();
  if (idEntry < 0 || idEntry >= n)
    {
      vtkErrorMacro("Annotation " << this->GetName() << " Annotation out of range")
      return 0;
    } 

  for (int i = idEntry; i < n-1; i++ ) 
    {
      dataArray->SetValue(i,dataArray->GetValue(i+1));
    }
  // Every attribute has its own data array - that is why it works 
  dataArray->Resize(n-1);

  return 1;
}



//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::SetText(int id, const char *newText,int selectedFlag, int visibleFlag)
{
  if (id < 0) 
    {
    vtkErrorMacro("Invalid ID");
    return;
    }
  if (!this->TextList) 
    {
    vtkErrorMacro("TextList is NULL");
    return;
    }

  vtkStdString newString;
  if (newText)
    {
    newString = vtkStdString(newText);
    }

  // check if the same as before
  if (((this->TextList->GetNumberOfValues() == 0) && (newText == NULL || newString == "")) ||
      ((this->TextList->GetNumberOfValues() > id) && 
       (this->TextList->GetValue(id) == newString) &&
       (this->GetAnnotationAttribute(id, TEXT_SELECTED) == selectedFlag) &&
       (this->GetAnnotationAttribute(id, TEXT_VISIBLE) == visibleFlag) ) )
    {
    return;
    }

  if (!this->PolyData || !this->PolyData->GetPointData())
    {
    this->ResetTextAttributesAll(); 
    }

  this->TextList->InsertValue(id,newString);
 
  for (int j = 0 ; j < NUM_TEXT_ATTRIBUTE_TYPES; j ++) 
    {
    this->SetAttributeSize(j,this->GetNumberOfTexts());
    }
  this->SetAnnotationAttribute(id, TEXT_SELECTED, selectedFlag);
  this->SetAnnotationAttribute(id, TEXT_VISIBLE, visibleFlag);

  if(!this->GetDisableModifiedEvent())
  {
  // invoke a modified event
  this->InvokeEvent(vtkCommand::ModifiedEvent);
  }

}

void vtkMRMLAnnotationNode::SetAttributeSize(vtkIdType  id, vtkIdType n)
{
  vtkBitArray *dataArray = dynamic_cast <vtkBitArray *> (this->GetAnnotationAttributes(id));
  if (!dataArray) 
    {
      this->ResetAttributes(id);
      dataArray = dynamic_cast <vtkBitArray *> (this->GetAnnotationAttributes(id));
    }
  dataArray->Resize(n);
}


//-------------------------------------------------------------------------
int vtkMRMLAnnotationNode::AddText(const char *newText,int selectedFlag, int visibleFlag) 
{
  if (!this->TextList) {
    vtkErrorMacro("Annotation: For " << this->GetName() << " text is not defined"); 
    return -1 ;
  }
  int n = this->GetNumberOfTexts();
  this->SetText(n,newText,selectedFlag, visibleFlag); 

  return n;
}

//-------------------------------------------------------------------------
vtkStdString  vtkMRMLAnnotationNode::GetText(int n)
{
  if ((this->GetNumberOfTexts() <= n) || n < 0 )
    {
      return vtkStdString();
    }
  return this->TextList->GetValue(n); 
}

//-------------------------------------------------------------------------
int  vtkMRMLAnnotationNode::DeleteText(int id)
{
  if (!this->TextList)
    {
      return -1;
    }

  int n = this->GetNumberOfTexts();
  if (id < 0 || id >= n) 
    {
      return -1;
    }

  for (int i = id; id < n-1; i++ ) 
    {
      this->TextList->SetValue(i,this->GetText(i+1));
    }

  this->TextList->Resize(n-1);

  if (!this->PolyData || !this->PolyData->GetPointData()) 
    {
      this->ResetTextAttributesAll(); 
      return 1;
    }

  for (int j = 0 ; j < NUM_TEXT_ATTRIBUTE_TYPES; j ++) 
    {
      vtkBitArray *dataArray = dynamic_cast <vtkBitArray *> (this->GetAnnotationAttributes(j));
      if (!dataArray) 
    {
      this->ResetAttributes(j);
      dataArray = dynamic_cast <vtkBitArray *> (this->GetAnnotationAttributes(j));
      dataArray->Resize(this->GetNumberOfTexts());
    }
      else 
    {
      this->DeleteAttribute(id,j);
    }
    }
  return 1;
}


//-------------------------------------------------------------------------
int vtkMRMLAnnotationNode::GetNumberOfTexts() {
  if (!this->TextList) 
    {
      return -1;
    }
  return this->TextList->GetNumberOfValues();
}


//-------------------------------------------------------------------------
const char *vtkMRMLAnnotationNode::GetAttributeTypesEnumAsString(int val) 
{
  if (val == TEXT_SELECTED) return "textSelected";
  if (val == TEXT_VISIBLE) return "textVisible";
  return "(unknown)";
};


//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLAnnotationNode::CreateDefaultStorageNode()
{
  return vtkMRMLStorageNode::SafeDownCast(vtkMRMLAnnotationStorageNode::New());
}

//----------------------------------------------------------------------------
vtkMRMLAnnotationTextDisplayNode* vtkMRMLAnnotationNode::GetAnnotationTextDisplayNode()
{
  int nnodes = this->GetNumberOfDisplayNodes();
  vtkMRMLAnnotationTextDisplayNode *node = NULL;
  for (int n=0; n<nnodes; n++)
    {
    node = vtkMRMLAnnotationTextDisplayNode::SafeDownCast(this->GetNthDisplayNode(n));
    if (node && node->IsA("vtkMRMLAnnotationTextDisplayNode"))
      {
    return node;
      }
    }
  return 0;
}


//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::CreateAnnotationTextDisplayNode()
{
  vtkMRMLAnnotationTextDisplayNode *node = this->GetAnnotationTextDisplayNode();
  if (node) return;
  if (!this->GetScene()) 
    {
      vtkErrorMacro("vtkMRMLAnnotationNode::CreateAnnotationTextDisplayNode Annotation: No scene defined" ) ;
      return;
    }

  node = vtkMRMLAnnotationTextDisplayNode::New();
  node->SetScene(this->GetScene());
  this->GetScene()->AddNode(node);
  node->Delete();
  this->AddAndObserveDisplayNodeID(node->GetID());
  // This assumes I want to display the poly data , which I do not want to as it is displayed by widgets 
  // node->SetPolyData(this->GetPolyData());
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::SetTextScale(double textScale)
{
  this->GetAnnotationTextDisplayNode()->SetTextScale(textScale);
  this->InvokeEvent(vtkCommand::ModifiedEvent);
}

//---------------------------------------------------------------------------
double vtkMRMLAnnotationNode::GetTextScale()
{
  return this->GetAnnotationTextDisplayNode()->GetTextScale();
}

//---------------------------------------------------------------------------
void vtkMRMLAnnotationNode::SetLocked(int locked)
{
    if (this->Locked == locked)
    {
        return;
    }
    this->Locked = locked;
    if(!this->GetDisableModifiedEvent())
    {
      // invoke a lock modified event
      this->InvokeEvent(vtkMRMLAnnotationNode::LockModifiedEvent);
      this->Modified();
    }
    this->ModifiedSinceReadOn();
}
   

//----------------------------------------------------------------------------
void vtkMRMLAnnotationNode::Initialize(vtkMRMLScene* mrmlScene)
{
  if (!mrmlScene)
  {
    vtkErrorMacro("Scene was null!")
    return;
  }

  // we need to disable the modified event which would get fired when we set the new displayNode
  this->DisableModifiedEventOn();
  this->CreateAnnotationTextDisplayNode();
  this->DisableModifiedEventOff();

  mrmlScene->AddNode(this);
}

//----------------------------------------------------------------------------
// Create a backup of this node and store it with the node.
void vtkMRMLAnnotationNode::CreateBackup()
{

  vtkMRMLAnnotationNode * backupNode = vtkMRMLAnnotationNode::New();

  backupNode->CopyWithoutModifiedEvent(this);

  this->m_Backup = backupNode;


}

//----------------------------------------------------------------------------
// Clears the backup of this node.
void vtkMRMLAnnotationNode::ClearBackup()
{
  if (this->m_Backup)
    {
    this->m_Backup->Delete();
    this->m_Backup = 0;
    }
}

//----------------------------------------------------------------------------
// Returns the backup of this node.
vtkMRMLAnnotationNode * vtkMRMLAnnotationNode::GetBackup()
{

  return this->m_Backup;

}

//----------------------------------------------------------------------------
// Restores the backup of this node.
void vtkMRMLAnnotationNode::RestoreBackup()
{

  if (this->m_Backup)
    {
    this->CopyWithSingleModifiedEvent(this->m_Backup);
    }
  else
    {
    vtkErrorMacro("RestoreBackup - could not get the attached backup")
    }

}


//----------------------------------------------------------------------------
// Save the views
void vtkMRMLAnnotationNode::SaveView()
{

  // pointers to the current sliceNodes in the scene
  vtkMRMLSliceNode* redSliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetScene()->GetNthNodeByClass(0,"vtkMRMLSliceNode"));
  vtkMRMLSliceNode* yellowSliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetScene()->GetNthNodeByClass(1,"vtkMRMLSliceNode"));
  vtkMRMLSliceNode* greenSliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetScene()->GetNthNodeByClass(2,"vtkMRMLSliceNode"));

  // the current camera
  vtkMRMLCameraNode* cameraNode = vtkMRMLCameraNode::SafeDownCast(this->GetScene()->GetNthNodeByClass(0,"vtkMRMLCameraNode"));

  // TODO support dual 3D view layout

  // initialize our copies of the current sliceNodes
  this->m_RedSliceNode = vtkMRMLSliceNode::New();
  this->m_YellowSliceNode = vtkMRMLSliceNode::New();
  this->m_GreenSliceNode = vtkMRMLSliceNode::New();
  this->m_CameraNode = vtkMRMLCameraNode::New();

  // now copy the current ones over to our sliceNodes
  this->m_RedSliceNode->Copy(redSliceNode);
  this->m_YellowSliceNode->Copy(yellowSliceNode);
  this->m_GreenSliceNode->Copy(greenSliceNode);
  this->m_CameraNode->Copy(cameraNode);

}

//----------------------------------------------------------------------------
// Restore the saved views
void vtkMRMLAnnotationNode::RestoreView()
{

  // if we do not have stores views, exit now
  if (!this->m_RedSliceNode)
    {
    return;
    }

  if (!this->m_YellowSliceNode)
    {
    return;
    }

  if (!this->m_GreenSliceNode)
    {
    return;
    }


  // pointers to the current sliceNodes in the scene
  vtkMRMLSliceNode* redSliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetScene()->GetNthNodeByClass(0,"vtkMRMLSliceNode"));
  vtkMRMLSliceNode* yellowSliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetScene()->GetNthNodeByClass(1,"vtkMRMLSliceNode"));
  vtkMRMLSliceNode* greenSliceNode = vtkMRMLSliceNode::SafeDownCast(this->GetScene()->GetNthNodeByClass(2,"vtkMRMLSliceNode"));

  // the current camera
  vtkMRMLCameraNode* cameraNode = vtkMRMLCameraNode::SafeDownCast(this->GetScene()->GetNthNodeByClass(0,"vtkMRMLCameraNode"));


  // now copy our saved sliceNodes over the current ones
  // this restores the view
  redSliceNode->CopyWithSingleModifiedEvent(this->m_RedSliceNode);
  yellowSliceNode->CopyWithSingleModifiedEvent(this->m_YellowSliceNode);
  greenSliceNode->CopyWithSingleModifiedEvent(this->m_GreenSliceNode);
  cameraNode->CopyWithSingleModifiedEvent(this->m_CameraNode);

}

