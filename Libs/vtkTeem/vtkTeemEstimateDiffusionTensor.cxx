/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkTeemEstimateDiffusionTensor.cxx,v $
  Date:      $Date: 2007/04/09 08:10:15 $
  Version:   $Revision: 1.3.2.1 $

=========================================================================auto=*/
#include "vtkTeemEstimateDiffusionTensor.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkFloatArray.h"
#include <vtkVersion.h>


#define VTKEPS 10e-12

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTeemEstimateDiffusionTensor);


//----------------------------------------------------------------------------
vtkTeemEstimateDiffusionTensor::vtkTeemEstimateDiffusionTensor()
{
  // may be set by user
  this->Transform = NULL;

  this->NumberOfGradients = 7;

  // this is LeBihan's b factor for physical MR gradient parameters
  // (same number as C-F uses)
  this->BValues = vtkDoubleArray::New();
  this->BValues->SetNumberOfComponents(1);
  this->BValues->SetNumberOfTuples(this->NumberOfGradients);
  for (int i=0; i<this->NumberOfGradients;i++)
    {
    this->BValues->SetValue(i,1000);
    }
  this->MaxB = 1000;

  // Scalar Factor for the tensor values
  //this->ScaleFactor = 1;
  this->MinimumSignalValue = 1.0;
  this->Sigma = 0.0;
  this->EstimationMethod = tenEstimateMethodLLS;

  this->NumberOfWLSIterations = 1;
  this->knownB0 = 0;
  this->ShiftNegativeEigenvalues = 0;

  // Output images beside the estimated tensor
  this->SetNumberOfOutputPorts(3);

  // defaults are from DT-MRI
  // (from Processing and Visualization for
  // Diffusion Tensor MRI, C-F Westin, pg 8)
  this->DiffusionGradients = vtkDoubleArray::New();
  this->DiffusionGradients->SetNumberOfComponents(3);
  this->DiffusionGradients->SetNumberOfTuples(this->NumberOfGradients);
  this->SetDiffusionGradient(0,0,0,0);
  this->SetDiffusionGradient(1,1,1,0);
  this->SetDiffusionGradient(2,0,1,1);
  this->SetDiffusionGradient(3,1,0,1);
  this->SetDiffusionGradient(4,0,1,-1);
  this->SetDiffusionGradient(5,1,-1,0);
  this->SetDiffusionGradient(6,-1,0,1);

  this->RescaledDiffusionGradients = vtkDoubleArray::New();
  this->RescaledDiffusionGradients->DeepCopy(this->DiffusionGradients);

}

