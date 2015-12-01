#include <map>

#include "vtkNRRDWriter.h"


#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include <vtkVersion.h>

class AttributeMapType: public std::map<std::string, std::string> {};

vtkStandardNewMacro(vtkNRRDWriter);

//----------------------------------------------------------------------------
vtkNRRDWriter::vtkNRRDWriter()
{
  this->FileName = NULL;
  this->BValues = vtkDoubleArray::New();
  this->DiffusionGradients = vtkDoubleArray::New();
  this->IJKToRASMatrix = vtkMatrix4x4::New();
  this->MeasurementFrameMatrix = vtkMatrix4x4::New();
  this->UseCompression = 1;
  this->DiffusionWeigthedData = 0;
  this->FileType = VTK_BINARY;
  this->WriteErrorOff();
  this->Attributes = new AttributeMapType;
}

//----------------------------------------------------------------------------
vtkNRRDWriter::~vtkNRRDWriter()
{
  if ( this->FileName )
    {
    delete [] this->FileName;
    }

  if (this->DiffusionGradients)
    {
    this->DiffusionGradients->Delete();
    }
  if (this->BValues)
    {
    this->BValues->Delete();
    }
  if (this->IJKToRASMatrix)
    {
    this->IJKToRASMatrix->Delete();
    }
  if (this->MeasurementFrameMatrix)
    {
    this->MeasurementFrameMatrix->Delete();
    }
  if (this->Attributes)
    {
    delete this->Attributes;
    }
}

//----------------------------------------------------------------------------
vtkImageData* vtkNRRDWriter::GetInput()
{
  return vtkImageData::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
vtkImageData* vtkNRRDWriter::GetInput(int port)
{
  return vtkImageData::SafeDownCast(this->Superclass::GetInput(port));
}

//----------------------------------------------------------------------------
int vtkNRRDWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;

}

//----------------------------------------------------------------------------
// Writes all the data from the input.
void vtkNRRDWriter::vtkImageDataInfoToNrrdInfo(vtkImageData *in, int &kind, size_t &numComp, int &vtkType, void **buffer)
{

  vtkDataArray *array;
  this->DiffusionWeigthedData = 0;
  if ((array = static_cast<vtkDataArray *> (in->GetPointData()->GetScalars())))
    {
    numComp = array->GetNumberOfComponents();
    vtkType = array->GetDataType();
    (*buffer) = array->GetVoidPointer(0);

    switch (numComp)
      {
      case 1:
        kind = nrrdKindScalar;
        break;
      case 2:
        kind = nrrdKindComplex;
        break;
      case 3:
        kind = nrrdKindRGBColor;
        break;
      case 4:
        kind = nrrdKindRGBAColor;
        break;
      default:
        size_t numGrad = this->DiffusionGradients->GetNumberOfTuples();
        size_t numBValues = this->BValues->GetNumberOfTuples();
        if (numGrad == numBValues && numGrad == numComp && numGrad>6)
          {
          kind = nrrdKindList;
          this->DiffusionWeigthedData = 1;
          }
        else
          {
          kind = nrrdKindList;
          }
       }
     }
   else if ((array = static_cast<vtkDataArray *> ( in->GetPointData()->GetVectors())))
     {
     *buffer = array->GetVoidPointer(0);
     vtkType = array->GetDataType();
     kind = nrrdKindVector;
     }
   else if ((array = static_cast<vtkDataArray *> ( in->GetPointData()->GetNormals())))
     {
     *buffer = array->GetVoidPointer(0);
     vtkType = array->GetDataType();
     kind = nrrdKindVector;
     numComp = array->GetNumberOfComponents();
     }
   else if ((array = static_cast<vtkDataArray *> ( in->GetPointData()->GetTensors())))
     {
     *buffer = array->GetVoidPointer(0);
     vtkType = array->GetDataType();
     kind = nrrdKind3DMatrix;
     numComp = array->GetNumberOfComponents();
     }
}

