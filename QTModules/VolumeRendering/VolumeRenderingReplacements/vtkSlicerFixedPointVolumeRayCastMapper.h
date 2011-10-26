/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSlicerFixedPointVolumeRayCastMapper.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSlicerFixedPointVolumeRayCastMapper - A fixed point mapper for volumes
// .SECTION Description
// This is a software ray caster for rendering volumes in vtkImageData.
// It works with all input data types and up to four components. It performs
// composite or MIP rendering, and can be intermixed with geometric data.
// Space leaping is used to speed up the rendering process. In addition,
// calculation are performed in 15 bit fixed point precision. This mapper
// is threaded, and will interleave scan lines across processors.
//
// This mapper is a good replacement for vtkVolumeRayCastMapper EXCEPT:
//   - it does not do isosurface ray casting
//   - it does only interpolate before classify compositing
//   - it does only maximum scalar value MIP
//
// The vtkVolumeRayCastMapper CANNOT be used in these instances when a
// vtkSlicerFixedPointVolumeRayCastMapper can be used:
//   - if the data is not unsigned char or unsigned short
//   - if the data has more than one component
//
// This mapper handles all data type from unsigned char through double.
// However, some of the internal calcultions are performed in float and
// therefore even the full float range may cause problems for this mapper
// (both in scalar data values and in spacing between samples).
//
// Space leaping is performed by creating a sub-sampled volume. 4x4x4
// cells in the original volume are represented by a min, max, and
// combined gradient and flag value. The min max volume has three
// unsigned shorts per 4x4x4 group of cells from the original volume -
// one reprenting the minumum scalar index (the scalar value adjusted
// to fit in the 15 bit range), the maximum scalar index, and a
// third unsigned short which is both the maximum gradient opacity in
// the neighborhood (an unsigned char) and the flag that is filled
// in for the current lookup tables to indicate whether this region
// can be skipped.

// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkSlicerFixedPointVolumeRayCastMapper_h
#define __vtkSlicerFixedPointVolumeRayCastMapper_h

#include "vtkVolumeMapper.h"
#include "VolumeRenderingReplacementsExport.h"


#define VTKKW_FP_SHIFT       15
#define VTKKW_FPMM_SHIFT     17
#define VTKKW_FP_MASK        0x7fff
#define VTKKW_FP_SCALE       32767.0

class vtkMatrix4x4;
class vtkMultiThreader;
class vtkPlaneCollection;
class vtkRenderer;
class vtkTimerLog;
class vtkVolume;
class vtkTransform;
class vtkRenderWindow;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class vtkSlicerFixedPointVolumeRayCastMIPHelper;
class vtkSlicerFixedPointVolumeRayCastCompositeHelper;
class vtkSlicerFixedPointVolumeRayCastCompositeGOHelper;
class vtkSlicerFixedPointVolumeRayCastCompositeGOShadeHelper;
class vtkSlicerFixedPointVolumeRayCastCompositeShadeHelper;
class vtkDirectionEncoder;
class vtkEncodedGradientShader;
class vtkFiniteDifferenceGradientEstimator;
#include "vtkSlicerRayCastImageDisplayHelper.h"
class vtkSlicerFixedPointRayCastImage;

// Forward declaration needed for use by friend declaration below.
VTK_THREAD_RETURN_TYPE SlicerFixedPointVolumeRayCastMapper_CastRays( void *arg );

/// \ingroup Slicer_QtModules_VolumeRendering
class Q_SLICER_QTMODULES_VOLUMERENDERING_REPLACEMENTS_EXPORT vtkSlicerFixedPointVolumeRayCastMapper : public vtkVolumeMapper
{
public:
  //SLICERADD
    vtkGetMacro(ManualInteractive,int);
    vtkSetMacro(ManualInteractive,int);
    vtkBooleanMacro(ManualInteractive,int);
    vtkGetMacro(ManualInteractiveRate,double);
    vtkSetMacro(ManualInteractiveRate,double);

  //ENDSLICERADD