//----------------------------------------------------------------------------
vtkTeemEstimateDiffusionTensor::~vtkTeemEstimateDiffusionTensor()
{
  this->BValues->Delete();
  this->DiffusionGradients->Delete();
  this->RescaledDiffusionGradients->Delete();
  if (this->Transform)
    {
    this->Transform->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkTeemEstimateDiffusionTensor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "NumberOfGradients: " << this->NumberOfGradients << "\n";
  double g[3];
  // print all of the gradients
  for (int i = 0; i < this->NumberOfGradients; i++ )
    {
      this->GetDiffusionGradient(i,g);
      os << indent << "Gradient " << i << ": ("
         << g[0] << ", "
         << g[1] << ", "
         << g[2] << ")"
         <<  "B value: "
         << this->BValues->GetValue(i) << "\n";
    }
}

//----------------------------------------------------------------------------
void vtkTeemEstimateDiffusionTensor::SetDiffusionGradients(vtkDoubleArray *grad)
{
  this->DiffusionGradients->DeepCopy(grad);
  this->NumberOfGradients = this->DiffusionGradients->GetNumberOfTuples();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTeemEstimateDiffusionTensor::SetBValues(vtkDoubleArray *bValues)
{
  this->BValues->DeepCopy(bValues);
  this->CalculateMaxB();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkTeemEstimateDiffusionTensor::CalculateMaxB()
{
  this->SetMaxB( this->BValues->GetRange()[1] );
}

//----------------------------------------------------------------------------
vtkImageData* vtkTeemEstimateDiffusionTensor::GetBaseline()
{
  return this->GetOutput(1);
}
//----------------------------------------------------------------------------
vtkImageData* vtkTeemEstimateDiffusionTensor::GetAverageDWI()
{
  return this->GetOutput(2);
}

//----------------------------------------------------------------------------
void vtkTeemEstimateDiffusionTensor::TransformDiffusionGradients()
{
  double gradient[3];
  double g[3];
  double norm;
  // if matrix has not been set by user don't use it
  if (this->Transform == NULL)
    {
      return;
    }

  vtkDebugMacro("Transforming diffusion gradients");
  //this->Transform->Print(cout);


  // transform each gradient by this matrix
  for (int i = 0; i < this->NumberOfGradients; i++ )
    {
      this->GetDiffusionGradient(i,g);
      this->Transform->TransformPoint(g,gradient);

      norm = sqrt(gradient[0]*gradient[0]+gradient[1]*gradient[1]+gradient[2]*gradient[2]);
      if (norm > VTKEPS)
        {
        gradient[0] /=norm;
        gradient[1] /=norm;
        gradient[2] /=norm;
        }
      // set the gradient to the transformed one
      this->SetDiffusionGradient(i,gradient);
    }
}

//----------------------------------------------------------------------------
// The number of required inputs is one more than the number of
// diffusion gradients applied.  (Since image 0 is an image
// acquired without diffusion gradients).
void vtkTeemEstimateDiffusionTensor::SetNumberOfGradients(int num)
{
  if (this->NumberOfGradients != num)
    {
      vtkDebugMacro ("setting num gradients to " << num);
      // internal array for storage of gradient vectors
      this->DiffusionGradients->SetNumberOfTuples(num);
      this->BValues->SetNumberOfTuples(num);
      // this class's info
      this->NumberOfGradients = num;
      //this->NumberOfRequiredInputs = num;
      this->Modified();
    }
}

void vtkTeemEstimateDiffusionTensor::GetDiffusionGradient(int num,double grad[3])
  {  if (num<this->DiffusionGradients->GetNumberOfTuples()) {
       grad[0]=this->DiffusionGradients->GetComponent(num,0);
       grad[1]=this->DiffusionGradients->GetComponent(num,1);
       grad[2]=this->DiffusionGradients->GetComponent(num,2);
     } else {
       vtkErrorMacro("Gradient number is out of range");
     }
  }

//----------------------------------------------------------------------------
//
int vtkTeemEstimateDiffusionTensor::RequestInformation(
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  int scalarType = VTK_INT;
  vtkInformation *inScalarInfo =
    vtkDataObject::GetActiveFieldInformation(inInfo,
      vtkDataObject::FIELD_ASSOCIATION_POINTS,
      vtkDataSetAttributes::SCALARS);
  if (inScalarInfo)
    {
    scalarType = inScalarInfo->Get(vtkDataObject::FIELD_ARRAY_TYPE());
    }
  // We always want to output input scalars Type
  // We output one scalar components: baseline (for legacy issues)
  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, scalarType, 1);

  vtkInformation* baselineOutInfo = outputVector->GetInformationObject(1);
  vtkInformation* averageDWIOutInfo = outputVector->GetInformationObject(2);
  vtkDataObject::SetPointDataActiveScalarInfo(baselineOutInfo, scalarType, 1);
  vtkDataObject::SetPointDataActiveScalarInfo(averageDWIOutInfo, scalarType, 1);

  return 1;

}

//----------------------------------------------------------------------------
// Replace superclass Execute with a function that allocates tensors
// as well as scalars.  This gets called before multithreader starts
// (after which we can't allocate, for example in ThreadedExecute).
// Note we return to the regular pipeline at the end of this function.
int vtkTeemEstimateDiffusionTensor::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //Check inputs numbertenEstimateMethodLLS
  if (inData == NULL)
    {
    vtkErrorMacro("Input with DWIs has not been assigned");
    return 0;
    }

  //Check if this input is multicomponent and match the number of  gradients
  int ncomp = inData->GetPointData()->GetScalars()->GetNumberOfComponents();
  if (ncomp != this->NumberOfGradients)
    {
    vtkErrorMacro("The input has to have a number of components equal to the number of gradients");
    return 0;
    }

  // set extent so we know how many tensors to allocate
  output->SetExtent(this->GetUpdateExtent());

  // allocate output tensors
  vtkFloatArray* data = vtkFloatArray::New();
  int* dims = output->GetDimensions();
  vtkDebugMacro("Allocating output tensors, dims " << dims[0] <<" " << dims[1] << " " << dims[2]);
  data->SetNumberOfComponents(9);
  data->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  output->GetPointData()->SetTensors(data);
  data->Delete();

  // Allocate baseline and averageDWI images
  vtkImageData* baseline = 0;
  vtkImageData* averageDWI = 0;
  vtkInformation* baselineOutInfo = outputVector->GetInformationObject(1);
  vtkInformation* averageDWIOutInfo = outputVector->GetInformationObject(2);

  baseline = vtkImageData::SafeDownCast(
    baselineOutInfo->Get(vtkDataObject::DATA_OBJECT()));
  averageDWI = vtkImageData::SafeDownCast(
    averageDWIOutInfo->Get(vtkDataObject::DATA_OBJECT()));
  baseline->SetExtent(this->GetUpdateExtent());
  averageDWI->SetExtent(this->GetUpdateExtent());
  baseline->AllocateScalars(baselineOutInfo);
  averageDWI->AllocateScalars(averageDWIOutInfo);
  baseline->GetPointData()->GetScalars()->SetName("Baseline");
  averageDWI->GetPointData()->GetScalars()->SetName("AverageDWI");

  // make sure our gradient matrix is up to date
  //This update is not thread safe and it has to be performed outside
  // the threaded part.
  // if the user has transformed the coordinate system
  this->TransformDiffusionGradients();

  // Rescale gradients to account for multiple b-Values.
  //This is done in a different array to preserve the original gradients set by the user
  this->RescaleGradients();

  // jump back into normal pipeline: call standard superclass method here
  //Do not jump to do the proper allocation of output data
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkTeemEstimateDiffusionTensorExecute(vtkTeemEstimateDiffusionTensor *self,
                                           vtkImageData *inData,
                                           T * inPtr,
                                           vtkImageData *outData,
                                           T * outPtr,
                                           int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  vtkIdType inIncX, inIncY, inIncZ;
  vtkIdType outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int numInputs;
  double *dwi;
  double averageDWI;
  int numDWI;
  vtkDataArray *outTensors;
  float outT[3][3];
  int ptId;

  T * baselinePtr = NULL;
  T * averageDWIPtr = NULL;
  Nrrd *ngrad =NULL;
  Nrrd *nbmat =NULL;
  //Creating new nrrd arrays
  ngrad  = nrrdNew();
  nbmat = nrrdNew();

  // Get information to march through output tensor data
  outTensors = self->GetOutput()->GetPointData()->GetTensors();

  // Set Ten Context
  tenEstimateContext *tec = tenEstimateContextNew();
  if (self->SetTenContext(tec,ngrad,nbmat)) {
    cout<<"TenContext cannot be set. Bailing out"<<endl;
    tenEstimateContextNix(tec);
    nrrdNuke(nbmat);
    nrrdNix(ngrad);
    return;
  }

  // changed from arrays to pointers
  vtkIdType *outInc;
  int *outFullUpdateExt;
  outInc = self->GetOutput()->GetIncrements();
  outFullUpdateExt = self->GetUpdateExtent(); //We are only working over the update extent
  ptId = ((outExt[0] - outFullUpdateExt[0]) * outInc[0]
         + (outExt[2] - outFullUpdateExt[2]) * outInc[1]
         + (outExt[4] - outFullUpdateExt[4]) * outInc[2]);

  // Get pointer to Baseline and AverageDWI Images
  baselinePtr = (T *) self->GetBaseline()->GetScalarPointerForExtent(outExt);
  averageDWIPtr = (T *) self->GetAverageDWI()->GetScalarPointerForExtent(outExt);

  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2];
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)(outData->GetNumberOfScalarComponents()*
                           (maxZ+1)*(maxY+1)/50.0);
  target++;

  // Get increments to march through image data
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  numInputs = inData->GetNumberOfScalarComponents();
  dwi = new double[numInputs];

  double _ten[7];

  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
      for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++)
        {
          if (!id)
            {
              if (!(count%target))
                {
                  self->UpdateProgress(count/(50.0*target));
                }
              count++;
            }
          for (idxX = 0; idxX <= maxX; idxX++)
            {
             // create tensor from combination of gradient inputs
             averageDWI = 0.0;
             numDWI =0;
             for (int k=0; k< numInputs; k++)
             {
               dwi[k] = (double) inPtr[k];
               if (self->GetBValues()->GetValue(k) > 1)
                 {
                 averageDWI += dwi[k];
                 numDWI++;
                 }
             }
             // Set dwi to context
             //Main method
              tenEstimate1TensorSingle_d(tec,_ten, dwi);
              outT[0][0] = _ten[1];
              outT[0][1] = outT[1][0] = _ten[2];
              outT[0][2] = outT[2][0] = _ten[3];
              outT[1][1] = _ten[4];
              outT[1][2] = outT[2][1] = _ten[5];
              outT[2][2] = _ten[6];

              // Pixel operation
              outTensors->SetTuple(ptId,(float *)outT);
              // copy no diffusion data through for scalars
              *outPtr = (T) tec->estimatedB0;

              // Copy B0 and DWI
             *baselinePtr = (T) tec->estimatedB0;
             if (numDWI > 0)
                *averageDWIPtr = (T) (averageDWI/numDWI);
              else
                *averageDWIPtr = (T) 0;

              // Ideally we should saved the Chisquare error of the fitting.
              // this is really relevant information
              //*outPtr = (T) tec->errorDwi;

              inPtr += numInputs;
              ptId ++;
              outPtr++;
              baselinePtr++;
              averageDWIPtr++;
            }
          outPtr += outIncY;
          ptId += outIncY;
          baselinePtr += outIncY;
          averageDWIPtr += outIncY;
          inPtr += inIncY;
         }
      outPtr += outIncZ;
      ptId += outIncZ;
      baselinePtr += outIncZ;
      averageDWIPtr += outIncZ;
      inPtr += inIncZ;
    }
  delete [] dwi;
  // Delete Context
  tenEstimateContextNix(tec);
  nrrdNix(ngrad);
  nrrdNuke(nbmat);
}

