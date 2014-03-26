/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkSlicerOpenGLVolumeTextureMapper3D.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSlicerOpenGLVolumeTextureMapper3D.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkVolumeProperty.h"
#include "vtkTransform.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"
#include "vtkVolumeTextureMapper3D_OneComponentNoShadeFP.h"
#include "vtkVolumeTextureMapper3D_OneComponentShadeFP.h"
#include "vtkVolumeTextureMapper3D_TwoDependentNoShadeFP.h"
#include "vtkVolumeTextureMapper3D_TwoDependentShadeFP.h"
#include "vtkVolumeTextureMapper3D_FourDependentNoShadeFP.h"
#include "vtkVolumeTextureMapper3D_FourDependentShadeFP.h"

#define PrintError(S)                                                           \
  {                                                                             \
  GLenum errorCode;                                                             \
  if ( (errorCode = glGetError()) != GL_NO_ERROR )                              \
    {                                                                           \
    cout << S << endl;                                                          \
    cout << "ERROR" << endl;                                                    \
    switch (errorCode)                                                          \
      {                                                                         \
      case GL_INVALID_ENUM: cout << "invalid enum" << endl; break;              \
      case GL_INVALID_VALUE: cout << "invalid value" << endl; break;            \
      case GL_INVALID_OPERATION: cout << "invalid operation" << endl; break;    \
      case GL_STACK_OVERFLOW: cout << "stack overflow" << endl; break;          \
      case GL_STACK_UNDERFLOW: cout << "stack underflow" << endl; break;        \
      case GL_OUT_OF_MEMORY: cout << "out of memory" << endl; break;            \
      default: cout << "unknown error" << endl;                                 \
      }                                                                         \
    }}

//#ifndef VTK_IMPLEMENT_MESA_CXX
vtkCxxRevisionMacro(vtkSlicerOpenGLVolumeTextureMapper3D, "$Revision: 1.7.2.2 $");
vtkStandardNewMacro(vtkSlicerOpenGLVolumeTextureMapper3D);
//#endif



vtkSlicerOpenGLVolumeTextureMapper3D::vtkSlicerOpenGLVolumeTextureMapper3D()
{
  this->Initialized                  =  0;
  this->Volume1Index                 =  0;
  this->Volume2Index                 =  0;
  this->Volume3Index                 =  0;
  this->ColorLookupIndex             =  0;
  this->RenderWindow                 = NULL;
}

vtkSlicerOpenGLVolumeTextureMapper3D::~vtkSlicerOpenGLVolumeTextureMapper3D()
{
}

// Release the graphics resources used by this texture.
void vtkSlicerOpenGLVolumeTextureMapper3D::ReleaseGraphicsResources(vtkWindow
                                                                *renWin)
{
  if (( this->Volume1Index || this->Volume2Index ||
        this->Volume3Index || this->ColorLookupIndex) && renWin)
    {
    static_cast<vtkRenderWindow *>(renWin)->MakeCurrent();
#ifdef GL_VERSION_1_1
    // free any textures
    this->DeleteTextureIndex( &this->Volume1Index );
    this->DeleteTextureIndex( &this->Volume2Index );
    this->DeleteTextureIndex( &this->Volume3Index );
    this->DeleteTextureIndex( &this->ColorLookupIndex );
    this->DeleteTextureIndex( &this->AlphaLookupIndex );
#endif
    }
  this->Volume1Index     = 0;
  this->Volume2Index     = 0;
  this->Volume3Index     = 0;
  this->ColorLookupIndex = 0;
  this->RenderWindow     = NULL;
  this->Modified();
}

void vtkSlicerOpenGLVolumeTextureMapper3D::Render(vtkRenderer *ren, vtkVolume *vol)
{
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NO_METHOD )
    {
    vtkErrorMacro( "required extensions not supported" );
    return;
    }

  ren->GetRenderWindow()->MakeCurrent();

  if ( !this->Initialized )
    {
    this->Initialize(ren->GetRenderWindow());
    }

    // If the timer is not accurate enough, set it to a small
  // time so that it is not zero
  if ( this->TimeToDraw == 0.0 )
    {
    this->TimeToDraw = 0.0001;
    }

  //adjust ray steps based on requrestd frame rate
  this->AdaptivePerformanceControl();

  // Start the timer now for more accurate overall rendering time (needed for performance control)
  this->Timer->StartTimer();

  vtkMatrix4x4       *matrix = vtkMatrix4x4::New();
  vtkPlaneCollection *clipPlanes;
  vtkPlane           *plane;
  int                numClipPlanes = 0;
  double             planeEquation[4];


  // build transformation
  vol->GetMatrix(matrix);
  matrix->Transpose();

  glPushAttrib(GL_ENABLE_BIT   |
               GL_COLOR_BUFFER_BIT   |
               GL_STENCIL_BUFFER_BIT |
               GL_DEPTH_BUFFER_BIT   |
               GL_POLYGON_BIT        |
               GL_TEXTURE_BIT);

  int i;

  // Use the OpenGL clip planes
  clipPlanes = this->ClippingPlanes;
  if ( clipPlanes )
    {
    numClipPlanes = clipPlanes->GetNumberOfItems();
    if (numClipPlanes > 6)
      {
      vtkErrorMacro(<< "OpenGL guarantees only 6 additional clipping planes");
      }

    for (i = 0; i < numClipPlanes; i++)
      {
      glEnable(static_cast<GLenum>(GL_CLIP_PLANE0+i));

      plane = static_cast<vtkPlane *>(clipPlanes->GetItemAsObject(i));

      planeEquation[0] = plane->GetNormal()[0];
      planeEquation[1] = plane->GetNormal()[1];
      planeEquation[2] = plane->GetNormal()[2];
      planeEquation[3] = -(planeEquation[0]*plane->GetOrigin()[0]+
                           planeEquation[1]*plane->GetOrigin()[1]+
                           planeEquation[2]*plane->GetOrigin()[2]);
      glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0+i),planeEquation);
      }
    }



  // insert model transformation
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glMultMatrixd(matrix->Element[0]);

  glColor4f( 1.0, 1.0, 1.0, 1.0 );

  // Turn lighting off - the polygon textures already have illumination
  glDisable( GL_LIGHTING );

  switch ( this->RenderMethod )
    {
    case vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD:
      this->RenderNV(ren,vol);
      break;
    case vtkSlicerVolumeTextureMapper3D::FRAGMENT_PROGRAM_METHOD:
      this->RenderFP(ren,vol);
      break;
    }

  // pop transformation matrix
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  matrix->Delete();
  glPopAttrib();

  glFlush();
  glFinish();

  this->Timer->StopTimer();

  this->TimeToDraw = static_cast<float>(this->Timer->GetElapsedTime());
}

