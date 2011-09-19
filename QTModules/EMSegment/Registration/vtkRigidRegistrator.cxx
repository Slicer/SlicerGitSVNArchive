#define ITK_LEAN_AND_MEAN
#include "vtkRigidRegistrator.h"
#include "vtkObjectFactory.h"
#include "itkImage.h"
#include "itkVTKImageImport.h"
#include "vtkITKUtility.h"
#include "vtkImageExport.h"
#include "itkMultiResolutionImageRegistrationMethod.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkNormalizedCorrelationImageToImageMetric.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkRealTimeClock.h"
#include "itkPixelTraits.h"
#include "vtkImageCast.h"
#include "vtkTypeTraits.h"
#include "itkVersorRigid3DTransform.h"
#include "itkCenteredVersorTransformInitializer.h"
#include "itkVersorRigid3DTransformOptimizer.h"
#include "vtkRegistratorTypeTraits.h"
#include "vtkImageChangeInformation.h"
#include "vtkImagePermute.h"
#include "itkImageFileWriter.h"

vtkCxxRevisionMacro(vtkRigidRegistrator, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkRigidRegistrator);

//
//  The following section of code implements a Command observer
//  that will monitor the evolution of the registration process.
//
#include "itkCommand.h"
#include <iomanip>
template <class TOptimizer>
class CommandIterationUpdate : public itk::Command
{
public:
  typedef CommandIterationUpdate   Self;
  typedef itk::Command             Superclass;
  typedef itk::SmartPointer<Self>  Pointer;
  itkNewMacro( Self );
protected:
  CommandIterationUpdate() {};
public:
  typedef TOptimizer                 OptimizerType;
  typedef const OptimizerType   *    OptimizerPointer;

  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *)caller, event);
  }

  // this should be specialized later...
  void Execute(const itk::Object * object, const itk::EventObject & event)
  {
    OptimizerPointer optimizer =
      dynamic_cast< OptimizerPointer >( object );
    if( ! itk::IterationEvent().CheckEvent( &event ) )
    {
      return;
    }
    std::cerr << "   " << std::setw(7) << std::right << std::setfill('.')
              << optimizer->GetCurrentIteration();
    std::cerr << std::setw(20) << std::right << std::setfill('.')
              << optimizer->GetValue();
    std::cerr << std::setw(17) << std::right << std::setfill('.')
              << optimizer->GetCurrentStepLength();
    std::cerr << std::endl;
    std::cerr << "              "
              << optimizer->GetCurrentPosition() << std::endl;
  }
};

template <class TRegistration, class TMetric, class TOptimizer>
class CommandStartLevelUpdate : public itk::Command
{
public:
  typedef  CommandStartLevelUpdate   Self;
  typedef  itk::Command              Superclass;
  typedef itk::SmartPointer<Self>    Pointer;
  itkNewMacro( Self );
protected:
  CommandStartLevelUpdate()
  {
    this->m_SamplingRatio = 0.33;
    this->m_NumberOfIterations = 10;
  }
  double m_SamplingRatio;
  int    m_NumberOfIterations;
public:
  typedef TRegistration                 RegistrationType;
  typedef RegistrationType   *          RegistrationPointer;
  typedef TMetric                       MetricType;
  typedef MetricType*                   MetricPointer;
  typedef TOptimizer                    OptimizerType;
  typedef OptimizerType*                OptimizerPointer;

  itkSetMacro(SamplingRatio, double);
  itkSetMacro(NumberOfIterations, int);

  void Execute(const itk::Object *itkNotUsed(caller), const itk::EventObject & itkNotUsed(event))
  {
    return;
  }

