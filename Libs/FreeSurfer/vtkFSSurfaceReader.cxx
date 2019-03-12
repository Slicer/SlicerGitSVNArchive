/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkFSSurfaceReader.cxx,v $
  Date:      $Date: 2006/08/04 18:48:03 $
  Version:   $Revision: 1.14 $

=========================================================================auto=*/

// FreeSurfer includes
#include "vtkFSIO.h"
#include "vtkFSSurfaceReader.h"

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkByteSwap.h>
#include <vtkCellArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkPolyData.h>
#include <vtkStreamingDemandDrivenPipeline.h>

//-------------------------------------------------------------------------
vtkStandardNewMacro(vtkFSSurfaceReader);

//-------------------------------------------------------------------------
vtkFSSurfaceReader::vtkFSSurfaceReader()
{
  this->FileName = nullptr;
  this->SetNumberOfInputPorts(0);
}

//-------------------------------------------------------------------------
vtkFSSurfaceReader::~vtkFSSurfaceReader()
{
  delete[] this->FileName;
  this->FileName = nullptr;
}

//----------------------------------------------------------------------------
int vtkFSSurfaceReader::RequestData(
        vtkInformation *,
        vtkInformationVector **,
        vtkInformationVector *outputVector)
{
  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the output
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  FILE* surfaceFile;
  int magicNumber;
  char line[256];
  int numVertices = 0;
  int numFaces = 0;
  int vIndex, fIndex;
  int numVerticesPerFace = 0;
  int tmpX, tmpY, tmpZ;
  int tmpfIndex = 0;
  float locations[3];
  int fvIndex;
  int faceIncrement = 1;
  int faceMultiplier = 1;
  vtkIdType faceIndices[4];
  vtkPoints *outputVertices;
  vtkCellArray *outputFaces;

#if FS_CALC_NORMALS
  vtkFloatArray *outputNormals;
  FSVertex* vertices;
  FSFace* faces;
  FSVertex* v;
  FSFace* f;
  FSVertex* fv0;
  FSVertex* fv1;
  FSVertex* fv2;
  int fvIndex1, fvIndex2, fvIndex0;
  float faceVector0[3], faceVector1[3];
  float faceNormal[3];
  float length;
#endif
  int totalSteps = 1;
  int thisStep = 0;

  vtkDebugMacro(<<"RequestData: Reading vtk polygonal data...");

  // Try to open the file.
  surfaceFile = fopen(this->GetFileName(), "rb") ;
  if (!surfaceFile) {
    vtkErrorMacro (<< "Could not open file " << this->GetFileName());
    return 1;
  }

  // Get the three byte magic number. We support two file types.
  vtkFSIO::ReadInt3 (surfaceFile, magicNumber);
  if (magicNumber != vtkFSSurfaceReader::FS_QUAD_FILE_MAGIC_NUMBER &&
      magicNumber != vtkFSSurfaceReader::FS_NEW_QUAD_FILE_MAGIC_NUMBER &&
      magicNumber != vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER) {
    vtkErrorMacro (<< "vtkFSSurfaceReader.cxx Execute: Wrong file type when loading " << this->GetFileName() << "\n magic number = " << magicNumber << ". Supported ar " << vtkFSSurfaceReader::FS_QUAD_FILE_MAGIC_NUMBER << ", " << vtkFSSurfaceReader::FS_NEW_QUAD_FILE_MAGIC_NUMBER << ", and " << vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER );
    return 1;
  }

#if FS_DEBUG
  switch (magicNumber) {
  case vtkFSSurfaceReader::FS_QUAD_FILE_MAGIC_NUMBER:
    cerr << "Reading old quad file" << endl;
    break;
  case vtkFSSurfaceReader::FS_NEW_QUAD_FILE_MAGIC_NUMBER:
    cerr << "Reading new quad file" << endl;
    break;
  case vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER:
    cerr << "Reading triangle file" << endl;
    break;
  }
#endif


  // Triangle file has some kind of header string at the
  // beginning. Skip it.
  if (vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER == magicNumber)
    {
    char *skipchars = fgets (line, 200, surfaceFile);
    int skip = fscanf (surfaceFile, "\n");
    if (skipchars == nullptr || skip > 0)
      {
      // trying to avoid unused var warnings while checking return values
      }
    }

  // Triangle files use normal ints to store their number of vertices
  // and faces, while quad files use three byte ints.
  size_t retval;
  switch (magicNumber)
    {
    case vtkFSSurfaceReader::FS_QUAD_FILE_MAGIC_NUMBER:
    case vtkFSSurfaceReader::FS_NEW_QUAD_FILE_MAGIC_NUMBER:
      vtkFSIO::ReadInt3 (surfaceFile, numVertices);
      vtkFSIO::ReadInt3 (surfaceFile, numFaces);
      break;
    case vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER:
      retval = fread (&numVertices, sizeof(int), 1, surfaceFile);
      if (retval == 1)
        {
        vtkByteSwap::Swap4BE (&numVertices);
        }
      else
        {
        vtkErrorMacro("Error reading number of vertices");
        }
      retval = fread (&numFaces, sizeof(int), 1, surfaceFile);
      if (retval == 1)
        {
        vtkByteSwap::Swap4BE (&numFaces);
        }
      else
        {
        vtkErrorMacro("Error reading number of faces");
        }
      break;
    }

  // In quad files, we want to skip every other face but count twice
  // as many of them. This has to do with the way they are stored;
  // here we just generate quads where as in the old code they
  // generated tries from the quads. (Trust me.) In tri files, we use
  // every face.
  switch (magicNumber) {
  case vtkFSSurfaceReader::FS_QUAD_FILE_MAGIC_NUMBER:
  case vtkFSSurfaceReader::FS_NEW_QUAD_FILE_MAGIC_NUMBER:
    faceIncrement = 2;
    faceMultiplier = 2;
    break;
  case vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER:
    faceIncrement = 1;
    faceMultiplier = 1;
    break;
  }

#if FS_DEBUG
  cerr << numVertices << " vertices, " << numFaces * faceMultiplier << " faces" << endl;
#endif

  // If quad files, there are four vertices per face, in tri files,
  // there are three.
  switch (magicNumber) {
  case vtkFSSurfaceReader::FS_QUAD_FILE_MAGIC_NUMBER:
  case vtkFSSurfaceReader::FS_NEW_QUAD_FILE_MAGIC_NUMBER:
    numVerticesPerFace = vtkFSSurfaceReader::FS_NUM_VERTS_IN_QUAD_FACE;
    break;
  case vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER:
    numVerticesPerFace = vtkFSSurfaceReader::FS_NUM_VERTS_IN_TRI_FACE;
    break;
  }

  // Allocate our VTK arrays.
  outputVertices = vtkPoints::New();
  outputVertices->Allocate (numVertices);
  outputFaces = vtkCellArray::New();
  outputFaces->Allocate (outputFaces->EstimateSize(numFaces,
                           numVerticesPerFace));
#if FS_CALC_NORMALS
  outputNormals = vtkFloatArray::New();
  outputNormals->Allocate (numVertices);
  outputNormals->SetNumberOfComponents (3);
  outputNormals->SetName ("Normals");

  // Allocate our vertex and face connectivity arrays for calculating
  // the normals. If we can't, no big deal.
  vertices = (FSVertex*) calloc (numVertices, sizeof(FSVertex));
  faces = (FSFace*) calloc (numFaces, sizeof(FSFace));
  if (nullptr == vertices || nullptr == faces) {
    vtkErrorMacro (<< "Couldn't allocate vertex or face connectivity "
           "structures, unable to calc normals");
  }
#endif

  totalSteps = numVertices + (numFaces*faceMultiplier/faceIncrement * numVerticesPerFace);
#if FS_CALC_NORMALS
  if (nullptr != vertices && nullptr != faces) {
      for (vIndex = 0; vIndex < numVertices; vIndex++) {
          fv1 = &vertices[vIndex];
          for (fvIndex = 0; fvIndex < fv1->numFaces; fvIndex++) {
              totalSteps++;
          }
      }
  }
#endif
  vtkDebugMacro(<<"Got total steps = " << totalSteps);

  // For each vertex...
  for (vIndex = 0; vIndex < numVertices; vIndex++) {

      thisStep++;

      // Depending on the file type, read in three two bytes ints and
      // convert them from meters to millimeters or read in three floats
      // in millimeters. Insert them into the vertices array.  The old
      // quad format uses the ints and the new quad and triangle formats
      // use floats.
      switch (magicNumber) {
      case vtkFSSurfaceReader::FS_QUAD_FILE_MAGIC_NUMBER:
          vtkFSIO::ReadInt2 (surfaceFile, tmpX);
          vtkFSIO::ReadInt2 (surfaceFile, tmpY);
          vtkFSIO::ReadInt2 (surfaceFile, tmpZ);
          locations[0] = (float)tmpX / 100.0;
          locations[1] = (float)tmpY / 100.0;
          locations[2] = (float)tmpZ / 100.0;
          break;
      case vtkFSSurfaceReader::FS_NEW_QUAD_FILE_MAGIC_NUMBER:
      case vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER:
          vtkFSIO::ReadFloat (surfaceFile, locations[0]);
          vtkFSIO::ReadFloat (surfaceFile, locations[1]);
          vtkFSIO::ReadFloat (surfaceFile, locations[2]);
          break;
      }

      outputVertices->InsertNextPoint (locations);

#if FS_CALC_NORMALS
    // If we have connectivity info, fill out the location and
    // initialize the normals to 0 for this vertex.
      if (nullptr != vertices && nullptr != faces) {
          v = &vertices[vIndex];
          v->x = locations[0];
          v->y = locations[1];
          v->z = locations[2];
          v->nx = 0;
          v->ny = 0;
          v->nz = 0;
          v->numFaces = 0;
      }

#endif
      if ((thisStep % 1000) == 0)
      {
          this->UpdateProgress(1.0*thisStep/totalSteps);
      }
  }

  // For each face...
  for (fIndex = 0;
       fIndex < numFaces * faceMultiplier;
       fIndex += faceIncrement) {

    // For each vertex in the face...
    for (fvIndex = 0; fvIndex < numVerticesPerFace; fvIndex++) {

        thisStep++;

        // Read in a vertex index. Triangle format gets a normal int,
        // quad formats get three byte ints.
        switch (magicNumber) {
        case vtkFSSurfaceReader::FS_QUAD_FILE_MAGIC_NUMBER:
        case vtkFSSurfaceReader::FS_NEW_QUAD_FILE_MAGIC_NUMBER:
            vtkFSIO::ReadInt3 (surfaceFile, tmpfIndex);
            break;
        case vtkFSSurfaceReader::FS_TRIANGLE_FILE_MAGIC_NUMBER:
            retval = fread (&tmpfIndex, sizeof(int), 1, surfaceFile);
            if (retval == 1)
              {
              vtkByteSwap::Swap4BE (&tmpfIndex);
              }
            else
              {
              vtkErrorMacro("Error reading integer at index " << fvIndex);
              }
            break;
        }

        faceIndices[fvIndex] = tmpfIndex;

#if FS_CALC_NORMALS
        // Fill out connectivity info for this face. Get the vertex from
        // the vertex index and add this face index to its list of
        // faces. Then add this vertex index to the list of indices in
        // the face.
        if (nullptr != vertices && nullptr != faces) {
            v = &vertices[tmpfIndex];
            v->faces[v->numFaces] = fIndex;
            v->indicesInFace[v->numFaces] = fvIndex;
            v->numFaces++;

            f = &faces[fIndex];
            f->vertices[fvIndex] = tmpfIndex;
        }
#endif
    }

    // Add the face to the list.
    outputFaces->InsertNextCell (numVerticesPerFace, faceIndices);
    if ((thisStep % 1000) == 0)
    {
        this->UpdateProgress(1.0*thisStep/totalSteps);
    }
  }

  // Close the surface file.
  fclose (surfaceFile);

#if FS_DEBUG
  cerr << "Done reading surface." << endl;
#endif

#if FS_CALC_NORMALS

  // If we allocated the space for our vertex and face connectivity,
  // calculate the normals.
  if (nullptr != vertices && nullptr != faces) {
      // For each vertex...
      for (vIndex = 0; vIndex < numVertices; vIndex++) {

          // For each face it is a part of...
          fv1 = &vertices[vIndex];
          for (fIndex = 0; fIndex < fv1->numFaces; fIndex++) {

              thisStep++;

              // Get this face. fv1->indicesInFace tells us which index,
              // 0 - numVerticesPerFace-1, it is in the face. Get the two
              // indices surrounding it so we can get the vertices
              // surrounding it. Now we have fv0, fv1, and fv2 which are all
              // adjacent in the face, with fv1 being the original vertex.
              f = &faces[fv1->faces[fIndex]];
              fvIndex1 = fv1->indicesInFace[fIndex];
              if (0 == fvIndex1)
                  fvIndex0 = numVerticesPerFace - 1;
              else
                  fvIndex0 = fvIndex1 - 1;
              if (numVerticesPerFace == fvIndex1)
                  fvIndex2 = 0;
              else
                  fvIndex2 = fvIndex1 + 1;

              fv0 = &vertices[f->vertices[fvIndex0]];
              fv2 = &vertices[f->vertices[fvIndex2]];

              // Get two vectors from these points and normalize them. Then
              // just cross them to get the perpendicular vector. This is
              // the normal for the face. Add this to the normal for the
              // vertex.
              faceVector0[0] = fv1->x - fv0->x;
              faceVector0[1] = fv1->y - fv0->y;
              faceVector0[2] = fv1->z - fv0->z;
              faceVector1[0] = fv2->x - fv1->x;
              faceVector1[1] = fv2->y - fv1->y;
              faceVector1[2] = fv2->z - fv1->z;
              length = sqrt( faceVector0[0]*faceVector0[0] +
                             faceVector0[1]*faceVector0[1] +
                             faceVector0[2]*faceVector0[2] );
              if (length > 0) {
                  faceVector0[0] /= length;
                  faceVector0[1] /= length;
                  faceVector0[2] /= length;
              }
              length = sqrt( faceVector1[0]*faceVector1[0] +
                             faceVector1[1]*faceVector1[1] +
                             faceVector1[2]*faceVector1[2] );
              if (length > 0) {
                  faceVector1[0] /= length;
                  faceVector1[1] /= length;
                  faceVector1[2] /= length;
              }

              // get the cross product
              faceNormal[0] = -faceVector1[1]*faceVector0[2] +
                  faceVector0[1]*faceVector1[2];
              faceNormal[1] = faceVector1[0]*faceVector0[2] -
                  faceVector0[0]*faceVector1[2];
              faceNormal[2] = -faceVector1[0]*faceVector0[1] +
                  faceVector0[0]*faceVector1[1];

              // add it to the normal vector at this vertex.
              fv1->nx += faceNormal[0];
              fv1->ny += faceNormal[1];
              fv1->nz += faceNormal[2];
          }

          // When all the faces for this vertex have been processed, the
          // normal at the vertex is the sum of all the normals for the
          // adjacent faces. Normalize it and we're done.
          faceNormal[0] = fv1->nx;
          faceNormal[1] = fv1->ny;
          faceNormal[2] = fv1->nz;
          length = sqrt( faceNormal[0]*faceNormal[0] +
                         faceNormal[1]*faceNormal[1] +
                         faceNormal[2]*faceNormal[2] );
          if (length > 0) {
              faceNormal[0] /= length;
              faceNormal[1] /= length;
              faceNormal[2] /= length;
          }

          if ((thisStep % 1000) == 0)
          {
              this->UpdateProgress(1.0*thisStep/totalSteps);
          }

          // Add the final normal to the array.
          outputNormals->InsertNextTuple(faceNormal);
      }

      free (vertices);
      free (faces);
  }
#endif

  // Set all the arrays in the output.
  output->SetPoints (outputVertices);
  outputVertices->Delete();

  this->SetProgressText("");
  this->UpdateProgress(0.0);

#if FS_CALC_NORMALS
  output->GetPointData()->SetNormals (outputNormals);
  outputNormals->Delete();
#endif


  outputFaces->Squeeze();
  output->SetPolys(outputFaces);
  outputFaces->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void vtkFSSurfaceReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