void vtkSlicerOpenGLVolumeTextureMapper3D::AdaptivePerformanceControl()
{
  //do automatic performance control
  if(this->Framerate <= 0.01f)
    this->Framerate = 1.0f;

  float targetTime = 1.0/this->Framerate;

  if (fabs(targetTime - this->TimeToDraw) < 0.1*targetTime)
    return;

  float spacing[3];
  this->GetVolumeSpacing(spacing);

  float minSampleDistance = spacing[0];
  minSampleDistance = minSampleDistance < spacing[1] ? minSampleDistance : spacing[1];
  minSampleDistance = minSampleDistance < spacing[2] ? minSampleDistance : spacing[2];
  minSampleDistance /= 2; //each slice blending maximumlly 32 polygons

  float maxSampleDistance = spacing[0];
  maxSampleDistance = maxSampleDistance > spacing[1] ? maxSampleDistance : spacing[1];
  maxSampleDistance = maxSampleDistance > spacing[2] ? maxSampleDistance : spacing[2];
  maxSampleDistance *= 16;

  this->SampleDistance *= 1.5f*this->TimeToDraw/targetTime;

//  printf("%f %f %f\n", this->Framerate, this->TimeToDraw, 1.0/this->TimeToDraw);
  // add clamp
  if (this->SampleDistance < minSampleDistance) this->SampleDistance = minSampleDistance;
  if (this->SampleDistance > maxSampleDistance) this->SampleDistance = maxSampleDistance;
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderFP( vtkRenderer *ren, vtkVolume *vol )
{
  glAlphaFunc (GL_GREATER, static_cast<GLclampf>(0));
  glEnable (GL_ALPHA_TEST);

  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  int components = this->GetInput()->GetNumberOfScalarComponents();
  switch ( components )
    {
    case 1:
      if ( !vol->GetProperty()->GetShade() )
        {
        this->RenderOneIndependentNoShadeFP(ren,vol);
        }
      else
        {
  this->RenderOneIndependentShadeFP(ren,vol);
        }
      break;

    case 2:
      if ( !vol->GetProperty()->GetShade() )
        {
  this->RenderTwoDependentNoShadeFP(ren,vol);
        }
      else
        {
  this->RenderTwoDependentShadeFP(ren,vol);
        }
      break;

    case 3:
    case 4:
      if ( !vol->GetProperty()->GetShade() )
        {
  this->RenderFourDependentNoShadeFP(ren,vol);
        }
      else
        {
  this->RenderFourDependentShadeFP(ren,vol);
        }
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE2);
  glDisable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );

  vtkgl::ActiveTexture( vtkgl::TEXTURE1);
  glDisable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );

  vtkgl::ActiveTexture( vtkgl::TEXTURE0);
  glDisable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderNV( vtkRenderer *ren, vtkVolume *vol )
{
  glAlphaFunc (GL_GREATER, static_cast<GLclampf>(0));
  glEnable (GL_ALPHA_TEST);

  glEnable( GL_BLEND );
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

  int components = this->GetInput()->GetNumberOfScalarComponents();
  switch ( components )
    {
    case 1:
      if ( !vol->GetProperty()->GetShade() )
        {
        this->RenderOneIndependentNoShadeNV(ren,vol);
        }
      else
        {
        this->RenderOneIndependentShadeNV(ren,vol);
        }
      break;

    case 2:
      if ( !vol->GetProperty()->GetShade() )
        {
        this->RenderTwoDependentNoShadeNV(ren,vol);
        }
      else
        {
        this->RenderTwoDependentShadeNV(ren,vol);
        }
      break;

    case 3:
    case 4:
      if ( !vol->GetProperty()->GetShade() )
        {
        this->RenderFourDependentNoShadeNV(ren,vol);
        }
      else
        {
        this->RenderFourDependentShadeNV(ren,vol);
        }
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
  glDisable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );

  vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
  glDisable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );

  vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
  glDisable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );

  glDisable( vtkgl::TEXTURE_SHADER_NV );

  glDisable(vtkgl::REGISTER_COMBINERS_NV);
}

void vtkSlicerOpenGLVolumeTextureMapper3D::DeleteTextureIndex( GLuint *index )
{
  if (glIsTexture(*index))
    {
    GLuint tempIndex;
    tempIndex = *index;
    glDeleteTextures(1, &tempIndex);
    *index = 0;
    }
}