  // this should be specialized later...
  void Execute(itk::Object * object, const itk::EventObject & event)
  {
    RegistrationPointer registration =
      dynamic_cast< RegistrationPointer >( object );
    if( ! itk::IterationEvent().CheckEvent( &event ) )
      {
      return;
      }
    int level       = registration->GetCurrentLevel();
    int totalLevels = registration->GetNumberOfLevels();
    std::cerr << "   ### Starting registration level: "
              <<  level+1 << " of " << totalLevels << " ###" << std::endl;

    std::cerr << "       "
              << registration->GetOptimizer()->GetCurrentPosition()
              << std::endl;
    MetricPointer metric =
      dynamic_cast<MetricPointer>(registration->GetMetric());
    if (metric != NULL)
      {
      int numVoxels =
        registration->GetFixedImagePyramid()->GetOutput(level)->
        GetLargestPossibleRegion().GetNumberOfPixels();

      double samplingRatio =
        1.0 -
        registration->GetCurrentLevel() *
        (1.0 - m_SamplingRatio) / (registration->GetNumberOfLevels() - 1.0);

      metric->
        SetNumberOfSpatialSamples(static_cast<unsigned long>
                                  (samplingRatio * numVoxels));

      std::cerr << "       Image Size: " <<
        registration->GetFixedImagePyramid()->GetOutput(level)->
        GetLargestPossibleRegion().GetSize() << std::endl;
      std::cerr << "       Number of spatial samples: "
                << metric->GetNumberOfSpatialSamples()
                << " (" << floor(100 * samplingRatio) << "%)"
                << std::endl;
      }

    OptimizerPointer optimizer = dynamic_cast<OptimizerPointer>
      (registration->GetOptimizer());
    if (optimizer)
      {
      if (registration->GetCurrentLevel() == 0)
        {
        optimizer->SetMaximumStepLength(4.0);
        optimizer->SetMinimumStepLength(1.0);
        }
      else
        {
        optimizer->SetMaximumStepLength(optimizer->GetCurrentStepLength());
        optimizer->
          SetMinimumStepLength(optimizer->GetMinimumStepLength() / 10.0);
        }

      optimizer->SetNumberOfIterations
        (this->m_NumberOfIterations * (registration->GetNumberOfLevels() -
                                       registration->GetCurrentLevel()));

      std::cerr << "       Max Iterations: "
                << optimizer->GetNumberOfIterations()
                << std::endl;
      std::cerr << "       Min/Max Step Length: "
                << optimizer->GetMinimumStepLength()
                << " / "
                << optimizer->GetMaximumStepLength()
                << std::endl;
      }
  }
};

//----------------------------------------------------------------------------
vtkRigidRegistrator::
vtkRigidRegistrator()
{
  this->FixedImage  = NULL;
  this->MovingImage = NULL;

  this->FixedIJKToXYZ  = NULL;
  this->MovingIJKToXYZ = NULL;

  this->Transform   = vtkTransform::New();
  this->Transform->Identity();

  this->NumberOfIterations = 0;

  this->IntensityInterpolationType  = vtkRigidRegistrator::Linear;
  this->TransformInitializationType = vtkRigidRegistrator::ImageCenters;

  this->ImageToImageMetric   = vtkRigidRegistrator::MutualInformation;
  this->MetricComputationSamplingRatio = 1.0;
}

//----------------------------------------------------------------------------
vtkRigidRegistrator::
~vtkRigidRegistrator()
{
  this->SetFixedImage(NULL);
  this->SetMovingImage(NULL);
  this->SetFixedIJKToXYZ(NULL);
  this->SetMovingIJKToXYZ(NULL);
  this->Transform->Delete();
  this->Transform = NULL;
}

//----------------------------------------------------------------------------
void
vtkRigidRegistrator::
PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Transform->PrintSelf(os, indent);
  os << indent << "NumberOfIterations: " << this->NumberOfIterations
     << std::endl;
  os << indent << "MetricComputationSamplingRatio: "
     << this->MetricComputationSamplingRatio << std::endl;
  os << indent << "ImageToImageMetric: "
     << GetStringFromMetricType(this->ImageToImageMetric);
  os << indent << "InterpolationType: "
     << GetStringFromInterpolationType(this->IntensityInterpolationType);
  os << indent << "InitializationType: "
     << GetStringFromTransformInitializationType(this->TransformInitializationType)
     << std::endl;
}

//----------------------------------------------------------------------------
const char*
vtkRigidRegistrator::
GetStringFromMetricType(MetricType id)
{
  switch (id)
    {
    case (vtkRigidRegistrator::MutualInformation):
      return "MutualInformation";
    case (vtkRigidRegistrator::CrossCorrelation):
      return "CrossCorrelation";
    case (vtkRigidRegistrator::MeanSquaredError):
      return "MeanSquaredError";
    default:
      return "Unknown";
    };
}

//----------------------------------------------------------------------------
const char*
vtkRigidRegistrator::
GetStringFromInterpolationType(InterpolationType id)
{
  switch (id)
    {
    case (vtkRigidRegistrator::NearestNeighbor):
      return "NearestNeighbor";
    case (vtkRigidRegistrator::Linear):
      return "Linear";
    case (vtkRigidRegistrator::Cubic):
      return "Cubic";
    default:
      return "Unknown";
    };
}