//----------------------------------------------------------------------------
// To accomodate different b-values we might have to rescale the gradients
void vtkTeemEstimateDiffusionTensor::RescaleGradients()
{
  unsigned int Ngrads = this->DiffusionGradients->GetNumberOfTuples();
  // Reset rescaled diffusion grandients array before rescaling
  this->RescaledDiffusionGradients->DeepCopy(this->DiffusionGradients);
  double *data = (double *) this->DiffusionGradients->GetVoidPointer(0);
  double *data_r = (double *) this->RescaledDiffusionGradients->GetVoidPointer(0);
  double factor;
  for (unsigned int i=0; i < Ngrads; i++)
    {
    factor =  sqrt(this->BValues->GetValue(i)/this->MaxB);
    data_r[0] = data[0] * factor;
    data_r[1] = data[1] * factor;
    data_r[2] = data[2] * factor;
    data += 3;
    data_r += 3;
    }
}

//----------------------------------------------------------------------------
int vtkTeemEstimateDiffusionTensor::SetGradientsToContext(tenEstimateContext *tec,Nrrd *ngrad, Nrrd *nbmat)
{
  char *err = NULL;
  const int type = nrrdTypeDouble;
  size_t size[2];
  size[0] = 3;
  size[1] = this->RescaledDiffusionGradients->GetNumberOfTuples();
  double *data = (double *) this->RescaledDiffusionGradients->GetVoidPointer(0);
  if(nrrdWrap_nva(ngrad ,data,type,2,size)) {
    biffAdd(NRRD, err);
    sprintf(err,"%s:",this->GetClassName());
    return 1;
  }

  //tenEstimateGradientsSet(tec,ngrad,maxB,!this->knownB0);

  if (tenBMatrixCalc(nbmat,ngrad) ) {
    biffAdd(NRRD, err);
    sprintf(err,"%s:",this->GetClassName());
    return 1;
  }

  tenEstimateBMatricesSet(tec,nbmat,this->MaxB,!this->knownB0);
  tec->knownB0 = this->knownB0;
  return 0;
}

