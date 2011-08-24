/*=========================================================================

  Program:   Registration stand-alone
  Module:    $RCSfile: $
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

=========================================================================*/

#include "AffineRegistrationCLP.h"

#include "itkOrientImageFilter.h"
#include "itkImageFileWriter.h"
#include "itkTransformFileReader.h"
#include "itkTransformFileWriter.h"

#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkImageRegistrationMethod.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkResampleImageFilter.h"
#include "itkBinomialBlurImageFilter.h"

#include "itkPluginUtilities.h"

#include "itkTimeProbesCollectorBase.h"

// Use an anonymous namespace to keep class types and function names
// from colliding when module is used as shared object module.  Every
// thing should be in an anonymous namespace except for the module
// entry point, e.g. main()
//
namespace
{

//  The following section of code implements a Command observer
//  used to monitor the evolution of the registration process.
//
class CommandIterationUpdate : public itk::Command
{
public:
  typedef  CommandIterationUpdate Self;
  typedef  itk::Command           Superclass;
  typedef itk::SmartPointer<Self> Pointer;
  itkNewMacro( Self );
protected:
  CommandIterationUpdate()
  {
  };
  itk::ProcessObject::Pointer m_Registration;
public:
  typedef itk::RegularStepGradientDescentOptimizer OptimizerType;
  typedef   const OptimizerType *                  OptimizerPointer;

  void SetRegistration( itk::ProcessObject *p)
  {
    m_Registration = p;
  }

  void Execute(itk::Object *caller, const itk::EventObject & event)
  {
    Execute( (const itk::Object *)caller, event);
  }