//----------------------------------------------------------------------------
const char*
vtkRigidRegistrator::
GetStringFromTransformInitializationType(InitializationType id)
{
  switch (id)
    {
    case (vtkRigidRegistrator::Identity):
      return "Identity";
    case (vtkRigidRegistrator::CentersOfMass):
      return "CentersOfMass";
    case (vtkRigidRegistrator::ImageCenters):
      return "ImageCenters";
    default:
      return "Unknown";
    };
}

//----------------------------------------------------------------------------
void
vtkRigidRegistrator::
ComputeReorientationInformation(const vtkMatrix4x4* IJKToXYZ,
                                int*    filteredAxesForPermuteFilter,
                                double* originForChangeInformationFilter,
                                double* spacingForChangeInformationFilter)
{
  // origin is easy...
  originForChangeInformationFilter[0] = (*IJKToXYZ)[0][3];
  originForChangeInformationFilter[1] = (*IJKToXYZ)[1][3];
  originForChangeInformationFilter[2] = (*IJKToXYZ)[2][3];

  // figure out spacing and permutation.  Assumes one nonzero entry
  // per row/column of directions matrix.
  for (int c = 0; c < 3; ++c)
    {
    for (int r = 0; r < 3; ++r)
      {
      double t = (*IJKToXYZ)[r][c];
      if (t != 0)
        {
        filteredAxesForPermuteFilter[r]      = c;
        spacingForChangeInformationFilter[r] = t;
        break;
        }
      }
    }
}

