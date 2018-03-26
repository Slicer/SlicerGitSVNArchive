#include "vtkMRMLStreamingVolumeNode.h"

// MRML includes
#include "vtkMRMLScene.h"
#include "vtkMRMLNRRDStorageNode.h"

// VTK includes
#include <vtkCollection.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkXMLUtilities.h>
#include <vtkDataArray.h>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLStreamingVolumeNode);

//----------------------------------------------------------------------------
// vtkMRMLStreamingVolumeNode methods

//-----------------------------------------------------------------------------
vtkMRMLStreamingVolumeNode::vtkMRMLStreamingVolumeNode()
{
  this->CompressionCodec = NULL;
  this->FrameUpdated = false;
  this->KeyFrameDecoded = false;
  this->KeyFrameUpdated = false;
  this->KeyFrame = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->Frame = vtkSmartPointer<vtkUnsignedCharArray>::New();
  this->CodecName = NULL;
  this->CodecType = NULL;
  this->CodecClassName = NULL;
}

//-----------------------------------------------------------------------------
vtkMRMLStreamingVolumeNode::~vtkMRMLStreamingVolumeNode()
{
}

//-----------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::SetCodecNameToCompressor(std::string name)
{
  if(this->CompressionCodec)
    {
    this->CompressionCodec->SetContentCodecName(name);
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
std::string vtkMRMLStreamingVolumeNode::GetCodecNameFromCompressor()
{
  if (this->CompressionCodec)
    {
    return this->CompressionCodec->GetContentCodecName();
    }
  return "";
}

//---------------------------------------------------------------------------
std::string vtkMRMLStreamingVolumeNode::GetCodecTypeFromCompressor()
{
  vtkSmartPointer<vtkStreamingVolumeCodec> codec = this->GetCompressionCodec();
  if (codec)
    {
    return codec->GetContentCodecType();
    }
  return "";
}

//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::SetCodecTypeToCompressor(std::string name)
{
  vtkSmartPointer<vtkStreamingVolumeCodec> codec = this->GetCompressionCodec();
  if(codec)
  {
    codec->SetContentCodecType(name);
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::ObserveOutsideCompressionCodec( vtkStreamingVolumeCodec* codec)
{
  if (codec)
    {
    codec->AddObserver(codec->GetCodecContentModifiedEvent(), this, &vtkMRMLStreamingVolumeNode::ProcessCodecModifiedEvents);
    //------
    if (codec->GetContent().frame != NULL && codec->GetContent().keyFrame != NULL)
      {
      vtkSmartPointer<vtkUnsignedCharArray> bitStream = codec->GetContent().frame;
      this->UpdateFrameByDeepCopy(bitStream);
      vtkSmartPointer<vtkUnsignedCharArray> bitKeyStream = codec->GetContent().keyFrame;
      this->UpdateKeyFrameByDeepCopy(bitKeyStream);
      if (codec->GetContent().image != NULL)
        {
        this->SetAndObserveImageData(codec->GetContent().image);
        }
      }
    this->SetCodecClassName(codec->GetClassName());
    this->SetCodecName(codec->GetContentCodecName().c_str());
    this->SetCodecType(codec->GetContentCodecType().c_str());
    this->CompressionCodec = codec; // should the interal video codec point to the external video codec?
    //-------
    return 1;
    }
  return 0;
}

void vtkMRMLStreamingVolumeNode::CopySrcArrayToDestArray(vtkSmartPointer<vtkUnsignedCharArray> srcString, vtkSmartPointer<vtkUnsignedCharArray> destString)
{
  destString->SetNumberOfTuples(srcString->GetNumberOfValues());
  destString->DeepCopy(srcString);
}

int vtkMRMLStreamingVolumeNode::UpdateFrameByDeepCopy(vtkSmartPointer<vtkUnsignedCharArray> buffer)
{
  if(this->Frame == NULL)
    {
    this->Frame = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->Frame->SetNumberOfComponents(1);
    }
  if(buffer == NULL)
    {
    vtkWarningMacro("Souce frame not valid.")
    return 0;
    }
  this->CopySrcArrayToDestArray(buffer, this->Frame);
  this->SetFrameUpdated(true);
  return 1;
};


int vtkMRMLStreamingVolumeNode::UpdateKeyFrameByDeepCopy(vtkSmartPointer<vtkUnsignedCharArray> buffer)
{
  if(this->KeyFrame == NULL)
    {
    this->KeyFrame = vtkSmartPointer<vtkUnsignedCharArray>::New();
    this->KeyFrame->SetNumberOfComponents(1);
    }
  if(buffer == NULL)
    {
    vtkWarningMacro("Souce key frame not valid.")
    return 0;
    }
  this->CopySrcArrayToDestArray(buffer, this->KeyFrame);
  this->SetKeyFrameUpdated(true);
  return 1;
};


//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::DecodeFrame(vtkUnsignedCharArray*  frame)
{
  if (this->CompressionCodec == NULL)
  {
    vtkWarningMacro("No compression codec available, use the ObserveOutsideCompressionCodec() to set up the compression codec.")
    return 0;
  }
  this->UpdateFrameByDeepCopy(frame);
  if (this->CompressionCodec->UncompressedDataFromStream(frame, true))
    {
    if (this->GetImageData() != this->CompressionCodec->GetContent().image)
      {
      this->SetAndObserveImageData(this->CompressionCodec->GetContent().image);
      }
    this->Modified();
    return 1;
    }
  return 0;
}


//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::DecodeKeyFrame(vtkUnsignedCharArray*  keyFrame)
{
  if (this->CompressionCodec == NULL)
  {
    vtkWarningMacro("No compression codec available, use the ObserveOutsideCompressionCodec() to set up the compression codec.")
    return 0;
  }
  this->UpdateKeyFrameByDeepCopy(keyFrame);
  if (this->CompressionCodec->UncompressedDataFromStream(keyFrame, true))
    {
    if (this->GetImageData() != this->CompressionCodec->GetContent().image)
      {
      this->SetAndObserveImageData(this->CompressionCodec->GetContent().image);
      }
    this->SetKeyFrameDecoded(true);
    this->Modified();
    return 1;
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkMRMLStreamingVolumeNode::EncodeImageData()
{
  if (this->CompressionCodec == NULL)
    {
    vtkWarningMacro("No compression codec available, use the ObserveOutsideCompressionCodec() to set up the compression codec.")
    return 0;
    }
  if (this->GetImageData() != this->CompressionCodec->GetContent().image)
    {
    this->CompressionCodec->SetContentImage(this->GetImageData());
    }
  vtkSmartPointer<vtkUnsignedCharArray> compressedStream = this->CompressionCodec->GetCompressedStreamFromData();
  if (compressedStream != NULL)
    {
    if (compressedStream->GetNumberOfValues()>0)
      {
      this->UpdateFrameByDeepCopy(compressedStream);
      bool keyFrameStatus = this->CompressionCodec->GetContent().keyFrameUpdated;
      this->SetKeyFrameUpdated(keyFrameStatus);
      if(keyFrameStatus)
        {
        this->UpdateKeyFrameByDeepCopy(compressedStream);
        }
        return 1;
      }
    }
  return 0;
}

void vtkMRMLStreamingVolumeNode::ProcessMRMLEvents(vtkObject *caller, unsigned long event, void *callData )
{
  this->vtkMRMLNode::ProcessMRMLEvents(caller, event, callData);
  this->InvokeEvent(ImageDataModifiedEvent);
}

void vtkMRMLStreamingVolumeNode::ProcessCodecModifiedEvents( vtkObject *caller, unsigned long event, void *callData )
{
  vtkSmartPointer<vtkStreamingVolumeCodec> modifiedCodec = reinterpret_cast<vtkStreamingVolumeCodec*>(caller);
  if (modifiedCodec == NULL)
    {
    // we are only interested in proxy node modified events
    return;
    }
  if (event != modifiedCodec->GetCodecContentModifiedEvent())
    {
    return;
    }
  this->SetKeyFrameUpdated(false);
  this->UpdateFrameByDeepCopy(modifiedCodec->GetContent().frame);
  if(modifiedCodec->GetContent().keyFrameUpdated)
    {
    this->UpdateKeyFrameByDeepCopy(modifiedCodec->GetContent().keyFrame);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkStreamingVolumeCodec* vtkMRMLStreamingVolumeNode::GetCompressionCodec()
{
  if (this->CompressionCodec)
    {
    return this->CompressionCodec;
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkMRMLWriteXMLBeginMacro(of);
  vtkMRMLWriteXMLStringMacro(CodecName, CodecName);
  vtkMRMLWriteXMLStringMacro(CodecClassName, CodecClassName);
  vtkMRMLWriteXMLStringMacro(CodecType, CodecType);
  vtkMRMLWriteXMLEndMacro();
}

//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  
  Superclass::ReadXMLAttributes(atts);
  vtkMRMLReadXMLBeginMacro(atts);
  vtkMRMLReadXMLStringMacro(CodecName, CodecName);
  vtkMRMLReadXMLStringMacro(CodecClassName, CodecClassName);
  vtkMRMLReadXMLStringMacro(CodecType, CodecType);
  vtkMRMLReadXMLEndMacro();
  this->EndModify(disabledModify);
}


//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();
  Superclass::Copy(anode);
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}
