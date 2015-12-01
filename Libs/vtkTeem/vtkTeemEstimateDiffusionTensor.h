/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkTeemEstimateDiffusionTensor.h,v $
  Date:      $Date: 2007/04/09 08:10:16 $
  Version:   $Revision: 1.3.2.1 $

=========================================================================auto=*/

#ifndef __vtkTeemEstimateDiffusionTensor_h
#define __vtkTeemEstimateDiffusionTensor_h

#include "vtkTeemConfigure.h"
#include "vtkThreadedImageAlgorithm.h"
#include "vtkDoubleArray.h"
#include "vtkTransform.h"
#include <vtkVersion.h>
#include "teem/nrrd.h"

/* avoid name conflicts with symbols from python */
#undef ECHO
#undef B0

#include "teem/ten.h"

class VTK_Teem_EXPORT vtkTeemEstimateDiffusionTensor : public vtkThreadedImageAlgorithm
{
 public:
  static vtkTeemEstimateDiffusionTensor *New();
  vtkTypeMacro(vtkTeemEstimateDiffusionTensor,vtkThreadedImageAlgorithm);

  ///
  /// The number of gradients is the same as the number of input
  /// diffusion ImageDatas this filter will require.
  void SetNumberOfGradients(int num);
  vtkGetMacro(NumberOfGradients,int);

  ///
  /// Set the 3-vectors describing the gradient directions
  void SetDiffusionGradient(int num, double gradient[3])
    {
    this->DiffusionGradients->SetTuple(num,gradient);
    this->Modified();
    }
  void SetDiffusionGradient(int num, double g0, double g1, double g2)
    {
      this->DiffusionGradients->SetComponent(num,0,g0);
      this->DiffusionGradients->SetComponent(num,1,g1);
      this->DiffusionGradients->SetComponent(num,2,g2);
      this->Modified();
    }
  void SetDiffusionGradients(vtkDoubleArray *grad);
  vtkGetObjectMacro(DiffusionGradients,vtkDoubleArray);

  ///
  /// Get the 3-vectors describing the gradient directions
  void GetDiffusionGradient(int num,double grad[3]);

  /// the following look messy but are copied from vtkSetGet.h,
  /// just adding the num parameter we need.

  void SetBValue(int num,double b)
   {
     this->BValues->SetValue(num,b);
     this->CalculateMaxB();
     this->Modified();
   }
  void SetBValues(vtkDoubleArray *bValues);
  vtkGetObjectMacro(BValues,vtkDoubleArray);

  /// Description
  /// need to calculate max B (using GetRange) outside threaded execute
  void CalculateMaxB();
  vtkSetMacro(MaxB, double);
  vtkGetMacro(MaxB, double);

  ///
  /// Get Baseline Image
  vtkImageData* GetBaseline();

  ///
  /// Get Average of all DWI images
  vtkImageData* GetAverageDWI();


  enum
    {
      tenEstimateMethodUnknown,  /* 0 */
      tenEstimateMethodLLS,      /* 1 */
      tenEstimateMethodWLS,      /* 2 */
      tenEstimateMethodNLS,      /* 3 */
      tenEstimateMethodLast
    };
   //Description
  vtkGetMacro(EstimationMethod,int);
  vtkSetMacro(EstimationMethod,int);
  void SetEstimationMethodToLLS() {
    this->SetEstimationMethod(tenEstimateMethodLLS);
  };
  void SetEstimationMethodToNLS() {
    this->SetEstimationMethod(tenEstimateMethodNLS);
  };
  void SetEstimationMethodToWLS() {
    this->SetEstimationMethod(tenEstimateMethodWLS);
  };

  vtkGetMacro(MinimumSignalValue,double);
  vtkSetMacro(MinimumSignalValue,double);

  /// Description
  /// Transformation of the tensors (for RAS coords, for example)
  /// The gradient vectors are multiplied by this matrix
  vtkSetObjectMacro(Transform, vtkTransform);
  vtkGetObjectMacro(Transform, vtkTransform);

  ///
  /// Internal class use only
  void TransformDiffusionGradients();
  void RescaleGradients();
  int SetGradientsToContext ( tenEstimateContext *tec,Nrrd *ngrad, Nrrd *nbmat);
  int SetTenContext(  tenEstimateContext *tec,Nrrd *ngrad, Nrrd *nbmat);

  ///
  /// Flag to shift eigenvalues upwards to that smallest one is non-negative
  /// (negEvalShift in Teem's Ten)
  vtkSetMacro(ShiftNegativeEigenvalues,int);
  vtkGetMacro(ShiftNegativeEigenvalues,int);

 protected:
  vtkTeemEstimateDiffusionTensor();
  ~vtkTeemEstimateDiffusionTensor();
  vtkTeemEstimateDiffusionTensor(const vtkTeemEstimateDiffusionTensor&);
  void operator=(const vtkTeemEstimateDiffusionTensor&);
  void PrintSelf(ostream& os, vtkIndent indent);

  int NumberOfGradients;

  vtkDoubleArray *BValues;
  vtkDoubleArray *DiffusionGradients;
  vtkDoubleArray *RescaledDiffusionGradients;
  /// Maximum of the B values
  double MaxB;


  /// for transforming tensors
  vtkTransform *Transform;

  /// Method
  int EstimationMethod;

  /// Minimum detectable value
  double MinimumSignalValue;

  /// Noise variance (useful for MLE)
  double Sigma;

  /// Matrices for LS fitting
  int knownB0;


  ///
  /// Flag to shift eigenvalues upwards to that smallest one is non-negative
  /// (negEvalShift in Teem's Ten)
  int ShiftNegativeEigenvalues;

  ///
  int NumberOfWLSIterations;

  virtual int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
        int extent[6], int id);

  /// We override this in order to allocate output tensors
  /// before threading happens.  This replaces the superclass
  /// vtkImageAlgorithm's RequestData function.
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);
};

#endif