//----------------------------------------------------------------------------
template <class TVoxel>
void
vtkRigidRegistrator::
RegisterImagesInternal3()
{
  //
  // Deal with orientation.  Permute images and setup origin and
  // spacing so that both images are measured in XYZ basis vectors
  // with only spacing and origin information.  This way ITK will do
  // registration in XYZ coordinates.
  //
  int     filteredAxesForPermuteFilter[3];
  double  originForChangeInformationFilter[3];
  double  spacingForChangeInformationFilter[3];

  // fixed ------
  vtkMatrix4x4* IJKToXYZMatrixFixed = vtkMatrix4x4::New();
  IJKToXYZMatrixFixed->Identity();
  if (this->FixedIJKToXYZ != NULL)
    {
    IJKToXYZMatrixFixed->DeepCopy(this->FixedIJKToXYZ);
    }
  vtkRigidRegistrator::
    ComputeReorientationInformation(IJKToXYZMatrixFixed,
                                    filteredAxesForPermuteFilter,
                                    originForChangeInformationFilter,
                                    spacingForChangeInformationFilter);

  vtkImagePermute* permuteFixedImage = vtkImagePermute::New();
  vtkImageChangeInformation* changeInformationFixedImage =
    vtkImageChangeInformation::New();

  permuteFixedImage->SetInput(this->FixedImage);
  permuteFixedImage->SetFilteredAxes(filteredAxesForPermuteFilter);

  changeInformationFixedImage->SetInput(permuteFixedImage->GetOutput());
  changeInformationFixedImage->
    SetOutputSpacing(spacingForChangeInformationFilter);
  changeInformationFixedImage->
    SetOutputOrigin(originForChangeInformationFilter);

  // moving ------
  vtkMatrix4x4* IJKToXYZMatrixMoving = vtkMatrix4x4::New();
  IJKToXYZMatrixMoving->Identity();
  if (this->MovingIJKToXYZ != NULL)
    {
    IJKToXYZMatrixMoving->DeepCopy(this->MovingIJKToXYZ);
    }
  vtkRigidRegistrator::
    ComputeReorientationInformation(IJKToXYZMatrixMoving,
                                    filteredAxesForPermuteFilter,
                                    originForChangeInformationFilter,
                                    spacingForChangeInformationFilter);

  vtkImagePermute* permuteMovingImage = vtkImagePermute::New();
  vtkImageChangeInformation* changeInformationMovingImage =
    vtkImageChangeInformation::New();

  permuteMovingImage->SetInput(this->MovingImage);
  permuteMovingImage->SetFilteredAxes(filteredAxesForPermuteFilter);

  changeInformationMovingImage->SetInput(permuteMovingImage->GetOutput());
  changeInformationMovingImage->
    SetOutputSpacing(spacingForChangeInformationFilter);
  changeInformationMovingImage->
    SetOutputOrigin(originForChangeInformationFilter);

  //
  // create vtk --> itk pipelines
  //

  typedef itk::Image<TVoxel, 3>                 ITKImageType;
  typedef itk::VTKImageImport<ITKImageType>     ImageImportType;

  // fixed image ------
  vtkImageCast* fixedImageCaster              = vtkImageCast::New();
  fixedImageCaster->SetInput(changeInformationFixedImage->GetOutput());
  fixedImageCaster->
    SetOutputScalarType(vtkTypeTraits<TVoxel>::VTKTypeID());
  vtkImageExport* fixedImageVTKToITKExporter  = vtkImageExport::New();
  fixedImageVTKToITKExporter->SetInput(fixedImageCaster->GetOutput());

  typename ImageImportType::Pointer fixedImageITKImporter =
    ImageImportType::New();
  ConnectPipelines(fixedImageVTKToITKExporter, fixedImageITKImporter);
  fixedImageITKImporter->Update();

  // moving image ------
  vtkImageCast*   movingImageCaster           = vtkImageCast::New();
  movingImageCaster->SetInput(changeInformationMovingImage->GetOutput());
  movingImageCaster->
    SetOutputScalarType(vtkTypeTraits<TVoxel>::VTKTypeID());
  vtkImageExport* movingImageVTKToITKExporter = vtkImageExport::New();
  movingImageVTKToITKExporter->SetInput(movingImageCaster->GetOutput());

  typename ImageImportType::Pointer movingImageITKImporter =
    ImageImportType::New();
  ConnectPipelines(movingImageVTKToITKExporter, movingImageITKImporter);
  movingImageITKImporter->Update();

  //
  // set up registration class
  //

  typedef typename itk::MultiResolutionPyramidImageFilter
    <ITKImageType, ITKImageType>   ImagePyramidType;
  typename ImagePyramidType::Pointer fixedImagePyramid =
    ImagePyramidType::New();
  typename ImagePyramidType::Pointer movingImagePyramid =
    ImagePyramidType::New();
  typedef typename itk::MultiResolutionImageRegistrationMethod
    <ITKImageType, ITKImageType> MultiResolutionRegistrationType;
  typename MultiResolutionRegistrationType::Pointer multiResRegistration =
    MultiResolutionRegistrationType::New();

  // set images
  multiResRegistration->SetFixedImage(fixedImageITKImporter->GetOutput());
  multiResRegistration->SetMovingImage(movingImageITKImporter->GetOutput());
  multiResRegistration->
    SetFixedImageRegion(fixedImageITKImporter->GetOutput()->
                        GetLargestPossibleRegion());

  fixedImagePyramid->SetNumberOfLevels(3);
  fixedImagePyramid->SetStartingShrinkFactors(4);
  movingImagePyramid->SetNumberOfLevels(3);
  movingImagePyramid->SetStartingShrinkFactors(4);
  multiResRegistration->SetFixedImagePyramid(fixedImagePyramid);
  multiResRegistration->SetMovingImagePyramid(movingImagePyramid);
  multiResRegistration->SetNumberOfLevels(3);

  //
  // set up metric
  switch (this->ImageToImageMetric)
    {
    case vtkRigidRegistrator::MutualInformation:
      {
      typedef itk::MattesMutualInformationImageToImageMetric<
        ITKImageType, ITKImageType>   MMetricType;
      typename MMetricType::Pointer    metric  = MMetricType::New();
      metric->SetNumberOfHistogramBins(50);
      metric->
        SetNumberOfSpatialSamples
        (static_cast<unsigned int>(multiResRegistration->GetFixedImage()->
                                   GetLargestPossibleRegion().
                                   GetNumberOfPixels() *
                                   this->MetricComputationSamplingRatio));
      metric->ReinitializeSeed(0);
      multiResRegistration->SetMetric(metric);
      std::cerr << "   Metric: MMI" << std::endl;
      std::cerr << "   Sampling Ratio: "
                << this->MetricComputationSamplingRatio << std::endl;
      break;
      }
    case vtkRigidRegistrator::CrossCorrelation:
      {
      typedef itk::NormalizedCorrelationImageToImageMetric<
        ITKImageType, ITKImageType>   MMetricType;
      typename MMetricType::Pointer    metric  = MMetricType::New();
      multiResRegistration->SetMetric(metric);
      std::cerr << "   Metric: NCC" << std::endl;
      std::cerr << "   Sampling Ratio: 1 (NOT IMPLEMENTED FOR NCC) "
                << this->MetricComputationSamplingRatio << std::endl;
      break;
      }
    case vtkRigidRegistrator::MeanSquaredError:
      {
      typedef itk::MeanSquaresImageToImageMetric<
        ITKImageType, ITKImageType>   MMetricType;
      typename MMetricType::Pointer    metric  = MMetricType::New();
      multiResRegistration->SetMetric(metric);
      std::cerr << "   Metric: MSE" << std::endl;

      break;
      }
    default:
      vtkErrorMacro("Unknown metric type: " << this->ImageToImageMetric);
      return;
    };

  //
  // set up interpolator
  switch (this->IntensityInterpolationType)
    {
    case vtkRigidRegistrator::NearestNeighbor:
      {
      typedef itk::NearestNeighborInterpolateImageFunction<
        ITKImageType,
        double          >    InterpolatorType;
      typename InterpolatorType::Pointer interpolator  =
        InterpolatorType::New();
      multiResRegistration->SetInterpolator(interpolator);
      std::cerr << "   Interpolation: Nearest neighbor" << std::endl;
      }
      break;

    case vtkRigidRegistrator::Linear:
      {
      typedef itk::LinearInterpolateImageFunction<
        ITKImageType,
        double          >    InterpolatorType;
      typename InterpolatorType::Pointer
        interpolator  = InterpolatorType::New();
      multiResRegistration->SetInterpolator(interpolator);
      std::cerr << "   Interpolation: Linear" << std::endl;
      }
      break;

    default:
      vtkErrorMacro(<< "Unknown interpolation type: "
                    << this->IntensityInterpolationType);
      return;
    };

  //
  // setup transform
  typedef itk::VersorRigid3DTransform< double >        TransformType;
  TransformType::Pointer  transform =                  TransformType::New();
  transform->SetIdentity();

  typedef
    itk::CenteredVersorTransformInitializer<ITKImageType, ITKImageType>
    TransformInitializerType;

  typename TransformInitializerType::Pointer transformInitializer =
    TransformInitializerType::New();
  transformInitializer->SetTransform(transform);
  transformInitializer->SetFixedImage(fixedImageITKImporter->GetOutput());
  transformInitializer->SetMovingImage(movingImageITKImporter->GetOutput());
  transformInitializer->InitializeTransform();

  if (this->TransformInitializationType == CentersOfMass)
  {
    transformInitializer->MomentsOn();
    std::cerr << "   Initialization: Moments" << std::endl;
  }
  else if (this->TransformInitializationType == ImageCenters)
  {
    transformInitializer->GeometryOn();
    std::cerr << "   Initialization: Image centers" << std::endl;
  }
  transformInitializer->InitializeTransform();

  multiResRegistration->SetTransform(transform);
  multiResRegistration->SetInitialTransformParameters(transform->GetParameters());

  std::cerr << "     After Initializtation: " << std::endl;
  transform->Print(std::cerr, 5);

  //
  // setup optomizer
  typedef itk::VersorRigid3DTransformOptimizer       OptimizerType;
  OptimizerType::Pointer optimizer =                 OptimizerType::New();
  multiResRegistration->SetOptimizer(optimizer);

  typedef OptimizerType::ScalesType       OptimizerScalesType;
  OptimizerScalesType optimizerScales(6);
  const double translationScale = 1.0 / 1000.0;

  double               initialStepLength               = 1.0;
  double               relaxationFactor                = 0.6;
  double               minimumStepLength               = 0.001;

  // matrix
  optimizerScales[0]  = 1.0;
  optimizerScales[1]  = 1.0;
  optimizerScales[2]  = 1.0;
  // translation
  optimizerScales[3] = translationScale;
  optimizerScales[4] = translationScale;
  optimizerScales[5] = translationScale;

  optimizer->SetScales( optimizerScales );
  optimizer->SetRelaxationFactor( relaxationFactor );
  optimizer->SetMaximumStepLength( initialStepLength );
  optimizer->SetMinimumStepLength( minimumStepLength );
  optimizer->SetNumberOfIterations( this->NumberOfIterations );
  std::cerr << "   Max Iterations: "
            << this->NumberOfIterations << std::endl;

  //
  // set up command observer
  CommandIterationUpdate<OptimizerType>::Pointer observer =
    CommandIterationUpdate<OptimizerType>::New();
  optimizer->AddObserver( itk::IterationEvent(), observer );

  // this is hacked for now!!!
  typename CommandStartLevelUpdate
    <MultiResolutionRegistrationType,
    itk::MattesMutualInformationImageToImageMetric
    <ITKImageType, ITKImageType>, OptimizerType >::Pointer
    startLevelCommand =
    CommandStartLevelUpdate<MultiResolutionRegistrationType,
    itk::MattesMutualInformationImageToImageMetric<ITKImageType, ITKImageType>, OptimizerType >
    ::New();
  startLevelCommand->SetSamplingRatio(this->MetricComputationSamplingRatio);
  startLevelCommand->SetNumberOfIterations(this->NumberOfIterations);
  multiResRegistration->AddObserver(itk::IterationEvent(), startLevelCommand);

  //
  // everything should be set up, run the registration
  //

  //
  // debug: write input images to disk
#ifdef NOT_EVER_DEFINED
  typedef itk::ImageFileWriter<ITKImageType> ITKWriterType;
  typename ITKWriterType::Pointer writerFixed = ITKWriterType::New();
  writerFixed->SetInput(fixedImageITKImporter->GetOutput());
  writerFixed->SetFileName("/tmp/Fixed.nhdr");
  writerFixed->Update();
  typename ITKWriterType::Pointer writerMoving = ITKWriterType::New();
  writerMoving->SetInput(movingImageITKImporter->GetOutput());
  writerMoving->SetFileName("/tmp/Moving.nhdr");
  writerMoving->Update();
#endif // NOT_EVER_DEFINED

  try
    {
    itk::RealTimeClock::Pointer clock = itk::RealTimeClock::New();
    //std::cerr << "  Starting registration..." << std::endl;
    std::cerr << "   Iteration         Image Match        Step Size"
              << std::endl;
#if ITK_VERSION_MAJOR < 4
    const double timeStart = clock->GetTimeStamp();
#else
    const double timeStart = clock->GetTimeInSeconds();
#endif

    multiResRegistration->StartRegistration();

#if ITK_VERSION_MAJOR < 4
    const double timeStop = clock->GetTimeStamp();
#else
    const double timeStop = clock->GetTimeInSeconds();
#endif
    const double timeLength = (timeStop - timeStart);
    std::cerr << "  DONE, time = " << timeLength << std::endl;
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << err << std::endl;
    throw;
    }

  //std::cerr << "After Registration: " << std::endl;
  //transform->Print(std::cerr, 0);

  //
  // copy transform from itk back to this vtk class
  //

  typename TransformType::MatrixType itkMatrix       = transform->GetMatrix();
  typename TransformType::OutputVectorType itkOffset = transform->GetOffset();
  vtkMatrix4x4* vtkMatrix = vtkMatrix4x4::New();
  vtkMatrix->Identity();
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      vtkMatrix->SetElement(i, j, itkMatrix(i,j));
    }
    vtkMatrix->SetElement(i, 3, itkOffset[i]);
  }

  this->Transform->SetMatrix(vtkMatrix);

  //
  // clean up memory
  //

  fixedImageCaster->Delete();
  movingImageCaster->Delete();
  fixedImageVTKToITKExporter->Delete();
  movingImageVTKToITKExporter->Delete();
  vtkMatrix->Delete();
  changeInformationFixedImage->Delete();
  changeInformationMovingImage->Delete();
  permuteFixedImage->Delete();
  permuteMovingImage->Delete();
  IJKToXYZMatrixFixed->Delete();
  IJKToXYZMatrixMoving->Delete();

  this->Modified();
}

