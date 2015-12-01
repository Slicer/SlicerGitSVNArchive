/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkTensorMask.cxx,v $
  Date:      $Date: 2006/02/15 19:09:57 $
  Version:   $Revision: 1.4.2.2 $

=========================================================================auto=*/
#include "vtkTensorMask.h"

#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTensorMask);

//----------------------------------------------------------------------------
vtkTensorMask::vtkTensorMask()
{

}

vtkTensorMask::~vtkTensorMask()
{

}

//----------------------------------------------------------------------------
// Replace superclass Execute with a function that allocates tensors
// as well as scalars.  This gets called before multithreader starts
// (after which we can't allocate, for example in ThreadedExecute).
// Note we return to the regular pipeline at the end of this function.
void vtkTensorMask::ExecuteData(vtkDataObject *out)
{
  vtkImageData *output = vtkImageData::SafeDownCast(out);

  // set extent so we know how many tensors to allocate
  output->SetExtent(this->GetUpdateExtent());

  // allocate output tensors
  vtkFloatArray* data = vtkFloatArray::New();
  int* dims = output->GetDimensions();
  data->SetNumberOfComponents(9);
  data->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  output->GetPointData()->SetTensors(data);
  data->Delete();

  // jump back into normal pipeline: call standard superclass method here
  //  this->vtkImageAlgorithm::ExecuteData(out);
  this->Superclass::ExecuteData(out);
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// This is exactly the same as the superclass: default behavior
// when only scalars are present
template <class T, class D>
static void vtkTensorMaskExecute(vtkTensorMask *self, int ext[6],
                vtkImageData *in1Data, T *in1Ptr,
                vtkImageData *in2Data, D *in2Ptr,
                vtkImageData *outData, T *outPtr, int id)
{
  int num0, num1, num2, numC, pixSize;
  int idx0, idx1, idx2;
  vtkIdType in1Inc0, in1Inc1, in1Inc2;
  vtkIdType in2Inc0, in2Inc1, in2Inc2;
  vtkIdType outInc0, outInc1, outInc2;
  T *maskedValue;
  double *v;
  int nv;
  int maskState;
  unsigned long count = 0;
  unsigned long target;

  // create a masked output value with the correct length by cycling
  numC = outData->GetNumberOfScalarComponents();
  maskedValue = new T[numC];
  v = self->GetMaskedOutputValue();
  nv = self->GetMaskedOutputValueLength();
  for (idx0 = 0, idx1 = 0; idx0 < numC; ++idx0, ++idx1)
    {
      if (idx1 >= nv)
    {
      idx1 = 0;
    }
      maskedValue[idx0] = (T)(v[idx1]);
    }
  pixSize = numC * sizeof(T);
  maskState = self->GetNotMask();

  // Get information to march through data
  in1Data->GetContinuousIncrements(ext, in1Inc0, in1Inc1, in1Inc2);
  in2Data->GetContinuousIncrements(ext, in2Inc0, in2Inc1, in2Inc2);
  outData->GetContinuousIncrements(ext, outInc0, outInc1, outInc2);
  num0 = ext[1] - ext[0] + 1;
  num1 = ext[3] - ext[2] + 1;
  num2 = ext[5] - ext[4] + 1;

  target = (unsigned long)(num2*num1/50.0);
  target++;

  // Loop through ouput pixels
  for (idx2 = 0; idx2 < num2; ++idx2)
    {
      for (idx1 = 0; !self->AbortExecute && idx1 < num1; ++idx1)
    {
      if (!id)
        {
          if (!(count%target))
        {
          self->UpdateProgress(count/(50.0*target));
        }
          count++;
        }
      for (idx0 = 0; idx0 < num0; ++idx0)
        {
          // Pixel operation
          if (*in2Ptr && maskState == 1)
        {
          memcpy(outPtr, maskedValue, pixSize);
        }
          else if ( ! *in2Ptr && maskState == 0)
        {
          memcpy(outPtr, maskedValue, pixSize);
        }
          else
        {
          memcpy(outPtr, in1Ptr, pixSize);
        }

          in1Ptr += numC;
          outPtr += numC;
          in2Ptr += 1;
        }
      in1Ptr += in1Inc1;
      in2Ptr += in2Inc1;
      outPtr += outInc1;
    }
      in1Ptr += in1Inc2;
      in2Ptr += in2Inc2;
      outPtr += outInc2;
    }

  delete [] maskedValue;
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
// this is for when only tensors are present
template <class D>
static void vtkTensorMaskExecuteTensor(vtkTensorMask *self, int ext[6],
                       vtkImageData *in1Data,
                       vtkImageData *in2Data, D *in2Ptr,
                       vtkImageData *outData, int id)
{
  int num0, num1, num2;
  int idx0, idx1, idx2;
  vtkIdType in1Inc0, in1Inc1, in1Inc2;
  vtkIdType in2Inc0, in2Inc1, in2Inc2;
  vtkIdType outInc0, outInc1, outInc2;
  int maskState;
  unsigned long count = 0;
  unsigned long target;

  vtkDataArray *inTensors;
  vtkDataArray *outTensors;
  double inT[3][3];
  double outT[3][3];

  int ptId;

  // do we NOT the mask?
  maskState = self->GetNotMask();

  // input tensors
  inTensors = self->GetImageDataInput(0)->GetPointData()->GetTensors();
  // output tensors
  outTensors = self->GetOutput()->GetPointData()->GetTensors();


  //Raul: Bad ptId inizialization
  // "GetTensorPointerForExtent" (get start spot in point data)
  // This is the id in the input and output datasets.
  //ptId = ext[0] + ext[2]*(ext[1]-ext[0]) + ext[4]*(ext[3]-ext[2]);

  vtkIdType outInc[3];
  int outFullUpdateExt[6];
  self->GetOutput()->GetIncrements(outInc);
  self->GetUpdateExtent(outFullUpdateExt); //We are only working over the update extent
  ptId = ((ext[0] - outFullUpdateExt[0]) * outInc[0]
         + (ext[2] - outFullUpdateExt[2]) * outInc[1]
         + (ext[4] - outFullUpdateExt[4]) * outInc[2]);

  // Get information to march through data
  in1Data->GetContinuousIncrements(ext, in1Inc0, in1Inc1, in1Inc2);
  in2Data->GetContinuousIncrements(ext, in2Inc0, in2Inc1, in2Inc2);
  outData->GetContinuousIncrements(ext, outInc0, outInc1, outInc2);
  num0 = ext[1] - ext[0] + 1;
  num1 = ext[3] - ext[2] + 1;
  num2 = ext[5] - ext[4] + 1;

  target = (unsigned long)(num2*num1/50.0);
  target++;

  // Loop through ouput pixels
  for (idx2 = 0; idx2 < num2; ++idx2)
    {
      for (idx1 = 0; !self->AbortExecute && idx1 < num1; ++idx1)
    {
      if (!id)
        {
          if (!(count%target))
        {
          self->UpdateProgress(count/(50.0*target));
        }
          count++;
        }
      for (idx0 = 0; idx0 < num0; ++idx0)
        {
          inTensors->GetTuple(ptId,(double *)inT);
          //outTensors->GetTuple(ptId,outT);

          // Pixel operation: clear or copy
          if (*in2Ptr && maskState == 1)
        {
          //outT->Initialize();
          for (int j=0; j<3; j++)
            {
              for (int i=0; i<3; i++)
            {
                   outT[i][j] = 0;
            }
            }
        }
          else if ( ! *in2Ptr && maskState == 0)
        {
          //outT->Initialize();
          for (int j=0; j<3; j++)
            {
              for (int i=0; i<3; i++)
            {
                   outT[i][j] = 0;
            }
            }
        }
          else
        {
          //outT->DeepCopy(inT);
          for (int j=0; j<3; j++)
            {
              for (int i=0; i<3; i++)
            {
                   outT[i][j] = inT[i][j];
            }
            }
        }

          // set the output tensor to the calculated one
          outTensors->SetTuple(ptId,(double *)outT);

          ptId += 1;
          in2Ptr += 1;
        }
      ptId += outInc1;
    }
      ptId += outInc2;
    }
}



//----------------------------------------------------------------------------
// This method is passed a input and output Datas, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the Datas data types.
void vtkTensorMask::ThreadedRequestData(
  vtkInformation * vtkNotUsed( request ),
  vtkInformationVector ** vtkNotUsed( inputVector ),
  vtkInformationVector * vtkNotUsed( outputVector ),
  vtkImageData ***inData,
  vtkImageData **outData,
  int outExt[6], int id)
{
  void *inPtr1;
  void *inPtr2;
  void *outPtr;
  int *tExt;
  vtkDataArray *inTensors;

  if (inData[0][0] == NULL)
    {
      vtkErrorMacro(<< "Input " << 0 << " must be specified.");
      return;
    }
  if (inData[1][0] == NULL)
    {
      vtkErrorMacro(<< "Input " << 1 << " must be specified.");
      return;
    }

  // input image
  inPtr1 = inData[0][0]->GetScalarPointerForExtent(outExt);
  // mask
  inPtr2 = inData[1][0]->GetScalarPointerForExtent(outExt);
  // output
  outPtr = outData[0]->GetScalarPointerForExtent(outExt);
  // input tensors
  inTensors = this->GetImageDataInput(0)->GetPointData()->GetTensors();

  tExt = inData[1][0]->GetExtent();
  if (tExt[0] > outExt[0] || tExt[1] < outExt[1] ||
      tExt[2] > outExt[2] || tExt[3] < outExt[3] ||
      tExt[4] > outExt[4] || tExt[5] < outExt[5])
    {
      vtkErrorMacro("Mask extent not large enough");
      return;
    }

  if (inData[1][0]->GetNumberOfScalarComponents() != 1)
    {
      vtkErrorMacro("Masks can have one component");
    }

  if (inData[0][0]->GetScalarType() != outData[0]->GetScalarType() ||
      (inData[1][0]->GetScalarType() != VTK_UNSIGNED_CHAR &&
       inData[1][0]->GetScalarType() != VTK_SHORT))
    {
      vtkErrorMacro(<< "Execute: image ScalarType ("
      << inData[0][0]->GetScalarType() << ") must match out ScalarType ("
      << outData[0]->GetScalarType() << "), and mask scalar type ("
      << inData[1][0]->GetScalarType() << ") must be unsigned char or short.");
      return;
    }

  // for now process scalars and tensors separately,
  // since scalars are an afterthought, though this is slower.

  // do we have input tensors?
  if (inTensors)
    {
      // call the execute code for tensors
      switch (inData[1][0]->GetScalarType())
    {
    case VTK_UNSIGNED_CHAR:
      vtkTensorMaskExecuteTensor(this, outExt, inData[0][0],
                     inData[1][0],
                     (unsigned char *)(inPtr2),
                     outData[0], id);
      break;
    case VTK_SHORT:
      vtkTensorMaskExecuteTensor(this, outExt, inData[0][0],
                     inData[1][0],
                     (short *)(inPtr2),
                     outData[0], id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType for mask input");
      return;
    }

    }

  // do we have input scalars?
  if (inPtr1)
    {
      switch (inData[1][0]->GetScalarType())
    {
    case VTK_UNSIGNED_CHAR:
      switch (inData[0][0]->GetScalarType())
        {
          vtkTemplateMacro(vtkTensorMaskExecute(this, outExt,
                inData[0][0], static_cast<VTK_TT*>(inPtr1),
                inData[1][0], static_cast<unsigned char*>(inPtr2),
                outData[0], static_cast<VTK_TT*>(outPtr), id));
        default:
          vtkErrorMacro(<< "Execute: Unknown ScalarType");
          return;
        }
      break;
    case VTK_SHORT:
      switch (inData[0][0]->GetScalarType())
        {
          vtkTemplateMacro(vtkTensorMaskExecute(this, outExt,
                inData[0][0], static_cast<VTK_TT*>(inPtr1),
                inData[1][0], static_cast<short*>(inPtr2),
                outData[0], static_cast<VTK_TT*>(outPtr), id));
        default:
          vtkErrorMacro(<< "Execute: Unknown ScalarType");
          return;
        }
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType for mask input");
      return;
    }
    }
}

void vtkTensorMask::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageMask::PrintSelf(os,indent);
}