  void Execute(const itk::Object * object, const itk::EventObject & event)
  {
    OptimizerPointer optimizer =
      dynamic_cast<OptimizerPointer>( object );

    if( !(itk::IterationEvent().CheckEvent( &event ) ) )
      {
      return;
      }

    std::cout << optimizer->GetCurrentIteration() << "   ";
    std::cout << optimizer->GetCurrentStepLength() << "   ";
    std::cout << optimizer->GetValue() << std::endl;
    if( m_Registration )
      {
      m_Registration->UpdateProgress(
        static_cast<double>(optimizer->GetCurrentIteration() )
        / static_cast<double>(optimizer->GetNumberOfIterations() ) );
      }
  }

};

template <class T1, class T2>
int DoIt2( int argc, char * argv[], const T1 &, const T2 & )
{
  //
  // Command line processing
  //
  PARSE_ARGS;

  const    unsigned int ImageDimension = 3;
  typedef  T1                                        FixedPixelType; // ##
  typedef itk::Image<FixedPixelType, ImageDimension> FixedImageType; // ##

  typedef itk::ImageFileReader<FixedImageType>                   FixedFileReaderType;   // ##
  typedef itk::OrientImageFilter<FixedImageType, FixedImageType> FixedOrientFilterType; // ##

  typedef  T2                                         MovingPixelType; // ##
  typedef itk::Image<MovingPixelType, ImageDimension> MovingImageType; // ##

  typedef itk::ImageFileReader<MovingImageType>                    MovingFileReaderType;   // ##
  typedef itk::OrientImageFilter<MovingImageType, MovingImageType> MovingOrientFilterType; // ##

  typedef itk::MattesMutualInformationImageToImageMetric<FixedImageType, MovingImageType> MetricType;    // ##
  typedef itk::RegularStepGradientDescentOptimizer                                        OptimizerType;
  typedef itk::LinearInterpolateImageFunction<MovingImageType, double>                    InterpolatorType; // ##
  typedef itk::ImageRegistrationMethod<FixedImageType, MovingImageType>                   RegistrationType; // ##
  typedef itk::AffineTransform<double>                                                    TransformType;
  typedef OptimizerType::ScalesType                                                       OptimizerScalesType;
  typedef itk::ResampleImageFilter<MovingImageType, MovingImageType>                      ResampleType;             // ##
  typedef itk::LinearInterpolateImageFunction<MovingImageType, double>                    ResampleInterpolatorType; // ##
  typedef itk::ImageFileWriter<MovingImageType>                                           WriterType;               // ##
  typedef itk::ContinuousIndex<double, 3>                                                 ContinuousIndexType;

  // bool DoInitializeTransform = false;
  // int RandomSeed = 1234567;

  // Add a time probe
  itk::TimeProbesCollectorBase collector;

  // Read the fixed and moving volumes
  //
  //
  typename FixedFileReaderType::Pointer fixedReader = FixedFileReaderType::New();
  fixedReader->SetFileName( FixedImageFileName.c_str() );

  try
    {
    collector.Start( "Read fixed volume" );
    fixedReader->Update();
    collector.Stop( "Read fixed volume" );
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "Error Reading Fixed image: " << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
    }

  typename MovingFileReaderType::Pointer movingReader = MovingFileReaderType::New();
  movingReader->SetFileName( MovingImageFileName.c_str() );

  try
    {
    collector.Start( "Read moving volume" );
    movingReader->Update();
    collector.Stop( "Read moving volume" );
    }
  catch( itk::ExceptionObject & err )
    {
    std::cerr << "Error Reading Moving image: " << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
    }

  // user decide if the input images need to be smoothed

  // Reorient to axials to avoid issues with registration metrics not
  // transforming image gradients with the image orientation in
  // calculating the derivative of metric wrt transformation
  // parameters.
  //
  // Forcing image to be axials avoids this problem. Note, that
  // reorientation only affects the internal mapping from index to
  // physical coordinates.  The reoriented data spans the same
  // physical space as the original data.  Thus, the registration
  // transform calculated on the reoriented data is also the
  // transform forthe original un-reoriented data.

  typename FixedOrientFilterType::Pointer orientFixed = FixedOrientFilterType::New(); // ##
  itk::PluginFilterWatcher watchOrientFixed(orientFixed,   "Orient Fixed Image",  CLPProcessInformation,  1.0 / 5.0,
                                            0.0);
  orientFixed->UseImageDirectionOn();
  orientFixed->SetDesiredCoordinateOrientationToAxial();

  if( FixedImageSmoothingFactor != 0 )
    {
    typedef itk::BinomialBlurImageFilter<FixedImageType, FixedImageType> BinomialFixedType;
    typename BinomialFixedType::Pointer BinomialFixed = BinomialFixedType::New();
    BinomialFixed->SetInput(   fixedReader->GetOutput() );
    BinomialFixed->SetRepetitions( FixedImageSmoothingFactor * 2);
    itk::PluginFilterWatcher watchfilter(BinomialFixed, "Binomial Filter Fixed",  CLPProcessInformation, 1.0 / 5.0,
                                         1.0 / 5.0);
    BinomialFixed->Update();
    orientFixed->SetInput(BinomialFixed->GetOutput() );
    }
  else
    {
    orientFixed->SetInput(fixedReader->GetOutput() );
    }
  collector.Start( "Orient fixed volume" );
  orientFixed->Update();
  collector.Stop( "Orient fixed volume" );

  typename MovingOrientFilterType::Pointer orientMoving = MovingOrientFilterType::New(); // ##
  itk::PluginFilterWatcher watchOrientMoving(orientMoving,  "Orient Moving Image", CLPProcessInformation,  1.0 / 5.0,
                                             2.0 / 5.0);
  orientMoving->UseImageDirectionOn();
  orientMoving->SetDesiredCoordinateOrientationToAxial();

  if( MovingImageSmoothingFactor != 0 )
    {
    typedef itk::BinomialBlurImageFilter<MovingImageType,  MovingImageType> BinomialMovingType;
    typename BinomialMovingType::Pointer BinomialMoving = BinomialMovingType::New();
    BinomialMoving->SetInput(   movingReader->GetOutput() );
    BinomialMoving->SetRepetitions( MovingImageSmoothingFactor * 2);
    itk::PluginFilterWatcher watchfilter(BinomialMoving, "Binomial Filter Moving",  CLPProcessInformation, 1.0 / 5.0,
                                         3.0 / 5.0);
    BinomialMoving->Update();
    orientMoving->SetInput(BinomialMoving->GetOutput() );
    }
  else
    {
    orientMoving->SetInput(movingReader->GetOutput() );
    }

  collector.Start( "Orient moving volume" );
  orientMoving->Update();
  collector.Stop( "Orient moving volume" );

  // If an initial transform was specified, read it
  //
  //
  typedef itk::TransformFileReader TransformReaderType;
  TransformReaderType::Pointer initialTransform;

  if( InitialTransform != "" )
    {
    initialTransform = TransformReaderType::New();
    initialTransform->SetFileName( InitialTransform );
    try
      {
      initialTransform->Update();
      }
    catch( itk::ExceptionObject & err )
      {
      std::cerr << err << std::endl;
      return EXIT_FAILURE;
      }
    }

  // Set up the optimizer
  //
  //
  typename OptimizerType::Pointer      optimizer     = OptimizerType::New();
  optimizer->SetNumberOfIterations( Iterations );
  optimizer->SetMinimumStepLength( .0005 );
  optimizer->SetMaximumStepLength( 10.0 );
  optimizer->SetMinimize(true);

  typename TransformType::Pointer transform = TransformType::New();
  OptimizerScalesType scales( transform->GetNumberOfParameters() );
  scales.Fill( 1.0 );
  for( unsigned j = 9; j < 12; j++ )
    {
    scales[j] = 1.0 / vnl_math_sqr(TranslationScale);
    }
  optimizer->SetScales( scales );

  // Create the Command observer and register it with the optimizer.
  //
  typename CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
  optimizer->AddObserver( itk::IterationEvent(), observer );

  // Initialize the transform
  //
  //
  typename TransformType::InputPointType centerFixed;
  typename FixedImageType::RegionType::SizeType sizeFixed =
    orientFixed->GetOutput()->GetLargestPossibleRegion().GetSize();
  // Find the center
  ContinuousIndexType indexFixed;
  for( unsigned j = 0; j < 3; j++ )
    {
    indexFixed[j] = (sizeFixed[j] - 1) / 2.0;
    }
  orientFixed->GetOutput()->TransformContinuousIndexToPhysicalPoint( indexFixed, centerFixed );

  typename TransformType::InputPointType centerMoving;
  typename MovingImageType::RegionType::SizeType sizeMoving =
    orientMoving->GetOutput()->GetLargestPossibleRegion().GetSize();
  // Find the center
  ContinuousIndexType indexMoving;
  for( unsigned j = 0; j < 3; j++ )
    {
    indexMoving[j] = (sizeMoving[j] - 1) / 2.0;
    }
  orientMoving->GetOutput()->TransformContinuousIndexToPhysicalPoint( indexMoving, centerMoving );

  transform->SetCenter( centerFixed );
  transform->Translate(centerMoving - centerFixed);
  std::cout << "Centering transform: "; transform->Print( std::cout );

  // If an initial transformation was provided, then use it instead.
  // (Should this be instead of the centering transform or composed
  // with the centering transform.)
  //
  if( InitialTransform != ""
      && initialTransform->GetTransformList()->size() != 0 )
    {
    TransformReaderType::TransformType::Pointer initial
      = *(initialTransform->GetTransformList()->begin() );

    // most likely, the transform coming in is a subclass of
    // MatrixOffsetTransformBase
    typedef itk::MatrixOffsetTransformBase<double, 3, 3> DoubleMatrixOffsetType;
    typedef itk::MatrixOffsetTransformBase<float, 3, 3>  FloatMatrixOffsetType;

    DoubleMatrixOffsetType::Pointer da
      = dynamic_cast<DoubleMatrixOffsetType *>(initial.GetPointer() );
    FloatMatrixOffsetType::Pointer fa
      = dynamic_cast<FloatMatrixOffsetType *>(initial.GetPointer() );

    if( da )
      {
      transform->SetMatrix( da->GetMatrix() );
      transform->SetOffset( da->GetOffset() );
      }
    else if( fa )
      {
      vnl_matrix<double> t(3, 3);
      for( int i = 0; i < 3; ++i )
        {
        for( int j = 0; j < 3; ++j )
          {
          t.put(i, j, fa->GetMatrix().GetVnlMatrix().get(i, j) );
          }
        }

      transform->SetMatrix( t );
      transform->SetOffset( fa->GetOffset() );
      }
    else
      {
      std::cout << "Initial transform is an unsupported type." << std::endl;
      }

    std::cout << "Initial transform: "; transform->Print( std::cout );
    }

  // Set up the metric
  //
  typename MetricType::Pointer  metric        = MetricType::New();
  metric->SetNumberOfHistogramBins( HistogramBins );
  metric->SetNumberOfSpatialSamples( SpatialSamples );
  metric->ReinitializeSeed(123);

  // Create the interpolator
  //
  typename InterpolatorType::Pointer interpolator = InterpolatorType::New();

  // Set up the registration
  //
  typename RegistrationType::Pointer registration = RegistrationType::New();
  registration->SetTransform( transform );
  registration->SetInitialTransformParameters( transform->GetParameters() );
  registration->SetMetric( metric );
  registration->SetOptimizer( optimizer );
  registration->SetInterpolator( interpolator );
  registration->SetFixedImage( orientFixed->GetOutput() );
  registration->SetMovingImage( orientMoving->GetOutput() );

  // Force an iteration event to trigger a progress event
  observer->SetRegistration( registration );

  try
    {
    itk::PluginFilterWatcher watchRegistration(registration,
                                               "Registering",
                                               CLPProcessInformation,
                                               1.0 / 5.0, 4.0 / 5.0);
    collector.Start( "Register" );
    registration->Update();
    collector.Stop( "Register" );
    }
  catch( itk::ExceptionObject & err )
    {
    std::cout << err << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
    }
  catch( ... )
    {
    return EXIT_FAILURE;
    }

  transform->SetParameters( registration->GetLastTransformParameters() );

  if( OutputTransform != "" )
    {
    typedef itk::TransformFileWriter TransformWriterType;
    TransformWriterType::Pointer outputTransformWriter;

    outputTransformWriter = TransformWriterType::New();
    outputTransformWriter->SetFileName( OutputTransform );
    outputTransformWriter->SetInput( transform );
    try
      {
      outputTransformWriter->Update();
      }
    catch( itk::ExceptionObject & err )
      {
      std::cerr << err << std::endl;
      return EXIT_FAILURE;
      }
    }

  // Resample to the original coordinate frame (not the reoriented
  // axial coordinate frame) of the fixed image
  //
  if( ResampledImageFileName != "" )
    {
    typename ResampleType::Pointer resample = ResampleType::New();
    typename ResampleInterpolatorType::Pointer Interpolator = ResampleInterpolatorType::New();
    itk::PluginFilterWatcher watchResample(resample,
                                           "Resample",
                                           CLPProcessInformation,
                                           1.0 / 5.0, 4.0 / 5.0);

    resample->SetInput( movingReader->GetOutput() );
    resample->SetTransform( transform );
    resample->SetInterpolator( Interpolator );

    // Set the output sampling based on the fixed image.
    // ResampleImageFilter needs an image of the same type as the
    // moving image.
    typename MovingImageType::Pointer fixedInformation = MovingImageType::New();
    fixedInformation->CopyInformation( fixedReader->GetOutput() );
    resample->SetOutputParametersFromImage( fixedInformation );

    collector.Start( "Resample" );
    resample->Update();
    collector.Stop( "Resample" );

    typename WriterType::Pointer resampledWriter = WriterType::New();
    resampledWriter->SetFileName( ResampledImageFileName.c_str() );
    resampledWriter->SetInput( resample->GetOutput() );
    try
      {
      collector.Start( "Write volume" );
      resampledWriter->Write();
      collector.Stop( "Write volume" );
      }
    catch( itk::ExceptionObject & err )
      {
      std::cerr << err << std::endl;
      std::cerr << err << std::endl;
      return EXIT_FAILURE;
      }
    }

  // Report the time taken by the registration
  collector.Report();

  return EXIT_SUCCESS;
}

template <class T>
int DoIt( int argc, char * argv[], const T& targ)
{

  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     pixelType;
  itk::ImageIOBase::IOComponentType componentType;

  try
    {
    itk::GetImageType(MovingImageFileName, pixelType, componentType);

    // This filter handles all types

    switch( componentType )
      {
      case itk::ImageIOBase::CHAR:
      case itk::ImageIOBase::UCHAR:
      case itk::ImageIOBase::SHORT:
        return DoIt2( argc, argv, targ, static_cast<short>(0) );
        break;
      case itk::ImageIOBase::USHORT:
      case itk::ImageIOBase::INT:
        return DoIt2( argc, argv, targ, static_cast<int>(0) );
        break;
      case itk::ImageIOBase::UINT:
      case itk::ImageIOBase::ULONG:
        return DoIt2( argc, argv, targ, static_cast<unsigned long>(0) );
        break;
      case itk::ImageIOBase::LONG:
        return DoIt2( argc, argv, targ, static_cast<long>(0) );
        break;
      case itk::ImageIOBase::FLOAT:
        return DoIt2( argc, argv, targ, static_cast<float>(0) );
        break;
      case itk::ImageIOBase::DOUBLE:
        return DoIt2( argc, argv, targ, static_cast<float>(0) );
        break;
      case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
      default:
        std::cout << "unknown component type" << std::endl;
        break;
      }
    }
  catch( itk::ExceptionObject & excep )
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_FAILURE;
}

} // end of anonymous namespace