  static vtkSlicerFixedPointVolumeRayCastMapper *New();
  vtkTypeRevisionMacro(vtkSlicerFixedPointVolumeRayCastMapper,vtkVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Set/Get the distance between samples used for rendering
  // when AutoAdjustSampleDistances is off, or when this mapper
  // has more than 1 second allocated to it for rendering.
  vtkSetMacro( SampleDistance, float );
  vtkGetMacro( SampleDistance, float );

  // Description:
  // Set/Get the distance between samples when interactive rendering is happening.
  // In this case, interactive is defined as this volume mapper having less than 1
  // second allocated for rendering. When AutoAdjustSampleDistance is On, and the
  // allocated render time is less than 1 second, then this InteractiveSampleDistance
  // will be used instead of the SampleDistance above.
  vtkSetMacro( InteractiveSampleDistance, float );
  vtkGetMacro( InteractiveSampleDistance, float );

  // Description:
  // Sampling distance in the XY image dimensions. Default value of 1 meaning
  // 1 ray cast per pixel. If set to 0.5, 4 rays will be cast per pixel. If
  // set to 2.0, 1 ray will be cast for every 4 (2 by 2) pixels. This value
  // will be adjusted to meet a desired frame rate when AutoAdjustSampleDistances
  // is on.
  vtkSetClampMacro( ImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( ImageSampleDistance, float );

  // Description:
  // This is the minimum image sample distance allow when the image
  // sample distance is being automatically adjusted.
  vtkSetClampMacro( MinimumImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( MinimumImageSampleDistance, float );

  // Description:
  // This is the maximum image sample distance allow when the image
  // sample distance is being automatically adjusted.
  vtkSetClampMacro( MaximumImageSampleDistance, float, 0.1f, 100.0f );
  vtkGetMacro( MaximumImageSampleDistance, float );

  // Description:
  // If AutoAdjustSampleDistances is on, the the ImageSampleDistance
  // and the SampleDistance will be varied to achieve the allocated
  // render time of this prop (controlled by the desired update rate
  // and any culling in use). If this is an interactive render (more
  // than 1 frame per second) the SampleDistance will be increased,
  // otherwise it will not be altered (a binary decision, as opposed
  // to the ImageSampleDistance which will vary continuously).
  vtkSetClampMacro( AutoAdjustSampleDistances, int, 0, 1 );
  vtkGetMacro( AutoAdjustSampleDistances, int );
  vtkBooleanMacro( AutoAdjustSampleDistances, int );

  // Description:
  // Set/Get the number of threads to use. This by default is equal to
  // the number of available processors detected.
  void SetNumberOfThreads( int num );
  int GetNumberOfThreads();

  // Description:
  // If IntermixIntersectingGeometry is turned on, the zbuffer will be
  // captured and used to limit the traversal of the rays.
  vtkSetClampMacro( IntermixIntersectingGeometry, int, 0, 1 );
  vtkGetMacro( IntermixIntersectingGeometry, int );
  vtkBooleanMacro( IntermixIntersectingGeometry, int );

  // Description:
  // What is the image sample distance required to achieve the desired time?
  // A version of this method is provided that does not require the volume
  // argument since if you are using an LODProp3D you may not know this information.
  // If you use this version you must be certain that the ray cast mapper is
  // only used for one volume (and not shared among multiple volumes)
  float ComputeRequiredImageSampleDistance( float desiredTime,
                                            vtkRenderer *ren );
  float ComputeRequiredImageSampleDistance( float desiredTime,
                                            vtkRenderer *ren,
                                            vtkVolume *vol );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Initialize rendering for this volume.
  void Render( vtkRenderer *, vtkVolume * );

  unsigned int ToSlicerFixedPointPosition( float val );
  void ToSlicerFixedPointPosition( float in[3], unsigned int out[3] );
  unsigned int ToSlicerFixedPointDirection( float dir );
  void ToSlicerFixedPointDirection( float in[3], unsigned int out[3] );
  void FixedPointIncrement( unsigned int position[3], unsigned int increment[3] );
  void GetFloatTripleFromPointer( float v[3], float *ptr );
  void GetUIntTripleFromPointer( unsigned int v[3], unsigned int *ptr );
  void ShiftVectorDown( unsigned int in[3], unsigned int out[3] );
  int CheckMinMaxVolumeFlag( unsigned int pos[3], int c );
  int CheckMIPMinMaxVolumeFlag( unsigned int pos[3], int c, unsigned short maxIdx );

  void LookupColorUC( unsigned short *colorTable,
                      unsigned short *scalarOpacityTable,
                      unsigned short index,
                      unsigned char  color[4] );
  void LookupDependentColorUC( unsigned short *colorTable,
                               unsigned short *scalarOpacityTable,
                               unsigned short index[4],
                               int            components,
                               unsigned char  color[4] );
  void LookupAndCombineIndependentColorsUC(
    unsigned short *colorTable[4],
    unsigned short *scalarOpacityTable[4],
    unsigned short index[4],
    float          weights[4],
    int            components,
    unsigned char  color[4] );
  int CheckIfCropped( unsigned int pos[3] );


  vtkGetObjectMacro( RenderWindow, vtkRenderWindow );
  vtkGetObjectMacro( MIPHelper, vtkSlicerFixedPointVolumeRayCastMIPHelper );
  vtkGetObjectMacro( CompositeHelper, vtkSlicerFixedPointVolumeRayCastCompositeHelper );
  vtkGetObjectMacro( CompositeGOHelper, vtkSlicerFixedPointVolumeRayCastCompositeGOHelper );
  vtkGetObjectMacro( CompositeGOShadeHelper, vtkSlicerFixedPointVolumeRayCastCompositeGOShadeHelper );
  vtkGetObjectMacro( CompositeShadeHelper, vtkSlicerFixedPointVolumeRayCastCompositeShadeHelper );
  vtkGetVectorMacro( TableShift, float, 4 );
  vtkGetVectorMacro( TableScale, float, 4 );
  vtkGetMacro( ShadingRequired, int );
  vtkGetMacro( GradientOpacityRequired, int );

  int             *GetRowBounds()                 {return this->RowBounds;}
  unsigned short  *GetColorTable(int c)           {return this->ColorTable[c];}
  unsigned short  *GetScalarOpacityTable(int c)   {return this->ScalarOpacityTable[c];}
  unsigned short  *GetGradientOpacityTable(int c) {return this->GradientOpacityTable[c];}
  vtkVolume       *GetVolume()                    {return this->Volume;}
  unsigned short **GetGradientNormal()            {return this->GradientNormal;}
  unsigned char  **GetGradientMagnitude()         {return this->GradientMagnitude;}
  unsigned short  *GetDiffuseShadingTable(int c)  {return this->DiffuseShadingTable[c];}
  unsigned short  *GetSpecularShadingTable(int c) {return this->SpecularShadingTable[c];}

  void ComputeRayInfo( int x, int y,
                       unsigned int pos[3],
                       unsigned int dir[3],
                       unsigned int *numSteps );

  void InitializeRayInfo( vtkVolume *vol );

  int ShouldUseNearestNeighborInterpolation( vtkVolume *vol );

  // Description:
  // Set / Get the underlying image object. One will be automatically
  // created - only need to set it when using from an AMR mapper which
  // renders multiple times into the same image.
  void SetRayCastImage( vtkSlicerFixedPointRayCastImage * );
  vtkGetObjectMacro( RayCastImage, vtkSlicerFixedPointRayCastImage  );

  int PerImageInitialization( vtkRenderer *, vtkVolume *, int,
                              double *, double *, int * );
  void PerVolumeInitialization( vtkRenderer *, vtkVolume * );
  void PerSubVolumeInitialization( vtkRenderer *, vtkVolume *, int );
  void RenderSubVolume();
  void DisplayRenderedImage( vtkRenderer *, vtkVolume * );
  void AbortRender();


protected:

    //SLICERADD
    int ManualInteractive;
    double ManualInteractiveRate;
  //ENDSLICERADD


  vtkSlicerFixedPointVolumeRayCastMapper();
  ~vtkSlicerFixedPointVolumeRayCastMapper();

  // The helper class that displays the image
  vtkSlicerRayCastImageDisplayHelper *ImageDisplayHelper;

  // The distance between sample points along the ray
  float                        SampleDistance;
  float                        InteractiveSampleDistance;

  // The distance between rays in the image
  float                        ImageSampleDistance;
  float                        MinimumImageSampleDistance;
  float                        MaximumImageSampleDistance;
  int                          AutoAdjustSampleDistances;

  // Saved values used to restore
  float                        OldSampleDistance;
  float                        OldImageSampleDistance;

  // Internal method for computing matrices needed during
  // ray casting
  void ComputeMatrices( double volumeOrigin[3],
                        double volumeSpacing[3],
                        int volumeExtent[6],
                        vtkRenderer  *ren,
                        vtkVolume    *vol );

  int ComputeRowBounds( vtkRenderer *ren,
                        int imageFlag, int rowBoundsFlag,
                        int volumeExtent[6]);

  void CaptureZBuffer( vtkRenderer *ren );

  friend VTK_THREAD_RETURN_TYPE SlicerFixedPointVolumeRayCastMapper_CastRays( void *arg );

  vtkMultiThreader  *Threader;

  vtkMatrix4x4   *PerspectiveMatrix;
  vtkMatrix4x4   *ViewToWorldMatrix;
  vtkMatrix4x4   *ViewToVoxelsMatrix;
  vtkMatrix4x4   *VoxelsToViewMatrix;
  vtkMatrix4x4   *WorldToVoxelsMatrix;
  vtkMatrix4x4   *VoxelsToWorldMatrix;

  vtkMatrix4x4   *VolumeMatrix;

  vtkTransform   *PerspectiveTransform;
  vtkTransform   *VoxelsTransform;
  vtkTransform   *VoxelsToViewTransform;

  // This object encapsulated the image and all related information
  vtkSlicerFixedPointRayCastImage *RayCastImage;

  int             *RowBounds;
  int             *OldRowBounds;

  float           *RenderTimeTable;
  vtkVolume      **RenderVolumeTable;
  vtkRenderer    **RenderRendererTable;
  int              RenderTableSize;
  int              RenderTableEntries;

  void             StoreRenderTime( vtkRenderer *ren, vtkVolume *vol, float t );
  float            RetrieveRenderTime( vtkRenderer *ren, vtkVolume *vol );
  float            RetrieveRenderTime( vtkRenderer *ren );

  int              IntermixIntersectingGeometry;

  float            MinimumViewDistance;

  vtkColorTransferFunction *SavedRGBFunction[4];
  vtkPiecewiseFunction     *SavedGrayFunction[4];
  vtkPiecewiseFunction     *SavedScalarOpacityFunction[4];
  vtkPiecewiseFunction     *SavedGradientOpacityFunction[4];
  int                       SavedColorChannels[4];
  float                     SavedScalarOpacityDistance[4];
  int                       SavedBlendMode;
  vtkImageData             *SavedParametersInput;
  vtkTimeStamp              SavedParametersMTime;

  vtkImageData             *SavedGradientsInput;
  vtkTimeStamp              SavedGradientsMTime;

  float                     SavedSampleDistance;


  unsigned short            ColorTable[4][32768*3];
  unsigned short            ScalarOpacityTable[4][32768];
  unsigned short            GradientOpacityTable[4][256];
  int                       TableSize[4];
  float                     TableScale[4];
  float                     TableShift[4];

  float                     GradientMagnitudeScale[4];
  float                     GradientMagnitudeShift[4];

  unsigned short           **GradientNormal;
  unsigned char            **GradientMagnitude;
  unsigned short            *ContiguousGradientNormal;
  unsigned char             *ContiguousGradientMagnitude;

  int                        NumberOfGradientSlices;

  vtkDirectionEncoder       *DirectionEncoder;

  vtkEncodedGradientShader  *GradientShader;

  vtkFiniteDifferenceGradientEstimator *GradientEstimator;

  unsigned short             DiffuseShadingTable [4][65536*3];
  unsigned short             SpecularShadingTable[4][65536*3];

  int                        ShadingRequired;
  int                        GradientOpacityRequired;

  vtkRenderWindow           *RenderWindow;
  vtkVolume                 *Volume;

  int           ClipRayAgainstVolume( float rayStart[3],
                                      float rayEnd[3],
                                      float rayDirection[3],
                                      double bounds[6] );

  int           UpdateColorTable( vtkVolume *vol );
  int           UpdateGradients( vtkVolume *vol );
  int           UpdateShadingTable( vtkRenderer *ren,
                                    vtkVolume *vol );
  void          UpdateCroppingRegions();

  void          ComputeGradients( vtkVolume *vol );

  int           ClipRayAgainstClippingPlanes( float  rayStart[3],
                                              float  rayEnd[3],
                                              int    numClippingPlanes,
                                              float *clippingPlanes );

  unsigned int  SlicerFixedPointCroppingRegionPlanes[6];
  unsigned int  CroppingRegionMask[27];

  // Get the ZBuffer value corresponding to location (x,y) where (x,y)
  // are indexing into the ImageInUse image. This must be converted to
  // the zbuffer image coordinates. Nearest neighbor value is returned.
  float         GetZBufferValue( int x, int y );

  vtkSlicerFixedPointVolumeRayCastMIPHelper              *MIPHelper;
  vtkSlicerFixedPointVolumeRayCastCompositeHelper        *CompositeHelper;
  vtkSlicerFixedPointVolumeRayCastCompositeGOHelper      *CompositeGOHelper;
  vtkSlicerFixedPointVolumeRayCastCompositeShadeHelper   *CompositeShadeHelper;
  vtkSlicerFixedPointVolumeRayCastCompositeGOShadeHelper *CompositeGOShadeHelper;

  // Some variables used for ray computation
  float ViewToVoxelsArray[16];
  float WorldToVoxelsArray[16];
  float VoxelsToWorldArray[16];

  double CroppingBounds[6];

  int NumTransformedClippingPlanes;
  float *TransformedClippingPlanes;

  double SavedSpacing[3];


  // Min Max structured used to do space leaping
  unsigned short *MinMaxVolume;
  int             MinMaxVolumeSize[4];
  vtkImageData   *SavedMinMaxInput;
  vtkTimeStamp    SavedMinMaxBuildTime;
  vtkTimeStamp    SavedMinMaxGradientTime;
  vtkTimeStamp    SavedMinMaxFlagTime;

  void            UpdateMinMaxVolume( vtkVolume *vol );
  void            FillInMaxGradientMagnitudes( int fullDim[3],
                                               int smallDim[3] );

private:
  vtkSlicerFixedPointVolumeRayCastMapper(const vtkSlicerFixedPointVolumeRayCastMapper&);  // Not implemented.
  void operator=(const vtkSlicerFixedPointVolumeRayCastMapper&);  // Not implemented.
};


inline unsigned int vtkSlicerFixedPointVolumeRayCastMapper::ToSlicerFixedPointPosition( float val )
{
  return static_cast<unsigned int>(val * VTKKW_FP_SCALE + 0.5);
}

inline void vtkSlicerFixedPointVolumeRayCastMapper::ToSlicerFixedPointPosition( float in[3], unsigned int out[3] )
{
  out[0] = static_cast<unsigned int>(in[0] * VTKKW_FP_SCALE + 0.5);
  out[1] = static_cast<unsigned int>(in[1] * VTKKW_FP_SCALE + 0.5);
  out[2] = static_cast<unsigned int>(in[2] * VTKKW_FP_SCALE + 0.5);
}

inline unsigned int vtkSlicerFixedPointVolumeRayCastMapper::ToSlicerFixedPointDirection( float dir )
{
  return ((dir<0.0)?
          (static_cast<unsigned int>(-dir * VTKKW_FP_SCALE + 0.5)):
          (0x80000000+static_cast<unsigned int>(dir*VTKKW_FP_SCALE + 0.5)));
}

inline void vtkSlicerFixedPointVolumeRayCastMapper::ToSlicerFixedPointDirection( float in[3], unsigned int out[3] )
{
  out[0] = ((in[0]<0.0)?
            (static_cast<unsigned int>(-in[0] * VTKKW_FP_SCALE + 0.5)):
            (0x80000000+
             static_cast<unsigned int>(in[0]*VTKKW_FP_SCALE + 0.5)));
  out[1] = ((in[1]<0.0)?
            (static_cast<unsigned int>(-in[1] * VTKKW_FP_SCALE + 0.5)):
            (0x80000000+
             static_cast<unsigned int>(in[1]*VTKKW_FP_SCALE + 0.5)));
  out[2] = ((in[2]<0.0)?
            (static_cast<unsigned int>(-in[2] * VTKKW_FP_SCALE + 0.5)):
            (0x80000000+
             static_cast<unsigned int>(in[2]*VTKKW_FP_SCALE + 0.5)));
}

inline void vtkSlicerFixedPointVolumeRayCastMapper::FixedPointIncrement( unsigned int position[3], unsigned int increment[3] )
{
  if ( increment[0]&0x80000000 )
    {
    position[0] += (increment[0]&0x7fffffff);
    }
  else
    {
    position[0] -= increment[0];
    }
  if ( increment[1]&0x80000000 )
    {
    position[1] += (increment[1]&0x7fffffff);
    }
  else
    {
    position[1] -= increment[1];
    }
  if ( increment[2]&0x80000000 )
    {
    position[2] += (increment[2]&0x7fffffff);
    }
  else
    {
    position[2] -= increment[2];
    }
}


inline void vtkSlicerFixedPointVolumeRayCastMapper::GetFloatTripleFromPointer( float v[3], float *ptr )
{
  v[0] = *(ptr);
  v[1] = *(ptr+1);
  v[2] = *(ptr+2);
}

inline void vtkSlicerFixedPointVolumeRayCastMapper::GetUIntTripleFromPointer( unsigned int v[3], unsigned int *ptr )
{
  v[0] = *(ptr);
  v[1] = *(ptr+1);
  v[2] = *(ptr+2);
}

inline void vtkSlicerFixedPointVolumeRayCastMapper::ShiftVectorDown( unsigned int in[3],
                                                       unsigned int out[3] )
{
  out[0] = in[0] >> VTKKW_FP_SHIFT;
  out[1] = in[1] >> VTKKW_FP_SHIFT;
  out[2] = in[2] >> VTKKW_FP_SHIFT;
}

inline int vtkSlicerFixedPointVolumeRayCastMapper::CheckMinMaxVolumeFlag( unsigned int mmpos[3], int c )
{
  unsigned int offset =
    this->MinMaxVolumeSize[3] *
    ( mmpos[2]*this->MinMaxVolumeSize[0]*this->MinMaxVolumeSize[1] +
      mmpos[1]*this->MinMaxVolumeSize[0] +
      mmpos[0] ) + c;

  return ((*(this->MinMaxVolume + 3*offset + 2))&0x00ff);
}

inline int vtkSlicerFixedPointVolumeRayCastMapper::CheckMIPMinMaxVolumeFlag( unsigned int mmpos[3], int c,
                                                                       unsigned short maxIdx )
{
  unsigned int offset =
    this->MinMaxVolumeSize[3] *
    ( mmpos[2]*this->MinMaxVolumeSize[0]*this->MinMaxVolumeSize[1] +
      mmpos[1]*this->MinMaxVolumeSize[0] +
      mmpos[0] ) + c;

  if ( (*(this->MinMaxVolume + 3*offset + 2)&0x00ff) )
    {
    return ( *(this->MinMaxVolume + 3*offset + 1) > maxIdx );
    }
  else
    {
    return 0;
    }
}

inline void vtkSlicerFixedPointVolumeRayCastMapper::LookupColorUC( unsigned short *colorTable,
                                                     unsigned short *scalarOpacityTable,
                                                     unsigned short index,
                                                     unsigned char  color[4] )
{
  unsigned short alpha = scalarOpacityTable[index];
  color[0] = static_cast<unsigned char>
    ((colorTable[3*index  ]*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
  color[1] = static_cast<unsigned char>
    ((colorTable[3*index+1]*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
  color[2] = static_cast<unsigned char>
    ((colorTable[3*index+2]*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
  color[3] = static_cast<unsigned char>(alpha>>(VTKKW_FP_SHIFT - 8));
}

inline void vtkSlicerFixedPointVolumeRayCastMapper::LookupDependentColorUC( unsigned short *colorTable,
                                                              unsigned short *scalarOpacityTable,
                                                              unsigned short index[4],
                                                              int            components,
                                                              unsigned char  color[4] )
{
  unsigned short alpha;
  switch ( components )
    {
    case 2:
      alpha = scalarOpacityTable[index[1]];
      color[0] = static_cast<unsigned char>
        ((colorTable[3*index[0]  ]*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
      color[1] = static_cast<unsigned char>
        ((colorTable[3*index[0]+1]*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
      color[2] = static_cast<unsigned char>
        ((colorTable[3*index[0]+2]*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
      color[3] = static_cast<unsigned char>(alpha>>(VTKKW_FP_SHIFT - 8));
      break;
    case 4:
      alpha = scalarOpacityTable[index[3]];
      color[0] = static_cast<unsigned char>((index[0]*alpha + 0x7fff)>>VTKKW_FP_SHIFT );
      color[1] = static_cast<unsigned char>((index[1]*alpha + 0x7fff)>>VTKKW_FP_SHIFT );
      color[2] = static_cast<unsigned char>((index[2]*alpha + 0x7fff)>>VTKKW_FP_SHIFT );
      color[3] = static_cast<unsigned char>(alpha>>(VTKKW_FP_SHIFT - 8));
      break;
    }
}


inline void vtkSlicerFixedPointVolumeRayCastMapper::LookupAndCombineIndependentColorsUC( unsigned short *colorTable[4],
                                                                           unsigned short *scalarOpacityTable[4],
                                                                           unsigned short  index[4],
                                                                           float           weights[4],
                                                                           int             components,
                                                                           unsigned char   color[4] )
{
  unsigned int tmp[4] = {0,0,0,0};

  for ( int i = 0; i < components; i++ )
    {
    unsigned short alpha = static_cast<unsigned short>(scalarOpacityTable[i][index[i]]*weights[i]);
    tmp[0] += static_cast<unsigned char>(((colorTable[i][3*index[i]  ])*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
    tmp[1] += static_cast<unsigned char>(((colorTable[i][3*index[i]+1])*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
    tmp[2] += static_cast<unsigned char>(((colorTable[i][3*index[i]+2])*alpha + 0x7fff)>>(2*VTKKW_FP_SHIFT - 8));
    tmp[3] += static_cast<unsigned char>(alpha>>(VTKKW_FP_SHIFT - 8));
    }

  color[0] = (tmp[0]>255)?(255):(tmp[0]);
  color[1] = (tmp[1]>255)?(255):(tmp[1]);
  color[2] = (tmp[2]>255)?(255):(tmp[2]);
  color[3] = (tmp[3]>255)?(255):(tmp[3]);

}

inline int vtkSlicerFixedPointVolumeRayCastMapper::CheckIfCropped( unsigned int pos[3] )
{
  int idx;

  if ( pos[2] < this->SlicerFixedPointCroppingRegionPlanes[4] )
    {
    idx = 0;
    }
  else if ( pos[2] > this->SlicerFixedPointCroppingRegionPlanes[5] )
    {
    idx = 18;
    }
  else
    {
    idx = 9;
    }

  if ( pos[1] >= this->SlicerFixedPointCroppingRegionPlanes[2] )
    {
    if ( pos[1] > this->SlicerFixedPointCroppingRegionPlanes[3] )
      {
      idx += 6;
      }
    else
      {
      idx += 3;
      }
    }

  if ( pos[0] >= this->SlicerFixedPointCroppingRegionPlanes[0] )
    {
    if ( pos[0] > this->SlicerFixedPointCroppingRegionPlanes[1] )
      {
      idx += 2;
      }
    else
      {
      idx += 1;
      }
    }

  return !(this->CroppingRegionFlags&this->CroppingRegionMask[idx]);
}

#endif