//----------------------------------------------------------------------------
template <class TFixedImageVoxel, class TMovingImageVoxel>
void
vtkRigidRegistrator::
RegisterImagesInternal2()
{
  //
  // First, find the smallest voxel type that can represent both fixed
  // and moving voxel type.  The, convert that type to one that we
  // have instantiated (currently short, unsigned short, float, and
  // double) in order to reduce code bloat.
  typedef itk::JoinTraits<TFixedImageVoxel, TMovingImageVoxel> TraitsType;
  typedef typename TraitsType::ValueType             CommonImageVoxelType;
  typedef
    typename RegistrationVoxelTypeTraits<CommonImageVoxelType>::
    RegistrationVoxelType
    RegistrationVoxelType;
  this->RegisterImagesInternal3<RegistrationVoxelType>();
}

//----------------------------------------------------------------------------
template <class TFixedImageVoxel>
void
vtkRigidRegistrator::
RegisterImagesInternal1()
{
  switch (this->MovingImage->GetScalarType())
    {
    vtkTemplateMacro((RegisterImagesInternal2<TFixedImageVoxel,VTK_TT>()));
    }
}

//----------------------------------------------------------------------------
void
vtkRigidRegistrator::
RegisterImages()
{
  switch (this->FixedImage->GetScalarType())
    {
    vtkTemplateMacro((RegisterImagesInternal1<VTK_TT>()));
    }
}