int main( int argc, char * argv[] )
{

  PARSE_ARGS;

  itk::ImageIOBase::IOPixelType     pixelType;
  itk::ImageIOBase::IOComponentType componentType;

  try
    {
    itk::GetImageType(FixedImageFileName, pixelType, componentType);

    // This filter handles all types

    switch( componentType )
      {
      case itk::ImageIOBase::CHAR:
      case itk::ImageIOBase::UCHAR:
      case itk::ImageIOBase::SHORT:
        return DoIt( argc, argv, static_cast<short>(0) );
        break;
      case itk::ImageIOBase::USHORT:
      case itk::ImageIOBase::INT:
        return DoIt( argc, argv, static_cast<int>(0) );
        break;
      case itk::ImageIOBase::UINT:
      case itk::ImageIOBase::ULONG:
        return DoIt( argc, argv, static_cast<unsigned long>(0) );
        break;
      case itk::ImageIOBase::LONG:
        return DoIt( argc, argv, static_cast<long>(0) );
        break;
      case itk::ImageIOBase::FLOAT:
        return DoIt( argc, argv, static_cast<float>(0) );
        break;
      case itk::ImageIOBase::DOUBLE:
        return DoIt( argc, argv, static_cast<float>(0) );
        break;
      case itk::ImageIOBase::UNKNOWNCOMPONENTTYPE:
      default:
        std::cout << "unknown component type" << std::endl;
        break;
      }
    }
  catch( itk::ExceptionObject & excep )
    {
    std::cerr << argv[0] << ": exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