//----------------------------------------------------------------------------
int vtkTeemEstimateDiffusionTensor
::SetTenContext(  tenEstimateContext *tec,Nrrd* ngrad, Nrrd* nbmat )
{
    tec->progress = AIR_TRUE;

    // Set gradients
    if (this->SetGradientsToContext(tec,ngrad, nbmat)) {
      vtkErrorMacro("Error setting gradient into tenEstimateContext. Bailing out");
      return 1;
    }

    int EE = 0;
    int verbose = 0;
    char *err;

    if (!EE) tenEstimateVerboseSet(tec, verbose);
    if (!EE) EE |= tenEstimateMethodSet(tec, this->EstimationMethod);
    if (!EE) EE |= tenEstimateValueMinSet(tec, this->MinimumSignalValue);

    switch(this->EstimationMethod) {
    case tenEstimateMethodLLS:
        tec->recordErrorLogDwi = AIR_TRUE;
        /* tec->recordErrorDwi = AIR_TRUE; */
      break;
    case tenEstimateMethodNLS:
        tec->recordErrorDwi = AIR_TRUE;
      break;
    case tenEstimateMethodWLS:
      if (!EE) tec->WLSIterNum = this->NumberOfWLSIterations;
      tec->recordErrorDwi = AIR_TRUE;
      break;
    /*
    case tenEstimateMethodMLE:
      if (this->Sigma < 0.0)) {
        vtkErrorMacro("Noise sigma has to be positive >=0.0");
        return 1;
      }
      if (!EE) EE |= tenEstimateSigmaSet(tec, this->Sigma);
      tec->recordErrorDwi = AIR_TRUE;
      break;
     */
    }

    if (!EE) tec->negEvalShift = this->ShiftNegativeEigenvalues;

    // Do not set any threshold for the mask. Do that later
    if (!EE) EE |= tenEstimateThresholdSet(tec, 0, 1);
    if (!EE) EE |= tenEstimateUpdate(tec);

    if (EE) {
      err=biffGetDone(TEN);
      fprintf(stderr, "%s: trouble setting up estimation:\n%s\n",
      this->GetClassName(), err);
      return 1;
    }

return 0;
}

//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkTeemEstimateDiffusionTensor::ThreadedExecute(vtkImageData *inData,
                                              vtkImageData *outData,
                                              int outExt[6], int id)
{
  void *inPtrs;
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  vtkDebugMacro("in threaded execute, " << this->GetNumberOfInputConnections(0) << " inputconnections ");

  // Loop through to fill input pointer array
  inPtrs = inData->GetScalarPointerForExtent(outExt);

  // call Execute method to handle all data at the same time
  switch (inData->GetScalarType())
    {
      vtkTemplateMacro(vtkTeemEstimateDiffusionTensorExecute(this,
                        inData, static_cast<VTK_TT*>(inPtrs),
                        outData, static_cast<VTK_TT*>(outPtr),
                        outExt, id));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}