void vtkSlicerOpenGLVolumeTextureMapper3D::CreateTextureIndex( GLuint *index )
{
  GLuint tempIndex=0;
  glGenTextures(1, &tempIndex);
  *index = static_cast<long>(tempIndex);
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderPolygons( vtkRenderer *ren,
                                                       vtkVolume *vol,
                                                       int stages[4] )
{
  vtkRenderWindow *renWin = ren->GetRenderWindow();

  if ( renWin->CheckAbortStatus() )
    {
    return;
    }

  double bounds[27][6];
  float distance2[27];

  int   numIterations;
  int i, j, k;

  // No cropping case - render the whole thing
  if ( !this->Cropping )
    {
    // Use the input data bounds - we'll take care of the volume's
    // matrix during rendering
    this->GetInput()->GetBounds(bounds[0]);
    numIterations = 1;
    }
  // Simple cropping case - render the subvolume
  else if ( this->CroppingRegionFlags == 0x2000 )
    {
    this->GetCroppingRegionPlanes(bounds[0]);
    numIterations = 1;
    }
  // Complex cropping case - render each region in back-to-front order
  else
    {
    // Get the camera position
    double camPos[4];
    vtkCamera *cam = ren->IsActiveCameraCreated() ? ren->GetActiveCamera() : NULL;
    if (cam)
      {
      cam->GetPosition(camPos);
      }

    double volBounds[6];
    this->GetInput()->GetBounds(volBounds);

    // Pass camera through inverse volume matrix
    // so that we are in the same coordinate system
    vtkMatrix4x4 *volMatrix = vtkMatrix4x4::New();
    vol->GetMatrix( volMatrix );
    camPos[3] = 1.0;
    volMatrix->Invert();
    volMatrix->MultiplyPoint( camPos, camPos );
    volMatrix->Delete();
    if ( camPos[3] )
      {
      camPos[0] /= camPos[3];
      camPos[1] /= camPos[3];
      camPos[2] /= camPos[3];
      }

    // These are the region limits for x (first four), y (next four) and
    // z (last four). The first region limit is the lower bound for
    // that axis, the next two are the region planes along that axis, and
    // the final one in the upper bound for that axis.
    float limit[12];
    for ( i = 0; i < 3; i++ )
      {
      limit[i*4  ] = volBounds[i*2];
      limit[i*4+1] = this->CroppingRegionPlanes[i*2];
      limit[i*4+2] = this->CroppingRegionPlanes[i*2+1];
      limit[i*4+3] = volBounds[i*2+1];
      }

    // For each of the 27 possible regions, find out if it is enabled,
    // and if so, compute the bounds and the distance from the camera
    // to the center of the region.
    int numRegions = 0;
    int region;
    for ( region = 0; region < 27; region++ )
      {
      int regionFlag = 1<<region;

      if ( this->CroppingRegionFlags & regionFlag )
        {
        // what is the coordinate in the 3x3x3 grid
        int loc[3];
        loc[0] = region%3;
        loc[1] = (region/3)%3;
        loc[2] = (region/9)%3;

        // compute the bounds and center
        float center[3];
        for ( i = 0; i < 3; i++ )
          {
          bounds[numRegions][i*2  ] = limit[4*i+loc[i]];
          bounds[numRegions][i*2+1] = limit[4*i+loc[i]+1];
          center[i] =
            (bounds[numRegions][i*2  ] +
             bounds[numRegions][i*2+1])/2.0;
          }

        // compute the distance squared to the center
        distance2[numRegions] =
          (camPos[0]-center[0])*(camPos[0]-center[0]) +
          (camPos[1]-center[1])*(camPos[1]-center[1]) +
          (camPos[2]-center[2])*(camPos[2]-center[2]);

        // we've added one region
        numRegions++;
        }
      }

    // Do a quick bubble sort on distance
    for ( i = 1; i < numRegions; i++ )
      {
      for ( j = i; j > 0 && distance2[j] > distance2[j-1]; j-- )
        {
        float tmpBounds[6];
        float tmpDistance2;

        for ( k = 0; k < 6; k++ )
          {
          tmpBounds[k] = bounds[j][k];
          }
        tmpDistance2 = distance2[j];

        for ( k = 0; k < 6; k++ )
          {
          bounds[j][k] = bounds[j-1][k];
          }
        distance2[j] = distance2[j-1];

        for ( k = 0; k < 6; k++ )
          {
          bounds[j-1][k] = tmpBounds[k];
          }
        distance2[j-1] = tmpDistance2;

        }
      }

    numIterations = numRegions;
    }

  // loop over all regions we need to render
  for ( int loop = 0;
        loop < numIterations;
        loop++ )
    {
    // Compute the set of polygons for this region
    // according to the bounds
    this->ComputePolygons( ren, vol, bounds[loop] );
//    printf("polygon %d, fps %f, dist %f\n", this->NumberOfPolygons, this->Framerate, this->SampleDistance);
    // Loop over the polygons
    for ( i = 0; i < this->NumberOfPolygons; i++ )
      {
      if ( i%64 == 1 )
        {
        //glFlush();
        //glFinish();
        }

      if ( renWin->CheckAbortStatus() )
        {
        return;
        }

      float *ptr = this->PolygonBuffer + 36*i;

      glBegin( GL_TRIANGLE_FAN );

      for ( j = 0; j < 6; j++ )
        {
        if ( ptr[0] < 0.0 )
          {
          break;
          }

        for ( k = 0; k < 4; k++ )
          {
          if ( stages[k] )
            {
            vtkgl::MultiTexCoord3fv( vtkgl::TEXTURE0 + k, ptr );
            }
          }
        glVertex3fv( ptr+3 );

        ptr += 6;
        }
      glEnd();
//      double progress=i/(double)this->NumberOfPolygons;
//      int step = this->NumberOfPolygons/10 == 0 ? 1 : this->NumberOfPolygons/10;
 //     if (i % step == 0)
 //         this->InvokeEvent(vtkCommand::VolumeMapperRenderProgressEvent, &progress);
      }
    }
}

void vtkSlicerOpenGLVolumeTextureMapper3D::Setup3DTextureParameters( vtkVolumeProperty *property )
{
  if ( property->GetInterpolationType() == VTK_NEAREST_INTERPOLATION )
    {
    glTexParameterf( vtkgl::TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf( vtkgl::TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    }
  else
    {
    glTexParameterf( vtkgl::TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( vtkgl::TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    }
  glTexParameterf( vtkgl::TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameterf( vtkgl::TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::SetupOneIndependentTextures( vtkRenderer *vtkNotUsed(ren),
                    vtkVolume *vol )
{
  vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
  glDisable( GL_TEXTURE_2D );
  glEnable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( vtkgl::TEXTURE_SHADER_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, vtkgl::TEXTURE_3D);
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
  glDisable( GL_TEXTURE_2D );
  glEnable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( vtkgl::TEXTURE_SHADER_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, vtkgl::TEXTURE_3D);
    }

  // Update the volume containing the 2 byte scalar / gradient magnitude
  if ( this->UpdateVolumes( vol ) || !this->Volume1Index || !this->Volume2Index || vol->GetNumberOfConsumers() > 1)
    {
    int dim[3];
    this->GetVolumeDimensions(dim);
    this->DeleteTextureIndex(&this->Volume3Index);

    vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
    glBindTexture(vtkgl::TEXTURE_3D,0);
    this->DeleteTextureIndex(&this->Volume1Index);
    this->CreateTextureIndex(&this->Volume1Index);
    glBindTexture(vtkgl::TEXTURE_3D, this->Volume1Index);
    vtkgl::TexImage3D( vtkgl::TEXTURE_3D, 0, GL_LUMINANCE8_ALPHA8, dim[0], dim[1], dim[2], 0,
                           GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, this->Volume1 );


    vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
    glBindTexture(vtkgl::TEXTURE_3D,0);
    this->DeleteTextureIndex(&this->Volume2Index);
    this->CreateTextureIndex(&this->Volume2Index);
    glBindTexture(vtkgl::TEXTURE_3D, this->Volume2Index);
    vtkgl::TexImage3D( vtkgl::TEXTURE_3D, 0, GL_RGBA8, dim[0], dim[1], dim[2], 0,
                           GL_RGB, GL_UNSIGNED_BYTE, this->Volume2 );
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
  glBindTexture(vtkgl::TEXTURE_3D, this->Volume1Index);
  this->Setup3DTextureParameters( vol->GetProperty() );

  vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
  glBindTexture(vtkgl::TEXTURE_3D, this->Volume2Index);
  this->Setup3DTextureParameters( vol->GetProperty() );

  vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
  glEnable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glTexEnvf ( vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV,
                vtkgl::DEPENDENT_AR_TEXTURE_2D_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::PREVIOUS_TEXTURE_INPUT_NV, vtkgl::TEXTURE0);
    }

  // Update the dependent 2D color table mapping scalar value and
  // gradient magnitude to RGBA
  if ( this->UpdateColorLookup( vol ) || !this->ColorLookupIndex || vol->GetNumberOfConsumers() > 1)
    {
    this->DeleteTextureIndex( &this->ColorLookupIndex );
    this->DeleteTextureIndex( &this->AlphaLookupIndex );

    this->CreateTextureIndex( &this->ColorLookupIndex );
    glBindTexture(GL_TEXTURE_2D, this->ColorLookupIndex);

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0,
                  GL_RGBA, GL_UNSIGNED_BYTE, this->ColorLookup );
    }

  glBindTexture(GL_TEXTURE_2D, this->ColorLookupIndex);
}

void vtkSlicerOpenGLVolumeTextureMapper3D::SetupRegisterCombinersNoShadeNV( vtkRenderer *vtkNotUsed(ren),
                  vtkVolume *vtkNotUsed(vol),
                  int components )
{
  if ( components < 3 )
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE2);
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, GL_NONE);

    if ( components == 1 )
      {
      vtkgl::ActiveTexture(vtkgl::TEXTURE3);
      glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, GL_NONE);
      }
    }


  glEnable(vtkgl::REGISTER_COMBINERS_NV);
  vtkgl::CombinerParameteriNV(vtkgl::NUM_GENERAL_COMBINERS_NV, 1);
  vtkgl::CombinerParameteriNV(vtkgl::COLOR_SUM_CLAMP_NV, GL_TRUE);

  vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_A_NV, GL_ZERO,         vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
  vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_B_NV, GL_ZERO,         vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
  vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_C_NV, GL_ZERO,         vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
  if ( components < 3 )
    {
    vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_D_NV, vtkgl::TEXTURE1, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
    }
  else
    {
    vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_D_NV, vtkgl::TEXTURE0, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
    }

  if ( components == 1 )
    {
    vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_G_NV, vtkgl::TEXTURE1, vtkgl::UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }
  else
    {
    vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_G_NV, vtkgl::TEXTURE3, vtkgl::UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }
}

void vtkSlicerOpenGLVolumeTextureMapper3D::SetupRegisterCombinersShadeNV( vtkRenderer *ren,
                      vtkVolume *vol,
                      int components )
{
  if ( components == 1 )
    {
    vtkgl::ActiveTexture(vtkgl::TEXTURE3);
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, GL_NONE);
    }

  GLfloat white[4] = {1,1,1,1};

  GLfloat lightDirection[2][4];
  GLfloat lightDiffuseColor[2][4];
  GLfloat lightSpecularColor[2][4];
  GLfloat halfwayVector[2][4];
  GLfloat ambientColor[4];

  // Gather information about the light sources. Although we gather info for multiple light sources,
  // in this case we will only use the first one, and will duplicate it (in opposite direction) to
  // approximate two-sided lighting.
  this->GetLightInformation( ren, vol, lightDirection, lightDiffuseColor,
                             lightSpecularColor, halfwayVector, ambientColor );

  float specularPower = vol->GetProperty()->GetSpecularPower();

  glEnable(vtkgl::REGISTER_COMBINERS_NV);
  glEnable( vtkgl::PER_STAGE_CONSTANTS_NV );
  vtkgl::CombinerParameteriNV(vtkgl::NUM_GENERAL_COMBINERS_NV, 8);
  vtkgl::CombinerParameteriNV(vtkgl::COLOR_SUM_CLAMP_NV, GL_TRUE);

  // Stage 0
  //
  //  N dot L is computed into vtkgl::SPARE0_NV
  // -N dot L is computed into vtkgl::SPARE1_NV
  //
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER0_NV, vtkgl::CONSTANT_COLOR0_NV, lightDirection[0] );

  vtkgl::CombinerInputNV( vtkgl::COMBINER0_NV, GL_RGB, vtkgl::VARIABLE_A_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::EXPAND_NORMAL_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER0_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                          vtkgl::TEXTURE2,       vtkgl::EXPAND_NORMAL_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER0_NV, GL_RGB, vtkgl::VARIABLE_C_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::EXPAND_NORMAL_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER0_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                          vtkgl::TEXTURE2,       vtkgl::EXPAND_NEGATE_NV, GL_RGB );

  vtkgl::CombinerOutputNV( vtkgl::COMBINER0_NV, GL_RGB, vtkgl::SPARE0_NV, vtkgl::SPARE1_NV, vtkgl::DISCARD_NV,
                            GL_NONE, GL_NONE, GL_TRUE, GL_TRUE, GL_FALSE );

  // Stage 1
  //
  // lightColor * max( 0, (N dot L)) + lightColor * max( 0, (-N dot L)) is computed into vtkgl::SPARE0_NV
  //
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER1_NV, vtkgl::CONSTANT_COLOR0_NV, lightDiffuseColor[0] );

  vtkgl::CombinerInputNV( vtkgl::COMBINER1_NV, GL_RGB, vtkgl::VARIABLE_A_NV,
                          vtkgl::SPARE0_NV,          vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER1_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER1_NV, GL_RGB, vtkgl::VARIABLE_C_NV,
                          vtkgl::SPARE1_NV,          vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER1_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );

  vtkgl::CombinerOutputNV( vtkgl::COMBINER1_NV, GL_RGB, vtkgl::DISCARD_NV, vtkgl::DISCARD_NV,
                           vtkgl::SPARE0_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  // Stage 2
  //
  // result from Stage 1 is added to the ambient color and stored in vtkgl::PRIMARY_COLOR_NV
  //
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER2_NV, vtkgl::CONSTANT_COLOR0_NV, white );
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER2_NV, vtkgl::CONSTANT_COLOR1_NV, ambientColor );

  vtkgl::CombinerInputNV( vtkgl::COMBINER2_NV, GL_RGB, vtkgl::VARIABLE_A_NV,
                          vtkgl::SPARE0_NV,          vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER2_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER2_NV, GL_RGB, vtkgl::VARIABLE_C_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER2_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                          vtkgl::CONSTANT_COLOR1_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );

  vtkgl::CombinerOutputNV( vtkgl::COMBINER2_NV, GL_RGB, vtkgl::DISCARD_NV, vtkgl::DISCARD_NV,
                           vtkgl::PRIMARY_COLOR_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  // Stage 3
  //
  //  N dot H is computed into vtkgl::SPARE0_NV
  // -N dot H is computed into vtkgl::SPARE1_NV
  //
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER3_NV, vtkgl::CONSTANT_COLOR0_NV, halfwayVector[0] );

  vtkgl::CombinerInputNV( vtkgl::COMBINER3_NV, GL_RGB, vtkgl::VARIABLE_A_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::EXPAND_NORMAL_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER3_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                          vtkgl::TEXTURE2, vtkgl::EXPAND_NORMAL_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER3_NV, GL_RGB, vtkgl::VARIABLE_C_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::EXPAND_NORMAL_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER3_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                          vtkgl::TEXTURE2, vtkgl::EXPAND_NEGATE_NV, GL_RGB );

  vtkgl::CombinerOutputNV( vtkgl::COMBINER3_NV, GL_RGB, vtkgl::SPARE0_NV, vtkgl::SPARE1_NV,
                           vtkgl::DISCARD_NV, GL_NONE, GL_NONE, GL_TRUE, GL_TRUE, GL_FALSE );

  // Stage 4
  //
  // if the specular power is greater than 1, then
  //
  //  N dot H squared is computed into vtkgl::SPARE0_NV
  // -N dot H squared is computed into vtkgl::SPARE1_NV
  //
  // otherwise these registers are simply multiplied by white
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER4_NV, vtkgl::CONSTANT_COLOR0_NV, white );

  vtkgl::CombinerInputNV( vtkgl::COMBINER4_NV, GL_RGB, vtkgl::VARIABLE_A_NV,
                          vtkgl::SPARE0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER4_NV, GL_RGB, vtkgl::VARIABLE_C_NV,
                          vtkgl::SPARE1_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  if ( specularPower > 1.0 )
    {
    vtkgl::CombinerInputNV( vtkgl::COMBINER4_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                            vtkgl::SPARE0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    vtkgl::CombinerInputNV( vtkgl::COMBINER4_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                            vtkgl::SPARE1_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  else
    {
    vtkgl::CombinerInputNV( vtkgl::COMBINER4_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                            vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    vtkgl::CombinerInputNV( vtkgl::COMBINER4_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                            vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    }

  vtkgl::CombinerOutputNV( vtkgl::COMBINER4_NV, GL_RGB, vtkgl::SPARE0_NV, vtkgl::SPARE1_NV, vtkgl::DISCARD_NV,
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  // Stage 5
  //
  // if the specular power is greater than 3, then
  //
  //  N dot H to the fourth is computed into vtkgl::SPARE0_NV
  // -N dot H to the fourth is computed into vtkgl::SPARE1_NV
  //
  // otherwise these registers are simply multiplied by white
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER5_NV, vtkgl::CONSTANT_COLOR0_NV, white );

  vtkgl::CombinerInputNV( vtkgl::COMBINER5_NV, GL_RGB, vtkgl::VARIABLE_A_NV,
                          vtkgl::SPARE0_NV,  vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER5_NV, GL_RGB, vtkgl::VARIABLE_C_NV,
                          vtkgl::SPARE1_NV,  vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  if ( specularPower > 3.0 )
    {
    vtkgl::CombinerInputNV( vtkgl::COMBINER5_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                            vtkgl::SPARE0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    vtkgl::CombinerInputNV( vtkgl::COMBINER5_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                            vtkgl::SPARE1_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  else
    {
    vtkgl::CombinerInputNV( vtkgl::COMBINER5_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                            vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    vtkgl::CombinerInputNV( vtkgl::COMBINER5_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                            vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    }

  vtkgl::CombinerOutputNV( vtkgl::COMBINER5_NV, GL_RGB, vtkgl::SPARE0_NV, vtkgl::SPARE1_NV, vtkgl::DISCARD_NV,
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  // Stage 6
  //
  // if the specular power is greater than 6, then
  //
  //  N dot H to the eighth is computed into vtkgl::SPARE0_NV
  // -N dot H to the eighth is computed into vtkgl::SPARE1_NV
  //
  // otherwise these registers are simply multiplied by white
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER6_NV, vtkgl::CONSTANT_COLOR0_NV, white );

  vtkgl::CombinerInputNV( vtkgl::COMBINER6_NV, GL_RGB, vtkgl::VARIABLE_A_NV,
                          vtkgl::SPARE0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER6_NV, GL_RGB, vtkgl::VARIABLE_C_NV,
                          vtkgl::SPARE1_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );

  if ( specularPower > 6.0 )
    {
    vtkgl::CombinerInputNV( vtkgl::COMBINER6_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                            vtkgl::SPARE0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    vtkgl::CombinerInputNV( vtkgl::COMBINER6_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                            vtkgl::SPARE1_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    }
  else
    {
    vtkgl::CombinerInputNV( vtkgl::COMBINER6_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                            vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    vtkgl::CombinerInputNV( vtkgl::COMBINER6_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                            vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
    }

  vtkgl::CombinerOutputNV( vtkgl::COMBINER6_NV, GL_RGB, vtkgl::SPARE0_NV, vtkgl::SPARE1_NV,
                           vtkgl::DISCARD_NV, GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );


  // Stage 7
  //
  // Add the two specular contributions and multiply each by the specular color.
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER7_NV, vtkgl::CONSTANT_COLOR0_NV, lightSpecularColor[0] );
  vtkgl::CombinerStageParameterfvNV( vtkgl::COMBINER7_NV, vtkgl::CONSTANT_COLOR1_NV, lightSpecularColor[1] );

  vtkgl::CombinerInputNV( vtkgl::COMBINER7_NV, GL_RGB, vtkgl::VARIABLE_A_NV,
                          vtkgl::SPARE0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER7_NV, GL_RGB, vtkgl::VARIABLE_B_NV,
                          vtkgl::CONSTANT_COLOR0_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER7_NV, GL_RGB, vtkgl::VARIABLE_C_NV,
                          vtkgl::SPARE1_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );
  vtkgl::CombinerInputNV( vtkgl::COMBINER7_NV, GL_RGB, vtkgl::VARIABLE_D_NV,
                          vtkgl::CONSTANT_COLOR1_NV, vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB );

  vtkgl::CombinerOutputNV( vtkgl::COMBINER7_NV, GL_RGB, vtkgl::DISCARD_NV,
                           vtkgl::DISCARD_NV, vtkgl::SPARE0_NV,
                            GL_NONE, GL_NONE, GL_FALSE, GL_FALSE, GL_FALSE );

  vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_A_NV, vtkgl::PRIMARY_COLOR_NV,
                              vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
  if ( components < 3 )
    {
    vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_B_NV, vtkgl::TEXTURE1,
                                vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
    }
  else
    {
    vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_B_NV, vtkgl::TEXTURE0,
                                vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
    }
  vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_C_NV, GL_ZERO,
                              vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );
  vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_D_NV, vtkgl::SPARE0_NV,
                              vtkgl::UNSIGNED_IDENTITY_NV, GL_RGB  );

  if ( components == 1 )
    {
    vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_G_NV, vtkgl::TEXTURE1,
                                vtkgl::UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }
  else
    {
    vtkgl::FinalCombinerInputNV(vtkgl::VARIABLE_G_NV, vtkgl::TEXTURE3,
                                vtkgl::UNSIGNED_IDENTITY_NV, GL_ALPHA);
    }

}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderOneIndependentNoShadeNV(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  this->SetupOneIndependentTextures( ren, vol );

  this->SetupRegisterCombinersNoShadeNV( ren, vol, 1 );

  int stages[4] = {1,0,0,0};
  this->RenderPolygons( ren, vol, stages );
}


void vtkSlicerOpenGLVolumeTextureMapper3D::RenderOneIndependentShadeNV(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  this->SetupOneIndependentTextures( ren, vol );

  this->SetupRegisterCombinersShadeNV( ren, vol, 1 );

  int stages[4] = {1,0,1,0};
  this->RenderPolygons( ren, vol, stages );
}


void vtkSlicerOpenGLVolumeTextureMapper3D::SetupTwoDependentTextures(
  vtkRenderer *vtkNotUsed(ren),
  vtkVolume *vol )
{
  vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
  glDisable( GL_TEXTURE_2D );
  glEnable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( vtkgl::TEXTURE_SHADER_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, vtkgl::TEXTURE_3D);
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
  glDisable( GL_TEXTURE_2D );
  glEnable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( vtkgl::TEXTURE_SHADER_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, vtkgl::TEXTURE_3D);
    }

  // Update the volume containing the 3 byte scalars / gradient magnitude
  if ( this->UpdateVolumes( vol ) || !this->Volume1Index || !this->Volume2Index || vol->GetNumberOfConsumers() > 1)
    {
    int dim[3];
    this->GetVolumeDimensions(dim);
    this->DeleteTextureIndex(&this->Volume3Index);

    vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
    glBindTexture(vtkgl::TEXTURE_3D,0);
    this->DeleteTextureIndex(&this->Volume1Index);
    this->CreateTextureIndex(&this->Volume1Index);
    glBindTexture(vtkgl::TEXTURE_3D, this->Volume1Index);
    vtkgl::TexImage3D(vtkgl::TEXTURE_3D, 0, GL_RGB8, dim[0], dim[1], dim[2], 0,
                      GL_RGB, GL_UNSIGNED_BYTE, this->Volume1 );

    vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
    glBindTexture(vtkgl::TEXTURE_3D,0);
    this->DeleteTextureIndex(&this->Volume2Index);
    this->CreateTextureIndex(&this->Volume2Index);
    glBindTexture(vtkgl::TEXTURE_3D, this->Volume2Index);
    vtkgl::TexImage3D(vtkgl::TEXTURE_3D,0, GL_RGBA8, dim[0], dim[1], dim[2], 0,
                           GL_RGB, GL_UNSIGNED_BYTE, this->Volume2 );
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
  glBindTexture(vtkgl::TEXTURE_3D, this->Volume1Index);
  this->Setup3DTextureParameters( vol->GetProperty() );

  vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
  glBindTexture(vtkgl::TEXTURE_3D, this->Volume2Index);
  this->Setup3DTextureParameters( vol->GetProperty() );

  vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
  glEnable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glTexEnvf ( vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV,
                vtkgl::DEPENDENT_AR_TEXTURE_2D_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::PREVIOUS_TEXTURE_INPUT_NV, vtkgl::TEXTURE0);
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
  glEnable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glTexEnvf ( vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV,
                vtkgl::DEPENDENT_GB_TEXTURE_2D_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::PREVIOUS_TEXTURE_INPUT_NV, vtkgl::TEXTURE0);
    }

  // Update the dependent 2D color table mapping scalar value and
  // gradient magnitude to RGBA
  if ( this->UpdateColorLookup( vol ) ||
       !this->ColorLookupIndex || !this->AlphaLookupIndex || vol->GetNumberOfConsumers() > 1)
    {
    vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
    glBindTexture(GL_TEXTURE_2D,0);
    this->DeleteTextureIndex(&this->ColorLookupIndex);
    this->CreateTextureIndex(&this->ColorLookupIndex);
    glBindTexture(GL_TEXTURE_2D, this->ColorLookupIndex);

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB8, 256, 256, 0,
                  GL_RGB, GL_UNSIGNED_BYTE, this->ColorLookup );

    vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
    glBindTexture(GL_TEXTURE_2D,0);
    this->DeleteTextureIndex(&this->AlphaLookupIndex);
    this->CreateTextureIndex(&this->AlphaLookupIndex);
    glBindTexture(GL_TEXTURE_2D, this->AlphaLookupIndex);

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA8, 256, 256, 0,
                  GL_ALPHA, GL_UNSIGNED_BYTE, this->AlphaLookup );
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
  glBindTexture(GL_TEXTURE_2D, this->ColorLookupIndex);

  vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
  glBindTexture(GL_TEXTURE_2D, this->AlphaLookupIndex);
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderTwoDependentNoShadeNV(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  this->SetupTwoDependentTextures(ren, vol);

  this->SetupRegisterCombinersNoShadeNV( ren, vol, 2 );

  int stages[4] = {1,0,0,0};
  this->RenderPolygons( ren, vol, stages );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderTwoDependentShadeNV(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  this->SetupTwoDependentTextures( ren, vol );

  this->SetupRegisterCombinersShadeNV( ren, vol, 2 );

  int stages[4] = {1,0,1,0};
  this->RenderPolygons( ren, vol, stages );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::SetupFourDependentTextures(
  vtkRenderer *vtkNotUsed(ren),
  vtkVolume *vol )
{
  vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
  glDisable( GL_TEXTURE_2D );
  glEnable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( vtkgl::TEXTURE_SHADER_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, vtkgl::TEXTURE_3D);
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
  glDisable( GL_TEXTURE_2D );
  glEnable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( vtkgl::TEXTURE_SHADER_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, vtkgl::TEXTURE_3D);
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
  glDisable( GL_TEXTURE_2D );
  glEnable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glEnable( vtkgl::TEXTURE_SHADER_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV, vtkgl::TEXTURE_3D);
    }

  // Update the volume containing the 3 byte scalars / gradient magnitude
  if ( this->UpdateVolumes( vol ) || !this->Volume1Index ||
       !this->Volume2Index || !this->Volume3Index || vol->GetNumberOfConsumers() > 1)
    {
    int dim[3];
    this->GetVolumeDimensions(dim);

    vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
    glBindTexture(vtkgl::TEXTURE_3D,0);
    this->DeleteTextureIndex(&this->Volume1Index);
    this->CreateTextureIndex(&this->Volume1Index);
    glBindTexture(vtkgl::TEXTURE_3D, this->Volume1Index);
    vtkgl::TexImage3D(vtkgl::TEXTURE_3D, 0, GL_RGB8, dim[0], dim[1], dim[2], 0,
                      GL_RGB, GL_UNSIGNED_BYTE, this->Volume1 );

    vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
    glBindTexture(vtkgl::TEXTURE_3D,0);
    this->DeleteTextureIndex(&this->Volume2Index);
    this->CreateTextureIndex(&this->Volume2Index);
    glBindTexture(vtkgl::TEXTURE_3D, this->Volume2Index);
    vtkgl::TexImage3D(vtkgl::TEXTURE_3D,0,GL_LUMINANCE8_ALPHA8,dim[0],dim[1],
                      dim[2], 0,GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE,
                      this->Volume2 );

    vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
    glBindTexture(vtkgl::TEXTURE_3D,0);
    this->DeleteTextureIndex(&this->Volume3Index);
    this->CreateTextureIndex(&this->Volume3Index);
    glBindTexture(vtkgl::TEXTURE_3D, this->Volume3Index);
    vtkgl::TexImage3D( vtkgl::TEXTURE_3D,0, GL_RGB8, dim[0], dim[1], dim[2], 0,
                       GL_RGB, GL_UNSIGNED_BYTE, this->Volume3 );
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE0 );
  glBindTexture(vtkgl::TEXTURE_3D, this->Volume1Index);
  this->Setup3DTextureParameters( vol->GetProperty() );

  vtkgl::ActiveTexture( vtkgl::TEXTURE1 );
  glBindTexture(vtkgl::TEXTURE_3D, this->Volume2Index);
  this->Setup3DTextureParameters( vol->GetProperty() );

  vtkgl::ActiveTexture( vtkgl::TEXTURE2 );
  glBindTexture(vtkgl::TEXTURE_3D_EXT, this->Volume3Index);
  this->Setup3DTextureParameters( vol->GetProperty() );

  vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
  glEnable( GL_TEXTURE_2D );
  glDisable( vtkgl::TEXTURE_3D );
  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD )
    {
    glTexEnvf ( vtkgl::TEXTURE_SHADER_NV, vtkgl::SHADER_OPERATION_NV,
                vtkgl::DEPENDENT_AR_TEXTURE_2D_NV );
    glTexEnvi(vtkgl::TEXTURE_SHADER_NV, vtkgl::PREVIOUS_TEXTURE_INPUT_NV,
              vtkgl::TEXTURE1);
    }

  // Update the dependent 2D table mapping scalar value and
  // gradient magnitude to opacity
  if ( this->UpdateColorLookup( vol ) || !this->AlphaLookupIndex || vol->GetNumberOfConsumers() > 1)
    {
    this->DeleteTextureIndex(&this->ColorLookupIndex);

    vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
    glBindTexture(GL_TEXTURE_2D,0);
    this->DeleteTextureIndex(&this->AlphaLookupIndex);
    this->CreateTextureIndex(&this->AlphaLookupIndex);
    glBindTexture(GL_TEXTURE_2D, this->AlphaLookupIndex);

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA8, 256, 256, 0,
                  GL_ALPHA, GL_UNSIGNED_BYTE, this->AlphaLookup );
    }

  vtkgl::ActiveTexture( vtkgl::TEXTURE3 );
  glBindTexture(GL_TEXTURE_2D, this->AlphaLookupIndex);
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderFourDependentNoShadeNV(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  this->SetupFourDependentTextures(ren, vol);

  this->SetupRegisterCombinersNoShadeNV( ren, vol, 4 );

  int stages[4] = {1,1,0,0};
  this->RenderPolygons( ren, vol, stages );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderFourDependentShadeNV(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  this->SetupFourDependentTextures( ren, vol );

  this->SetupRegisterCombinersShadeNV( ren, vol, 4 );

  int stages[4] = {1,1,1,0};
  this->RenderPolygons( ren, vol, stages );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderOneIndependentNoShadeFP(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  glEnable( vtkgl::FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  vtkgl::GenProgramsARB( 1, &fragmentProgram );


  vtkgl::BindProgramARB( vtkgl::FRAGMENT_PROGRAM_ARB, fragmentProgram );

  vtkgl::ProgramStringARB( vtkgl::FRAGMENT_PROGRAM_ARB,
          vtkgl::PROGRAM_FORMAT_ASCII_ARB,
          strlen(vtkVolumeTextureMapper3D_OneComponentNoShadeFP),
          vtkVolumeTextureMapper3D_OneComponentNoShadeFP );

  this->SetupOneIndependentTextures( ren, vol );

  int stages[4] = {1,0,0,0};
  this->RenderPolygons( ren, vol, stages );

  glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );

  vtkgl::DeleteProgramsARB( 1, &fragmentProgram );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderOneIndependentShadeFP(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  glEnable( vtkgl::FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  vtkgl::GenProgramsARB( 1, &fragmentProgram );


  vtkgl::BindProgramARB( vtkgl::FRAGMENT_PROGRAM_ARB, fragmentProgram );

  vtkgl::ProgramStringARB( vtkgl::FRAGMENT_PROGRAM_ARB,
          vtkgl::PROGRAM_FORMAT_ASCII_ARB,
          strlen(vtkVolumeTextureMapper3D_OneComponentShadeFP),
          vtkVolumeTextureMapper3D_OneComponentShadeFP );

  this->SetupOneIndependentTextures( ren, vol );
  this->SetupProgramLocalsForShadingFP( ren, vol );

  int stages[4] = {1,1,1,0};
  this->RenderPolygons( ren, vol, stages );

  glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );

  vtkgl::DeleteProgramsARB( 1, &fragmentProgram );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderTwoDependentNoShadeFP(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  glEnable( vtkgl::FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  vtkgl::GenProgramsARB( 1, &fragmentProgram );

  vtkgl::BindProgramARB( vtkgl::FRAGMENT_PROGRAM_ARB, fragmentProgram );

  vtkgl::ProgramStringARB( vtkgl::FRAGMENT_PROGRAM_ARB,
          vtkgl::PROGRAM_FORMAT_ASCII_ARB,
          strlen(vtkVolumeTextureMapper3D_TwoDependentNoShadeFP),
          vtkVolumeTextureMapper3D_TwoDependentNoShadeFP );

  this->SetupTwoDependentTextures(ren, vol);

  int stages[4] = {1,0,0,0};
  this->RenderPolygons( ren, vol, stages );

  glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );

  vtkgl::DeleteProgramsARB( 1, &fragmentProgram );
}


void vtkSlicerOpenGLVolumeTextureMapper3D::RenderTwoDependentShadeFP(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  glEnable( vtkgl::FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  vtkgl::GenProgramsARB( 1, &fragmentProgram );

  vtkgl::BindProgramARB( vtkgl::FRAGMENT_PROGRAM_ARB, fragmentProgram );

  vtkgl::ProgramStringARB( vtkgl::FRAGMENT_PROGRAM_ARB,
          vtkgl::PROGRAM_FORMAT_ASCII_ARB,
          strlen(vtkVolumeTextureMapper3D_TwoDependentShadeFP),
          vtkVolumeTextureMapper3D_TwoDependentShadeFP );

  this->SetupTwoDependentTextures(ren, vol);
  this->SetupProgramLocalsForShadingFP( ren, vol );

  int stages[4] = {1,0,1,0};
  this->RenderPolygons( ren, vol, stages );

  glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );

  vtkgl::DeleteProgramsARB( 1, &fragmentProgram );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderFourDependentNoShadeFP(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  glEnable( vtkgl::FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  vtkgl::GenProgramsARB( 1, &fragmentProgram );

  vtkgl::BindProgramARB( vtkgl::FRAGMENT_PROGRAM_ARB, fragmentProgram );

  vtkgl::ProgramStringARB( vtkgl::FRAGMENT_PROGRAM_ARB,
          vtkgl::PROGRAM_FORMAT_ASCII_ARB,
          strlen(vtkVolumeTextureMapper3D_FourDependentNoShadeFP),
          vtkVolumeTextureMapper3D_FourDependentNoShadeFP );

  this->SetupFourDependentTextures(ren, vol);

  int stages[4] = {1,1,0,0};
  this->RenderPolygons( ren, vol, stages );

  glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );

  vtkgl::DeleteProgramsARB( 1, &fragmentProgram );
}

void vtkSlicerOpenGLVolumeTextureMapper3D::RenderFourDependentShadeFP(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  glEnable( vtkgl::FRAGMENT_PROGRAM_ARB );

  GLuint fragmentProgram;
  vtkgl::GenProgramsARB( 1, &fragmentProgram );

  vtkgl::BindProgramARB( vtkgl::FRAGMENT_PROGRAM_ARB, fragmentProgram );

  vtkgl::ProgramStringARB( vtkgl::FRAGMENT_PROGRAM_ARB,
          vtkgl::PROGRAM_FORMAT_ASCII_ARB,
          strlen(vtkVolumeTextureMapper3D_FourDependentShadeFP),
          vtkVolumeTextureMapper3D_FourDependentShadeFP );

  this->SetupFourDependentTextures(ren, vol);
  this->SetupProgramLocalsForShadingFP( ren, vol );

  int stages[4] = {1,1,1,0};
  this->RenderPolygons( ren, vol, stages );

  glDisable( vtkgl::FRAGMENT_PROGRAM_ARB );

  vtkgl::DeleteProgramsARB( 1, &fragmentProgram );
}


void vtkSlicerOpenGLVolumeTextureMapper3D::GetLightInformation(
  vtkRenderer *ren,
  vtkVolume *vol,
  GLfloat lightDirection[2][4],
  GLfloat lightDiffuseColor[2][4],
  GLfloat lightSpecularColor[2][4],
  GLfloat halfwayVector[2][4],
  GLfloat ambientColor[4] )
{
  float ambient = vol->GetProperty()->GetAmbient();
  float diffuse  = vol->GetProperty()->GetDiffuse();
  float specular = vol->GetProperty()->GetSpecular();

  vtkTransform *volumeTransform = vtkTransform::New();

  volumeTransform->SetMatrix( vol->GetMatrix() );
  volumeTransform->Inverse();

  vtkLightCollection *lights = ren->GetLights();
  lights->InitTraversal();

  vtkLight *light[2];
  light[0] = lights->GetNextItem();
  light[1] = lights->GetNextItem();

  int lightIndex = 0;

  double cameraPosition[3];
  double cameraFocalPoint[3];

  vtkCamera *cam = ren->IsActiveCameraCreated() ? ren->GetActiveCamera() : NULL;
  if (cam)
    {
    cam->GetPosition( cameraPosition );
    cam->GetFocalPoint( cameraFocalPoint );
    }

  double viewDirection[3];

  volumeTransform->TransformPoint( cameraPosition, cameraPosition );
  volumeTransform->TransformPoint( cameraFocalPoint, cameraFocalPoint );

  viewDirection[0] = cameraFocalPoint[0] - cameraPosition[0];
  viewDirection[1] = cameraFocalPoint[1] - cameraPosition[1];
  viewDirection[2] = cameraFocalPoint[2] - cameraPosition[2];

  vtkMath::Normalize( viewDirection );


  ambientColor[0] = 0.0;
  ambientColor[1] = 0.0;
  ambientColor[2] = 0.0;
  ambientColor[3] = 0.0;

  for ( lightIndex = 0; lightIndex < 2; lightIndex++ )
    {
    float dir[3] = {0,0,0};
    float half[3] = {0,0,0};

    if ( light[lightIndex] == NULL ||
         light[lightIndex]->GetSwitch() == 0 )
      {
      lightDiffuseColor[lightIndex][0] = 0.0;
      lightDiffuseColor[lightIndex][1] = 0.0;
      lightDiffuseColor[lightIndex][2] = 0.0;
      lightDiffuseColor[lightIndex][3] = 0.0;

      lightSpecularColor[lightIndex][0] = 0.0;
      lightSpecularColor[lightIndex][1] = 0.0;
      lightSpecularColor[lightIndex][2] = 0.0;
      lightSpecularColor[lightIndex][3] = 0.0;
      }
    else
      {
      float lightIntensity = light[lightIndex]->GetIntensity();
      double lightColor[3];

      light[lightIndex]->GetDiffuseColor( lightColor );

      double lightPosition[3];
      double lightFocalPoint[3];
      light[lightIndex]->GetTransformedPosition( lightPosition );
      light[lightIndex]->GetTransformedFocalPoint( lightFocalPoint );

      volumeTransform->TransformPoint( lightPosition, lightPosition );
      volumeTransform->TransformPoint( lightFocalPoint, lightFocalPoint );

      dir[0] = lightPosition[0] - lightFocalPoint[0];
      dir[1] = lightPosition[1] - lightFocalPoint[1];
      dir[2] = lightPosition[2] - lightFocalPoint[2];

      vtkMath::Normalize( dir );

      lightDiffuseColor[lightIndex][0] = lightColor[0]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][1] = lightColor[1]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][2] = lightColor[2]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][3] = 1.0;

      lightSpecularColor[lightIndex][0]= lightColor[0]*specular*lightIntensity;
      lightSpecularColor[lightIndex][1]= lightColor[1]*specular*lightIntensity;
      lightSpecularColor[lightIndex][2]= lightColor[2]*specular*lightIntensity;
      lightSpecularColor[lightIndex][3] = 0.0;

      half[0] = dir[0] - viewDirection[0];
      half[1] = dir[1] - viewDirection[1];
      half[2] = dir[2] - viewDirection[2];

      vtkMath::Normalize( half );

      ambientColor[0] += ambient*lightColor[0];
      ambientColor[1] += ambient*lightColor[1];
      ambientColor[2] += ambient*lightColor[2];
      }

    lightDirection[lightIndex][0] = (dir[0]+1.0)/2.0;
    lightDirection[lightIndex][1] = (dir[1]+1.0)/2.0;
    lightDirection[lightIndex][2] = (dir[2]+1.0)/2.0;
    lightDirection[lightIndex][3] = 0.0;

    halfwayVector[lightIndex][0] = (half[0]+1.0)/2.0;
    halfwayVector[lightIndex][1] = (half[1]+1.0)/2.0;
    halfwayVector[lightIndex][2] = (half[2]+1.0)/2.0;
    halfwayVector[lightIndex][3] = 0.0;
    }

  volumeTransform->Delete();

}

void vtkSlicerOpenGLVolumeTextureMapper3D::SetupProgramLocalsForShadingFP(
  vtkRenderer *ren,
  vtkVolume *vol )
{
  GLfloat lightDirection[2][4];
  GLfloat lightDiffuseColor[2][4];
  GLfloat lightSpecularColor[2][4];
  GLfloat halfwayVector[2][4];
  GLfloat ambientColor[4];

  float ambient       = vol->GetProperty()->GetAmbient();
  float diffuse       = vol->GetProperty()->GetDiffuse();
  float specular      = vol->GetProperty()->GetSpecular();
  float specularPower = vol->GetProperty()->GetSpecularPower();

  vtkTransform *volumeTransform = vtkTransform::New();

  volumeTransform->SetMatrix( vol->GetMatrix() );
  volumeTransform->Inverse();

  vtkLightCollection *lights = ren->GetLights();
  lights->InitTraversal();

  vtkLight *light[2];
  light[0] = lights->GetNextItem();
  light[1] = lights->GetNextItem();

  int lightIndex = 0;

  double cameraPosition[3];
  double cameraFocalPoint[3];

  vtkCamera *cam = ren->IsActiveCameraCreated() ? ren->GetActiveCamera() : NULL;
  if (cam)
    {
    cam->GetPosition( cameraPosition );
    cam->GetFocalPoint( cameraFocalPoint );
    }

  volumeTransform->TransformPoint( cameraPosition, cameraPosition );
  volumeTransform->TransformPoint( cameraFocalPoint, cameraFocalPoint );

  double viewDirection[4];

  viewDirection[0] = cameraFocalPoint[0] - cameraPosition[0];
  viewDirection[1] = cameraFocalPoint[1] - cameraPosition[1];
  viewDirection[2] = cameraFocalPoint[2] - cameraPosition[2];
  viewDirection[3] = 0.0;

  vtkMath::Normalize( viewDirection );

  ambientColor[0] = 0.0;
  ambientColor[1] = 0.0;
  ambientColor[2] = 0.0;
  ambientColor[3] = 0.0;

  for ( lightIndex = 0; lightIndex < 2; lightIndex++ )
    {
    float dir[3] = {0,0,0};
    float half[3] = {0,0,0};

    if ( light[lightIndex] == NULL ||
         light[lightIndex]->GetSwitch() == 0 )
      {
      lightDiffuseColor[lightIndex][0] = 0.0;
      lightDiffuseColor[lightIndex][1] = 0.0;
      lightDiffuseColor[lightIndex][2] = 0.0;
      lightDiffuseColor[lightIndex][3] = 0.0;

      lightSpecularColor[lightIndex][0] = 0.0;
      lightSpecularColor[lightIndex][1] = 0.0;
      lightSpecularColor[lightIndex][2] = 0.0;
      lightSpecularColor[lightIndex][3] = 0.0;
      }
    else
      {
      float lightIntensity = light[lightIndex]->GetIntensity();
      double lightColor[3];

      light[lightIndex]->GetDiffuseColor( lightColor );

      double lightPosition[3];
      double lightFocalPoint[3];
      light[lightIndex]->GetTransformedPosition( lightPosition );
      light[lightIndex]->GetTransformedFocalPoint( lightFocalPoint );

      volumeTransform->TransformPoint( lightPosition, lightPosition );
      volumeTransform->TransformPoint( lightFocalPoint, lightFocalPoint );

      dir[0] = lightPosition[0] - lightFocalPoint[0];
      dir[1] = lightPosition[1] - lightFocalPoint[1];
      dir[2] = lightPosition[2] - lightFocalPoint[2];

      vtkMath::Normalize( dir );

      lightDiffuseColor[lightIndex][0] = lightColor[0]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][1] = lightColor[1]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][2] = lightColor[2]*diffuse*lightIntensity;
      lightDiffuseColor[lightIndex][3] = 0.0;

      lightSpecularColor[lightIndex][0]= lightColor[0]*specular*lightIntensity;
      lightSpecularColor[lightIndex][1]= lightColor[1]*specular*lightIntensity;
      lightSpecularColor[lightIndex][2]= lightColor[2]*specular*lightIntensity;
      lightSpecularColor[lightIndex][3] = 0.0;

      half[0] = dir[0] - viewDirection[0];
      half[1] = dir[1] - viewDirection[1];
      half[2] = dir[2] - viewDirection[2];

      vtkMath::Normalize( half );

      ambientColor[0] += ambient*lightColor[0];
      ambientColor[1] += ambient*lightColor[1];
      ambientColor[2] += ambient*lightColor[2];
      }

    lightDirection[lightIndex][0] = dir[0];
    lightDirection[lightIndex][1] = dir[1];
    lightDirection[lightIndex][2] = dir[2];
    lightDirection[lightIndex][3] = 0.0;

    halfwayVector[lightIndex][0] = half[0];
    halfwayVector[lightIndex][1] = half[1];
    halfwayVector[lightIndex][2] = half[2];
    halfwayVector[lightIndex][3] = 0.0;
    }

  volumeTransform->Delete();

  vtkgl::ProgramLocalParameter4fARB( vtkgl::FRAGMENT_PROGRAM_ARB, 0,
              lightDirection[0][0],
              lightDirection[0][1],
              lightDirection[0][2],
              lightDirection[0][3] );

  vtkgl::ProgramLocalParameter4fARB( vtkgl::FRAGMENT_PROGRAM_ARB, 1,
              halfwayVector[0][0],
              halfwayVector[0][1],
              halfwayVector[0][2],
              halfwayVector[0][3] );

  vtkgl::ProgramLocalParameter4fARB( vtkgl::FRAGMENT_PROGRAM_ARB, 2,
              ambient, diffuse, specular, specularPower );

  vtkgl::ProgramLocalParameter4fARB( vtkgl::FRAGMENT_PROGRAM_ARB, 3,
              lightDiffuseColor[0][0],
              lightDiffuseColor[0][1],
              lightDiffuseColor[0][2],
              lightDiffuseColor[0][3] );

  vtkgl::ProgramLocalParameter4fARB( vtkgl::FRAGMENT_PROGRAM_ARB, 4,
              lightSpecularColor[0][0],
              lightSpecularColor[0][1],
              lightSpecularColor[0][2],
              lightSpecularColor[0][3] );

  vtkgl::ProgramLocalParameter4fARB( vtkgl::FRAGMENT_PROGRAM_ARB, 5,
              viewDirection[0],
              viewDirection[1],
              viewDirection[2],
              viewDirection[3] );

  vtkgl::ProgramLocalParameter4fARB( vtkgl::FRAGMENT_PROGRAM_ARB, 6,
                                     2.0, -1.0, 0.0, 0.0 );
}

int  vtkSlicerOpenGLVolumeTextureMapper3D::IsRenderSupported(
  vtkRenderWindow* window, vtkVolumeProperty *property )
{
  if (window)
    {
    window->MakeCurrent();
    }
  if ( !this->Initialized )
    {
    this->Initialize(window);
    }

  if ( this->RenderMethod == vtkSlicerVolumeTextureMapper3D::NO_METHOD )
    {
    return 0;
    }

  if ( !this->GetInput() )
    {
    return 0;
    }

  if ( this->GetInput()->GetNumberOfScalarComponents() > 1 &&
       property->GetIndependentComponents() )
    {
    return 0;
    }

  return 1;
}

void vtkSlicerOpenGLVolumeTextureMapper3D::Initialize(vtkRenderWindow* window)
{
  this->Initialized = 1;
  vtkOpenGLExtensionManager * extensions = vtkOpenGLExtensionManager::New();
  if (window)
    {
    extensions->SetRenderWindow(window); // set render window to the current one.
    }

  int supports_texture3D=extensions->ExtensionSupported( "GL_VERSION_1_2" );
  if(supports_texture3D)
    {
    extensions->LoadExtension("GL_VERSION_1_2");
    }
  else
    {
    supports_texture3D=extensions->ExtensionSupported( "GL_EXT_texture3D" );
    if(supports_texture3D)
      {
      extensions->LoadCorePromotedExtension("GL_EXT_texture3D");
      }
    }

  int supports_multitexture=extensions->ExtensionSupported( "GL_VERSION_1_3" );
  if(supports_multitexture)
    {
    extensions->LoadExtension("GL_VERSION_1_3");
    }
  else
    {
    supports_multitexture=
      extensions->ExtensionSupported("GL_ARB_multitexture");
    if(supports_multitexture)
      {
      extensions->LoadCorePromotedExtension("GL_ARB_multitexture");
      }
    }

  int supports_GL_NV_texture_shader2     = extensions->ExtensionSupported( "GL_NV_texture_shader2" );
  int supports_GL_NV_register_combiners2 = extensions->ExtensionSupported( "GL_NV_register_combiners2" );
  int supports_GL_ATI_fragment_shader    = extensions->ExtensionSupported( "GL_ATI_fragment_shader" );
  int supports_GL_ARB_fragment_program   = extensions->ExtensionSupported( "GL_ARB_fragment_program" );
  int supports_GL_ARB_vertex_program     = extensions->ExtensionSupported( "GL_ARB_vertex_program" );
  int supports_GL_NV_register_combiners  = extensions->ExtensionSupported( "GL_NV_register_combiners" );

  if(supports_GL_NV_texture_shader2)
    {
    extensions->LoadExtension("GL_NV_texture_shader2" );
    }

  if(supports_GL_NV_register_combiners2)
    {
    extensions->LoadExtension( "GL_NV_register_combiners2" );
    }

  if(supports_GL_ATI_fragment_shader)
    {
    extensions->LoadExtension( "GL_ATI_fragment_shader" );
    }

  if(supports_GL_ARB_fragment_program)
    {
    extensions->LoadExtension( "GL_ARB_fragment_program" );
    }

  if(supports_GL_ARB_vertex_program)
    {
    extensions->LoadExtension( "GL_ARB_vertex_program" );
    }

  if(supports_GL_NV_register_combiners)
    {
    extensions->LoadExtension( "GL_NV_register_combiners" );
    }

  extensions->Delete();


  int canDoFP = 0;
  int canDoNV = 0;

  if ( supports_texture3D          &&
       supports_multitexture       &&
       supports_GL_ARB_fragment_program   &&
       supports_GL_ARB_vertex_program     &&
       vtkgl::TexImage3D               &&
       vtkgl::ActiveTexture            &&
       vtkgl::MultiTexCoord3fv         &&
       vtkgl::GenProgramsARB              &&
       vtkgl::DeleteProgramsARB           &&
       vtkgl::BindProgramARB              &&
       vtkgl::ProgramStringARB            &&
       vtkgl::ProgramLocalParameter4fARB )
    {
    canDoFP = 1;
    }
  else if ( supports_texture3D          &&
            supports_multitexture       &&
            supports_GL_NV_texture_shader2     &&
            supports_GL_NV_register_combiners2 &&
            supports_GL_NV_register_combiners  &&
            vtkgl::TexImage3D               &&
            vtkgl::ActiveTexture            &&
            vtkgl::MultiTexCoord3fv         &&
            vtkgl::CombinerParameteriNV        &&
            vtkgl::CombinerStageParameterfvNV  &&
            vtkgl::CombinerInputNV             &&
            vtkgl::CombinerOutputNV            &&
            vtkgl::FinalCombinerInputNV )
    {
    canDoNV = 1;
    }

  // can't do either
  if ( !canDoFP && !canDoNV )
    {
    this->RenderMethod = vtkSlicerVolumeTextureMapper3D::NO_METHOD;
    }
  // can only do FragmentProgram
  else if ( canDoFP && !canDoNV )
    {
    this->RenderMethod = vtkSlicerVolumeTextureMapper3D::FRAGMENT_PROGRAM_METHOD;
    }
  // can only do NVidia method
  else if ( !canDoFP && canDoNV )
    {
    this->RenderMethod = vtkSlicerVolumeTextureMapper3D::NVIDIA_METHOD;
    }
  // can do both - pick the preferred one
  else
    {
    this->RenderMethod = this->PreferredRenderMethod;
    }
}

int vtkSlicerOpenGLVolumeTextureMapper3D::IsTextureSizeSupported( int size[3] )
{
  if ( this->GetInput()->GetNumberOfScalarComponents() < 4 )
    {
    long maxSize = this->InternalVolumeSize * this->InternalVolumeSize * this->InternalVolumeSize;

    if ( size[0]*size[1]*size[2] > maxSize )//need to test graphics memory to determine volume size
      {
      return 0;
      }

    vtkgl::TexImage3D(vtkgl::PROXY_TEXTURE_3D, 0, GL_RGBA8, size[0]*2,
                      size[1]*2, size[2], 0, GL_RGBA, GL_UNSIGNED_BYTE,
                      this->Volume2 );
    }
  else
    {
    if ( size[0]*size[1]*size[2] > 128*128*128 )
      {
      return 0;
      }

    vtkgl::TexImage3D( vtkgl::PROXY_TEXTURE_3D, 0, GL_RGBA8, size[0]*2,
                       size[1]*2, size[2]*2, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                       this->Volume2 );
    }


  GLint params[1];
  glGetTexLevelParameteriv ( vtkgl::PROXY_TEXTURE_3D, 0, GL_TEXTURE_WIDTH,
                             params );

  if ( params[0] != 0 )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

// Print the vtkSlicerOpenGLVolumeTextureMapper3D
void vtkSlicerOpenGLVolumeTextureMapper3D::PrintSelf(ostream& os, vtkIndent indent)
{

  vtkOpenGLExtensionManager * extensions = vtkOpenGLExtensionManager::New();
  extensions->SetRenderWindow(NULL); // set render window to current render window

  os << indent << "Initialized " << this->Initialized << endl;
  if ( this->Initialized )
    {
    os << indent << "Supports GL_VERSION_1_2:"
       << extensions->ExtensionSupported( "GL_VERSION_1_2" ) << endl;
    os << indent << "Supports GL_EXT_texture3D:"
       << extensions->ExtensionSupported( "GL_EXT_texture3D" ) << endl;
     os << indent << "Supports GL_VERSION_1_3:"
       << extensions->ExtensionSupported( "GL_VERSION_1_3" ) << endl;
    os << indent << "Supports GL_ARB_multitexture: "
       << extensions->ExtensionSupported( "GL_ARB_multitexture" ) << endl;
    os << indent << "Supports GL_NV_texture_shader2: "
       << extensions->ExtensionSupported( "GL_NV_texture_shader2" ) << endl;
    os << indent << "Supports GL_NV_register_combiners2: "
       << extensions->ExtensionSupported( "GL_NV_register_combiners2" ) << endl;
    os << indent << "Supports GL_ATI_fragment_shader: "
       << extensions->ExtensionSupported( "GL_ATI_fragment_shader" ) << endl;
    os << indent << "Supports GL_ARB_fragment_program: "
       << extensions->ExtensionSupported( "GL_ARB_fragment_program" ) << endl;
    }
  extensions->Delete();

  this->Superclass::PrintSelf(os,indent);
}