int vtkNRRDWriter::VTKToNrrdPixelType( const int vtkPixelType )
  {
  switch( vtkPixelType )
    {
    default:
    case VTK_VOID:
      return nrrdTypeDefault;
      break;
    case VTK_CHAR:
      return nrrdTypeChar;
      break;
    case VTK_UNSIGNED_CHAR:
      return nrrdTypeUChar;
      break;
    case VTK_SHORT:
      return nrrdTypeShort;
      break;
    case VTK_UNSIGNED_SHORT:
      return nrrdTypeUShort;
      break;
      //    case nrrdTypeLLong:
      //      return LONG ;
      //      break;
      //    case nrrdTypeULong:
      //      return ULONG;
      //      break;
    case VTK_INT:
      return nrrdTypeInt;
      break;
    case VTK_UNSIGNED_INT:
      return nrrdTypeUInt;
      break;
    case VTK_FLOAT:
      return nrrdTypeFloat;
      break;
    case VTK_DOUBLE:
      return nrrdTypeDouble;
      break;
    }
  }



//----------------------------------------------------------------------------
// Writes all the data from the input.
void vtkNRRDWriter::WriteData()
{
  this->WriteErrorOff();
  if (this->GetFileName() == NULL)
    {
    vtkErrorMacro("FileName has not been set. Cannot save file");
    this->WriteErrorOn();
    return;
    }

  Nrrd *nrrd = nrrdNew();
  NrrdIoState *nio = nrrdIoStateNew();
  int kind[NRRD_DIM_MAX];
  size_t size[NRRD_DIM_MAX];
  unsigned int nrrdDim, baseDim, spaceDim;
  double spaceDir[NRRD_DIM_MAX][NRRD_SPACE_DIM_MAX];
  double origin[NRRD_DIM_MAX];
  void *buffer;
  int vtkType;

    // Fill in image information.

  //vtkImageData *input = this->GetInput();

  // Find Pixel type from data and select a buffer.
  this->vtkImageDataInfoToNrrdInfo(this->GetInput(),kind[0],size[0],vtkType, &buffer);

  spaceDim = 3; // VTK is always 3D volumes.
  if (size[0] > 1)
    {
    // the range axis has no space direction
    for (unsigned int saxi=0; saxi < spaceDim; saxi++)
      {
      spaceDir[0][saxi] = AIR_NAN;
      }
    baseDim = 1;
    }
  else
    {
    baseDim = 0;
    }
  nrrdDim = baseDim + spaceDim;

  unsigned int axi;
  for (axi=0; axi < spaceDim; axi++)
    {
    size[axi+baseDim] = this->GetInput()->GetDimensions()[axi];
    kind[axi+baseDim] = nrrdKindDomain;
    origin[axi] = this->IJKToRASMatrix->GetElement((int) axi,3);
    //double spacing = this->GetInput()->GetSpacing()[axi];
    for (unsigned int saxi=0; saxi < spaceDim; saxi++)
      {
      spaceDir[axi+baseDim][saxi] = this->IJKToRASMatrix->GetElement(saxi,axi);
      }
    }

  if (nrrdWrap_nva(nrrd, const_cast<void *> (buffer),
                   this->VTKToNrrdPixelType( vtkType ),
                   nrrdDim, size)
      || nrrdSpaceDimensionSet(nrrd, spaceDim)
      || nrrdSpaceOriginSet(nrrd, origin))
    {
    char *err = biffGetDone(NRRD); // would be nice to free(err)
    vtkErrorMacro("Write: Error wrapping nrrd for "
                      << this->GetFileName() << ":\n" << err);
    // Free the nrrd struct but don't touch nrrd->data
    nrrd = nrrdNix(nrrd);
    nio = nrrdIoStateNix(nio);
    this->WriteErrorOn();
    return;
    }
  nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoKind, kind);
  nrrdAxisInfoSet_nva(nrrd, nrrdAxisInfoSpaceDirection, spaceDir);
  nrrd->space = nrrdSpaceRightAnteriorSuperior;

  // Write out attributes, diffusion information and the measurement frame.
  //

  // 0. Write out any attributes. Write non-specific attributes first. This allows
  // variables like the diffusion gradients and B-values that are saved as
  // attributes on disk to overwrite the attributes later. (This
  // should ensure the version written to disk has the best information.)
  AttributeMapType::iterator ait;
  for (ait = this->Attributes->begin(); ait != this->Attributes->end(); ++ait)
    {
    nrrdKeyValueAdd(nrrd, (*ait).first.c_str(), (*ait).second.c_str());
    }

  // 1. Measurement Frame
  if (this->MeasurementFrameMatrix)
    {
    for (unsigned int saxi=0; saxi < nrrd->spaceDim; saxi++)
      {
      for (unsigned int saxj=0; saxj < nrrd->spaceDim; saxj++)
        {
        // Note the transpose: each entry in the nrrd measurementFrame
        // is a column of the matrix
        nrrd->measurementFrame[saxi][saxj] = this->MeasurementFrameMatrix->GetElement(saxj,saxi);
        }
      }
    }

  // 2. Take care about diffusion data
  if (this->DiffusionWeigthedData)
    {
    unsigned int numGrad = this->DiffusionGradients->GetNumberOfTuples();
    unsigned int numBValues = this->BValues->GetNumberOfTuples();

    if (kind[0] == nrrdKindList && numGrad == size[0] && numBValues == size[0])
      {
      // This is diffusion Data
      double *grad;
      double bVal,factor;
      double maxbVal = this->BValues->GetRange()[1];
      char value[1024];
      char key[1024];
      strcpy(key,"modality");
      strcpy(value,"DWMRI");
      nrrdKeyValueAdd(nrrd, key, value);

      strcpy(key,"DWMRI_b-value");
      //sprintf(value,"%f",maxbVal,1024);
      sprintf(value,"%f",maxbVal);
      nrrdKeyValueAdd(nrrd,key, value);
      for (unsigned int ig =0; ig< numGrad; ig++)
        {
        grad=this->DiffusionGradients->GetTuple3(ig);
        bVal = this->BValues->GetValue(ig);
        // for multiple b-values, scale factor is `sqrt(b/b_max)`
        // per NA-MIC DWI convention. so we take `norm^2 * b_max`
        // to get back the original b-values.
        factor = sqrt(bVal/maxbVal);
        sprintf(key,"%s%04d","DWMRI_gradient_",ig);
        sprintf(value,"%f %f %f",grad[0]*factor, grad[1]*factor, grad[2]*factor);
        nrrdKeyValueAdd(nrrd,key, value);
        }
      }
    }

  // set encoding for data: compressed (raw), (uncompressed) raw, or ascii
  if ( this->GetUseCompression() && nrrdEncodingGzip->available() )
    {
    // this is necessarily gzip-compressed *raw* data
    nio->encoding = nrrdEncodingGzip;
    }
  else
    {
    int fileType = this->GetFileType();
    switch ( fileType )
      {
      default:
      case VTK_BINARY:
        nio->encoding = nrrdEncodingRaw;
        break;
      case VTK_ASCII:
        nio->encoding = nrrdEncodingAscii;
        break;
      }
    }

  // set endianness as unknown of output
  nio->endian = airEndianUnknown;

  // Write the nrrd to file.
  if (nrrdSave(this->GetFileName(), nrrd, nio))
    {
    char *err = biffGetDone(NRRD); // would be nice to free(err)
    vtkErrorMacro("Write: Error writing "
                      << this->GetFileName() << ":\n" << err);
    this->WriteErrorOn();
    }
  // Free the nrrd struct but don't touch nrrd->data
  nrrd = nrrdNix(nrrd);
  nio = nrrdIoStateNix(nio);
  return;
}

void vtkNRRDWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "RAS to IJK Matrix: ";
     this->IJKToRASMatrix->PrintSelf(os,indent);
  os << indent << "Measurement frame: ";
     this->MeasurementFrameMatrix->PrintSelf(os,indent);
}

void vtkNRRDWriter::SetAttribute(const std::string& name, const std::string& value)
{
  if (!this->Attributes)
    {
    return;
    }

  (*this->Attributes)[name] = value;
}
