/*==============================================================================

  Copyright (c) Laboratory for Percutaneous Surgery (PerkLab)
  Queen's University, Kingston, ON, Canada. All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Adam Rankin and Csaba Pinter, PerkLab, Queen's
  University and was supported through the Applied Cancer Research Unit program of Cancer
  Care Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// Segmentations includes
#include "vtkMRMLSegmentationStorageNode.h"

#include "vtkSegmentation.h"
#include "vtkOrientedImageData.h"
#include "vtkOrientedImageDataResample.h"

// MRML includes
#include <vtkMRMLScene.h>
#include "vtkMRMLSegmentationNode.h"
#include "vtkMRMLSegmentationDisplayNode.h"

// VTK includes
#include <vtkDataObject.h>
#include <vtkDoubleArray.h>
#include <vtkFieldData.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageCast.h>
#include <vtkImageConstantPad.h>
#include <vtkImageExtractComponents.h>
#include <vtkInformation.h>
#include <vtkInformationIntegerVectorKey.h>
#include <vtkInformationStringKey.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkNew.h>
#include <vtkNRRDReader.h>
#include <vtkNRRDWriter.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkStringArray.h>
#include <vtkTransform.h>
#include <vtkXMLMultiBlockDataWriter.h>
#include <vtkXMLMultiBlockDataReader.h>
#include <vtksys/SystemTools.hxx>

#ifdef SUPPORT_4D_SPATIAL_NRRD
// ITK includes
#include <itkImageFileWriter.h>
#include <itkNrrdImageIO.h>
#include <itkExceptionObject.h>
#include <itkImageFileWriter.h>
#include <itkImageFileReader.h>
#include <itkMetaDataDictionary.h>
#include <itkMetaDataObject.h>
#endif

// STL & C++ includes
#include <iterator>
#include <sstream>

//----------------------------------------------------------------------------
static const std::string SERIALIZATION_SEPARATOR = "|";
static const std::string KEY_SEGMENT_ID = "ID";
static const std::string KEY_SEGMENT_NAME = "Name";
static const std::string KEY_SEGMENT_DEFAULT_COLOR = "DefaultColor";
static const std::string KEY_SEGMENT_TAGS = "Tags";
static const std::string KEY_SEGMENT_EXTENT = "Extent";
static const std::string KEY_SEGMENTATION_MASTER_REPRESENTATION = "MasterRepresentation";
static const std::string KEY_SEGMENTATION_CONVERSION_PARAMETERS = "ConversionParameters";
static const std::string KEY_SEGMENTATION_EXTENT = "Extent";
static const std::string KEY_SEGMENTATION_CONTAINED_REPRESENTATION_NAMES = "ContainedRepresentationNames";

static const int SINGLE_SEGMENT_INDEX = -1; // used as segment index when there is only a single segment
//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSegmentationStorageNode);

//----------------------------------------------------------------------------
vtkMRMLSegmentationStorageNode::vtkMRMLSegmentationStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationStorageNode::~vtkMRMLSegmentationStorageNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLStorageNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();

  Superclass::ReadXMLAttributes(atts);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, StorageID
void vtkMRMLSegmentationStorageNode::Copy(vtkMRMLNode *anode)
{
  int disabledModify = this->StartModify();

  Superclass::Copy(anode);

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Segmentation 4D NRRD volume (.seg.nrrd)");
  this->SupportedReadFileTypes->InsertNextValue("Segmentation Multi-block dataset (.seg.vtm)");
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::InitializeSupportedWriteFileTypes()
{
  Superclass::InitializeSupportedWriteFileTypes();

  vtkMRMLSegmentationNode* segmentationNode = this->GetAssociatedDataNode();
  if (segmentationNode)
    {
    const char* masterRepresentation = segmentationNode->GetSegmentation()->GetMasterRepresentationName();
    if (masterRepresentation)
      {
      if (!strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
        {
        // Binary labelmap -> 4D NRRD volume
        this->SupportedWriteFileTypes->InsertNextValue("Segmentation 4D NRRD volume (.seg.nrrd)");
        }
      else if ( !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
             || !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()) )
        {
        // Closed surface or planar contours -> MultiBlock polydata
        this->SupportedWriteFileTypes->InsertNextValue("Segmentation Multi-block dataset (.seg.vtm)");
        }
      }
    }
}

//----------------------------------------------------------------------------
vtkMRMLSegmentationNode* vtkMRMLSegmentationStorageNode::GetAssociatedDataNode()
{
  if (!this->GetScene())
    {
    return NULL;
    }

  std::vector<vtkMRMLNode*> segmentationNodes;
  unsigned int numberOfNodes = this->GetScene()->GetNodesByClass("vtkMRMLSegmentationNode", segmentationNodes);
  for (unsigned int nodeIndex=0; nodeIndex<numberOfNodes; nodeIndex++)
    {
    vtkMRMLSegmentationNode* node = vtkMRMLSegmentationNode::SafeDownCast(segmentationNodes[nodeIndex]);
    if (node)
      {
      const char* storageNodeID = node->GetStorageNodeID();
      if (storageNodeID && !strcmp(storageNodeID, this->ID))
        {
        return vtkMRMLSegmentationNode::SafeDownCast(node);
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkMRMLSegmentationStorageNode::GetDefaultWriteFileExtension()
{
  vtkMRMLSegmentationNode* segmentationNode = this->GetAssociatedDataNode();
  if (segmentationNode)
    {
    const char* masterRepresentation = segmentationNode->GetSegmentation()->GetMasterRepresentationName();
    if (masterRepresentation)
      {
      if (!strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
        {
        // Binary labelmap -> 4D NRRD volume
        return "seg.nrrd";
        }
      else if ( !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
             || !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()) )
        {
        // Closed surface or planar contours -> MultiBlock polydata
        return "seg.vtm";
        }
      }
    }

  // Master representation is not supported for writing to file
  return NULL;
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::ResetSupportedWriteFileTypes()
{
  this->InitializeSupportedWriteFileTypes();
}

//----------------------------------------------------------------------------
bool vtkMRMLSegmentationStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSegmentationNode");
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSegmentationNode* segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(refNode);
  if (!segmentationNode)
    {
    vtkErrorMacro("ReadDataInternal: Reference node is not a segmentation node");
    return 0;
    }

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
    {
    vtkErrorMacro("ReadDataInternal: File name not specified");
    return 0;
    }

  // Check that the file exists
  if (vtksys::SystemTools::FileExists(fullName.c_str()) == false)
    {
    vtkErrorMacro("ReadDataInternal: segmentation file '" << fullName.c_str() << "' not found.");
    return 0;
    }

  // Try to read as labelmap first then as poly data
  if (this->ReadBinaryLabelmapRepresentation(segmentationNode, fullName))
    {
    return 1;
    }
#ifdef SUPPORT_4D_SPATIAL_NRRD
  else if (this->ReadBinaryLabelmapRepresentation4DSpatial(segmentationNode, fullName))
    {
    return 1;
    }
#endif
  else if (this->ReadPolyDataRepresentation(segmentationNode, fullName))
    {
    return 1;
    }

  // Failed to read
  vtkErrorMacro("ReadDataInternal: File " << fullName << " could not be read neither as labelmap nor poly data");
  return 0;
}

#ifdef SUPPORT_4D_SPATIAL_NRRD
//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::ReadBinaryLabelmapRepresentation4DSpatial(vtkMRMLSegmentationNode* segmentationNode, std::string path)
{
  if (!vtksys::SystemTools::FileExists(path.c_str()))
    {
    vtkErrorMacro("ReadBinaryLabelmapRepresentation: Input file " << path << " does not exist!");
    return 0;
    }

  // Set up output segmentation
  if (!segmentationNode || segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
    {
    vtkErrorMacro("ReadBinaryLabelmapRepresentation: Output segmentation must exist and must be empty!");
    return 0;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();

  // Get display node to load displayed color and opacity
  segmentationNode->CreateDefaultDisplayNodes();

  // Read 4D NRRD image file
  typedef itk::ImageFileReader<BinaryLabelmap4DImageType> FileReaderType;
  FileReaderType::Pointer reader = FileReaderType::New();
  reader->SetFileName(path);
  try
    {
    reader->Update();
    }
  catch (itk::ImageFileReaderException &error)
    {
    // Do not report error as the file might contain poly data in which case ReadPolyDataRepresentation will read it alright
    vtkDebugMacro("ReadBinaryLabelmapRepresentation: Failed to load file " << path << " as segmentation. Exception:\n" << error);
    return 0;
    }
  BinaryLabelmap4DImageType::Pointer allSegmentLabelmapsImage = reader->GetOutput();

  // Read succeeded, set master representation
  segmentation->SetMasterRepresentationName(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());

  // Get metadata dictionary from image
  itk::MetaDataDictionary metadata = allSegmentLabelmapsImage->GetMetaDataDictionary();
  // Read common geometry extent
  std::string commonExtent;
  itk::ExposeMetaData<std::string>(metadata, GetSegmentationMetaDataKey(KEY_SEGMENTATION_EXTENT).c_str(), commonExtent);
  int commonGeometryExtent[6] = {0,-1,0,-1,0,-1};
  GetImageExtentFromString(commonGeometryExtent, commonExtent);
  // Read conversion parameters
  std::string conversionParameters;
  itk::ExposeMetaData<std::string>(metadata, GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONVERSION_PARAMETERS).c_str(), conversionParameters);
  segmentation->DeserializeConversionParameters(conversionParameters);
  // Read contained representation names
  std::string containedRepresentationNames;
  itk::ExposeMetaData<std::string>(metadata, GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONTAINED_REPRESENTATION_NAMES).c_str(), containedRepresentationNames);

  // Get image properties
  BinaryLabelmap4DImageType::RegionType itkRegion = allSegmentLabelmapsImage->GetLargestPossibleRegion();
  BinaryLabelmap4DImageType::PointType itkOrigin = allSegmentLabelmapsImage->GetOrigin();
  BinaryLabelmap4DImageType::SpacingType itkSpacing = allSegmentLabelmapsImage->GetSpacing();
  BinaryLabelmap4DImageType::DirectionType itkDirections = allSegmentLabelmapsImage->GetDirection();
  // Make image properties accessible for VTK
  double origin[3] = {itkOrigin[0], itkOrigin[1], itkOrigin[2]};
  double spacing[3] = {itkSpacing[0], itkSpacing[1], itkSpacing[2]};
  double directions[3][3] = {{1.0,0.0,0.0},{0.0,1.0,0.0},{0.0,0.0,1.0}};
  for (unsigned int col=0; col<3; col++)
    {
    for (unsigned int row=0; row<3; row++)
      {
      directions[row][col] = itkDirections[row][col];
      }
    }

  // Read segment binary labelmaps
  for (unsigned int segmentIndex = itkRegion.GetIndex()[3];
       segmentIndex < itkRegion.GetIndex()[3]+itkRegion.GetSize()[3];
       ++segmentIndex)
    {
    // Create segment
    vtkSmartPointer<vtkSegment> currentSegment = vtkSmartPointer<vtkSegment>::New();

    // Get metadata for current segment

    // ID
    std::string currentSegmentID;
    itk::ExposeMetaData<std::string>(
          metadata,
          GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_ID),
          currentSegmentID);

    // Name
    std::string currentSegmentName;
    itk::ExposeMetaData<std::string>(
          metadata,
          GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_NAME),
          currentSegmentName);
    currentSegment->SetName(currentSegmentName.c_str());

    // DefaultColor
    std::string defaultColorValue;
    itk::ExposeMetaData<std::string>(
          metadata,
          GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_DEFAULT_COLOR),
          defaultColorValue);
    double currentSegmentDefaultColor[3] = {0.0,0.0,0.0};
    GetSegmentDefaultColorFromString(currentSegmentDefaultColor, defaultColorValue);

    // Extent
    std::string extentValue;
    itk::ExposeMetaData<std::string>(
          metadata,
          GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_EXTENT),
          extentValue);
    int currentSegmentExtent[6] = {0,-1,0,-1,0,-1};
    GetImageExtentFromString(currentSegmentExtent, extentValue);

    // Tags
    std::string tagsValue;
    itk::ExposeMetaData<std::string>(
          metadata,
          GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_TAGS),
          tagsValue);
    SetSegmentTagsFromString(currentSegment, tagsValue);

    // Create binary labelmap volume
    vtkSmartPointer<vtkOrientedImageData> currentBinaryLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
    currentBinaryLabelmap->SetOrigin(origin);
    currentBinaryLabelmap->SetSpacing(spacing);
    currentBinaryLabelmap->SetDirections(directions);
    currentBinaryLabelmap->SetExtent(currentSegmentExtent);
    currentBinaryLabelmap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    unsigned char* labelmapPtr = (unsigned char*)currentBinaryLabelmap->GetScalarPointerForExtent(currentSegmentExtent);

    // Define ITK region for current segment
    BinaryLabelmap4DImageType::RegionType segmentRegion;
    BinaryLabelmap4DImageType::SizeType segmentRegionSize;
    BinaryLabelmap4DImageType::IndexType segmentRegionIndex;
    segmentRegionIndex[0] = segmentRegionIndex[1] = segmentRegionIndex[2] = 0;
    segmentRegionIndex[3] = segmentIndex;
    segmentRegionSize = itkRegion.GetSize();
    segmentRegionSize[3] = 1;
    segmentRegion.SetIndex(segmentRegionIndex);
    segmentRegion.SetSize(segmentRegionSize);

    // Iterate through current segment's region and read voxel values into segment labelmap
    BinaryLabelmap4DIteratorType segmentLabelmapIterator(allSegmentLabelmapsImage, segmentRegion);
    for (segmentLabelmapIterator.GoToBegin(); !segmentLabelmapIterator.IsAtEnd(); ++segmentLabelmapIterator)
      {
      // Skip region outside extent of current segment (consider common extent boundaries)
      BinaryLabelmap4DImageType::IndexType segmentIndex = segmentLabelmapIterator.GetIndex();
      if ( segmentIndex[0] + commonGeometryExtent[0] < currentSegmentExtent[0]
        || segmentIndex[0] + commonGeometryExtent[0] > currentSegmentExtent[1]
        || segmentIndex[1] + commonGeometryExtent[2] < currentSegmentExtent[2]
        || segmentIndex[1] + commonGeometryExtent[2] > currentSegmentExtent[3]
        || segmentIndex[2] + commonGeometryExtent[4] < currentSegmentExtent[4]
        || segmentIndex[2] + commonGeometryExtent[4] > currentSegmentExtent[5] )
        {
        continue;
        }

      // Get voxel value
      unsigned char voxelValue = segmentLabelmapIterator.Get();

      // Set voxel value in current segment labelmap
      (*labelmapPtr) = voxelValue;
      ++labelmapPtr;
      }

    // Set loaded binary labelmap to segment
    currentSegment->AddRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(), currentBinaryLabelmap);

    // Add segment to segmentation
    segmentation->AddSegment(currentSegment, currentSegmentID);
    }

  // Create contained representations now that all the data is loaded
  this->CreateRepresentationsBySerializedNames(segmentation, containedRepresentationNames);

  return 1;
}
#endif // SUPPORT_4D_SPATIAL_NRRD

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::ReadBinaryLabelmapRepresentation(vtkMRMLSegmentationNode* segmentationNode, std::string path)
{
  if (!vtksys::SystemTools::FileExists(path.c_str()))
    {
    vtkErrorMacro("ReadBinaryLabelmapRepresentation: Input file " << path << " does not exist!");
    return 0;
    }

  // Set up output segmentation
  if (!segmentationNode || segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
    {
    vtkErrorMacro("ReadBinaryLabelmapRepresentation: Output segmentation must exist and must be empty!");
    return 0;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();

  // Get display node to load displayed color and opacity
  segmentationNode->CreateDefaultDisplayNodes();

  vtkNew<vtkNRRDReader> reader;
  reader->SetFileName(path.c_str());

  // Check if this is a NRRD file that we can read
  if (!reader->CanReadFile(path.c_str()))
    {
    vtkDebugMacro("ReadBinaryLabelmapRepresentation: This is not a nrrd file");
    return 0;
    }

  // Read the header to see if the NRRD file corresponds to the
  // MRML Node
  reader->UpdateInformation();

  if (reader->GetPointDataType() != vtkDataSetAttributes::SCALARS)
    {
    vtkDebugMacro("ReadBinaryLabelmapRepresentation: only scalar point type is supported");
    return 0;
    }

  // Read the volume
  reader->Update();

  // Copy image data to sequence of volume nodes
  vtkImageData* imageData = reader->GetOutput();
  if (imageData == NULL)
    {
    vtkErrorMacro("vtkMRMLVolumeSequenceStorageNode::ReadDataInternal: invalid image data");
    return 0;
    }
  int numberOfFrames = imageData->GetNumberOfScalarComponents();

  // Read succeeded, set master representation
  segmentation->SetMasterRepresentationName(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName());

  int segmentationNodeWasModified = segmentationNode->StartModify();
  // Get metadata dictionary from image
  typedef std::vector<std::string> KeyVector;
  KeyVector keys = reader->GetHeaderKeysVector();

  // Read common geometry extent
  int commonGeometryExtent[6] = { 0, -1, 0, -1, 0, -1 };
  KeyVector::iterator kit = std::find(keys.begin(), keys.end(), GetSegmentationMetaDataKey(KEY_SEGMENTATION_EXTENT));
  if (kit != keys.end())
    {
    GetImageExtentFromString(commonGeometryExtent, reader->GetHeaderValue(GetSegmentationMetaDataKey(KEY_SEGMENTATION_EXTENT).c_str()));
    }

  int imageExtentInFile[6] = { 0, -1, 0, -1, 0, -1 };
  imageData->GetExtent(imageExtentInFile);
  if (imageExtentInFile[1] - imageExtentInFile[0] != commonGeometryExtent[1] - commonGeometryExtent[0]
    || imageExtentInFile[3] - imageExtentInFile[2] != commonGeometryExtent[3] - commonGeometryExtent[2]
    || imageExtentInFile[5] - imageExtentInFile[4] != commonGeometryExtent[5] - commonGeometryExtent[4])
    {
    vtkErrorMacro("vtkMRMLVolumeSequenceStorageNode::ReadDataInternal: " << GetSegmentationMetaDataKey(KEY_SEGMENTATION_EXTENT)<<" is inconsistent with the image size");
    return 0;
    }
  imageData->SetExtent(commonGeometryExtent);
  vtkNew<vtkImageExtractComponents> extractComponents;
  extractComponents->SetInputData(imageData);

  vtkNew<vtkImageConstantPad> padder;
  padder->SetInputConnection(extractComponents->GetOutputPort());

  // Read conversion parameters
  kit = std::find(keys.begin(), keys.end(), GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONVERSION_PARAMETERS));
  if (kit != keys.end())
    {
    std::string conversionParameters = reader->GetHeaderValue(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONVERSION_PARAMETERS).c_str());
    segmentation->DeserializeConversionParameters(conversionParameters);
    }

  // Read contained representation names
  std::string containedRepresentationNames;
  kit = std::find(keys.begin(), keys.end(), GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONTAINED_REPRESENTATION_NAMES));
  if (kit != keys.end())
    {
    containedRepresentationNames = reader->GetHeaderValue(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONTAINED_REPRESENTATION_NAMES).c_str());
    }

  // Get image properties
  vtkMatrix4x4* rasToIjkMatrix = reader->GetRasToIjkMatrix();
  vtkNew<vtkMatrix4x4> imageToWorldMatrix;
  vtkMatrix4x4::Invert(rasToIjkMatrix, imageToWorldMatrix.GetPointer());

  // Read segment binary labelmaps
  for (int segmentIndex = 0; segmentIndex < numberOfFrames; ++segmentIndex)
    {
    // Create segment
    vtkSmartPointer<vtkSegment> currentSegment = vtkSmartPointer<vtkSegment>::New();

    // Get metadata for current segment

    // ID
    std::string currentSegmentID = reader->GetHeaderValue(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_ID).c_str());

    // Name
    std::string currentSegmentName = reader->GetHeaderValue(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_NAME).c_str());
    currentSegment->SetName(currentSegmentName.c_str());

    // DefaultColor
    std::string defaultColorValue = reader->GetHeaderValue(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_DEFAULT_COLOR).c_str());
    double currentSegmentDefaultColor[3] = { 0.0, 0.0, 0.0 };
    GetSegmentDefaultColorFromString(currentSegmentDefaultColor, defaultColorValue);

    // Extent
    std::string extentValue = reader->GetHeaderValue(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_EXTENT).c_str());
    int currentSegmentExtent[6] = { 0, -1, 0, -1, 0, -1 };
    GetImageExtentFromString(currentSegmentExtent, extentValue);

    // Tags
    std::string tagsValue = reader->GetHeaderValue(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_TAGS).c_str());
    SetSegmentTagsFromString(currentSegment, tagsValue);

    // Create binary labelmap volume
    vtkSmartPointer<vtkOrientedImageData> currentBinaryLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();

    // Copy with clipping to specified extent
    if (currentSegmentExtent[0] <= currentSegmentExtent[1]
      && currentSegmentExtent[2] <= currentSegmentExtent[3]
      && currentSegmentExtent[4] <= currentSegmentExtent[5])
      {
      // non-empty segment
      extractComponents->SetComponents(segmentIndex);
      padder->SetOutputWholeExtent(currentSegmentExtent);
      padder->Update();
      currentBinaryLabelmap->DeepCopy(padder->GetOutput());
      }
    else
      {
      // empty segment
      currentBinaryLabelmap->SetExtent(currentSegmentExtent);
      currentBinaryLabelmap->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
      }
    currentBinaryLabelmap->SetImageToWorldMatrix(imageToWorldMatrix.GetPointer());

    // Set loaded binary labelmap to segment
    currentSegment->AddRepresentation(vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName(), currentBinaryLabelmap);

    // Add segment to segmentation
    segmentation->AddSegment(currentSegment, currentSegmentID);
    }

  segmentationNode->EndModify(segmentationNodeWasModified);

  // Create contained representations now that all the data is loaded
  this->CreateRepresentationsBySerializedNames(segmentation, containedRepresentationNames);

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::ReadPolyDataRepresentation(vtkMRMLSegmentationNode* segmentationNode, std::string path)
{
  if (!vtksys::SystemTools::FileExists(path.c_str()))
    {
    vtkErrorMacro("ReadPolyDataRepresentation: Input file " << path << " does not exist!");
    return 0;
    }

  // Set up output segmentation
  if (!segmentationNode || segmentationNode->GetSegmentation()->GetNumberOfSegments() > 0)
    {
    vtkErrorMacro("ReadPolyDataRepresentation: Output segmentation must exist and must be empty!");
    return 0;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();

  // Get display node to load displayed color and opacity
  segmentationNode->CreateDefaultDisplayNodes();

  // Add all files to storage node (multiblock dataset writes segments to individual files in a separate folder)
  this->AddPolyDataFileNames(path, segmentation);

  // Read multiblock dataset from disk
  vtkSmartPointer<vtkXMLMultiBlockDataReader> reader = vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
  reader->SetFileName(path.c_str());
  reader->Update();
  vtkMultiBlockDataSet* multiBlockDataset = vtkMultiBlockDataSet::SafeDownCast(reader->GetOutput());
  if (!multiBlockDataset || multiBlockDataset->GetNumberOfBlocks()==0)
    {
    vtkErrorMacro("ReadPolyDataRepresentation: Failed to read file " << path);
    return 0;
    }

  // Read segment poly datas
  std::string masterRepresentationName;
  std::string containedRepresentationNames;
  std::string conversionParameters;
  for (unsigned int blockIndex = 0;
       blockIndex<multiBlockDataset->GetNumberOfBlocks();
       ++blockIndex)
    {
    // Get poly data representation
    vtkPolyData* currentPolyData = vtkPolyData::SafeDownCast(multiBlockDataset->GetBlock(blockIndex));

    // Set master representation if it has not been set yet
    // (there is no global place to store it, but every segment field data contains a copy of it)
    if (masterRepresentationName.empty())
      {
      vtkStringArray* masterRepresentationArray = vtkStringArray::SafeDownCast(
        currentPolyData->GetFieldData()->GetAbstractArray(GetSegmentationMetaDataKey(KEY_SEGMENTATION_MASTER_REPRESENTATION).c_str()));
      if (!masterRepresentationArray)
        {
        vtkErrorMacro("ReadPolyDataRepresentation: Unable to find master representation for segmentation in file " << path);
        return 0;
        }
      masterRepresentationName = masterRepresentationArray->GetValue(0);
      segmentation->SetMasterRepresentationName(masterRepresentationName.c_str());
      }
    // Read conversion parameters (stored in each segment file, but need to set only once)
    if ( conversionParameters.empty()
      && currentPolyData->GetFieldData()->GetAbstractArray(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONVERSION_PARAMETERS).c_str()) )
      {
      vtkStringArray* conversionParametersArray = vtkStringArray::SafeDownCast(
        currentPolyData->GetFieldData()->GetAbstractArray(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONVERSION_PARAMETERS).c_str()) );
      conversionParameters = conversionParametersArray->GetValue(0);
      segmentation->DeserializeConversionParameters(conversionParameters);
      }
    // Read contained representation names
    if ( containedRepresentationNames.empty()
      && currentPolyData->GetFieldData()->GetAbstractArray(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONTAINED_REPRESENTATION_NAMES).c_str()) )
      {
      containedRepresentationNames = vtkStringArray::SafeDownCast(
        currentPolyData->GetFieldData()->GetAbstractArray(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONTAINED_REPRESENTATION_NAMES).c_str()) )->GetValue(0);
      }

    // Create segment
    vtkSmartPointer<vtkSegment> currentSegment = vtkSmartPointer<vtkSegment>::New();
    currentSegment->AddRepresentation(segmentation->GetMasterRepresentationName(), currentPolyData);

    // Set segment properties

    std::string currentSegmentID;
    vtkStringArray* idArray = vtkStringArray::SafeDownCast(
      currentPolyData->GetFieldData()->GetAbstractArray(GetSegmentMetaDataKey(SINGLE_SEGMENT_INDEX, KEY_SEGMENT_ID).c_str()) );
    if (idArray && idArray->GetNumberOfValues()>0)
      {
      currentSegmentID = idArray->GetValue(0);
      }
    else
      {
      vtkWarningMacro("ReadPolyDataRepresentation: segment ID property not found when reading segment " << blockIndex << " from file " << path);
      }

    std::string currentSegmentName;
    vtkStringArray* nameArray = vtkStringArray::SafeDownCast(
      currentPolyData->GetFieldData()->GetAbstractArray(GetSegmentMetaDataKey(SINGLE_SEGMENT_INDEX, KEY_SEGMENT_NAME).c_str()) );
    if (nameArray && nameArray->GetNumberOfValues()>0)
      {
      currentSegmentName = nameArray->GetValue(0);
      }
    else
      {
      vtkWarningMacro("ReadPolyDataRepresentation: segment Name property not found when reading segment " << blockIndex << " from file " << path);
      std::stringstream ssCurrentSegmentName;
      ssCurrentSegmentName << "Segment " << blockIndex;
      currentSegmentName = ssCurrentSegmentName.str();
      }
    currentSegment->SetName(currentSegmentName.c_str());

    double defaultColor[3]={1.0, 0.0, 0.0};
    vtkDoubleArray* defaultColorArray = vtkDoubleArray::SafeDownCast(
      currentPolyData->GetFieldData()->GetArray(GetSegmentMetaDataKey(SINGLE_SEGMENT_INDEX, KEY_SEGMENT_DEFAULT_COLOR).c_str()) );
    if (defaultColorArray && defaultColorArray->GetNumberOfTuples() > 0 && defaultColorArray->GetNumberOfComponents() == 3)
      {
      defaultColorArray->GetTuple(0, defaultColor);
      }
    else
      {
      vtkWarningMacro("ReadPolyDataRepresentation: segment DefaultColor property not found when reading segment " << blockIndex << " from file " << path);
      }
    currentSegment->SetDefaultColor(defaultColor);

    // Tags
    vtkStringArray* tagsArray = vtkStringArray::SafeDownCast(
      currentPolyData->GetFieldData()->GetAbstractArray(GetSegmentMetaDataKey(SINGLE_SEGMENT_INDEX, KEY_SEGMENT_TAGS).c_str()) );
    if (tagsArray)
      {
      std::string tags(tagsArray->GetValue(0).c_str());
      std::string separatorCharacter("|");
      size_t separatorPosition = tags.find(separatorCharacter);
      while (separatorPosition != std::string::npos)
        {
        std::string mapPairStr = tags.substr(0, separatorPosition);
        size_t colonPosition = mapPairStr.find(":");
        if (colonPosition == std::string::npos)
          {
          continue;
          }
        currentSegment->SetTag(mapPairStr.substr(0, colonPosition), mapPairStr.substr(colonPosition+1));
        tags = tags.substr(separatorPosition+1);
        separatorPosition = tags.find(separatorCharacter);
        }
      }

    // Add segment to segmentation
    segmentation->AddSegment(currentSegment, currentSegmentID);
    }

  // Create contained representations now that all the data is loaded
  this->CreateRepresentationsBySerializedNames(segmentation, containedRepresentationNames);

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
    {
    vtkErrorMacro("vtkMRMLModelNode: File name not specified");
    return 0;
    }

  vtkMRMLSegmentationNode *segmentationNode = vtkMRMLSegmentationNode::SafeDownCast(refNode);
  if (segmentationNode == NULL)
    {
    vtkErrorMacro("Segmentation node expected. Unable to write node to file.");
    return 0;
    }

  // Write only master representation
  const char* masterRepresentation = segmentationNode->GetSegmentation()->GetMasterRepresentationName();
  if (!strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
    // Binary labelmap -> 4D NRRD volume
    return this->WriteBinaryLabelmapRepresentation(segmentationNode, fullName);
    }
  else if ( !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
         || !strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()) )
    {
    // Closed surface or planar contours -> MultiBlock polydata
    return this->WritePolyDataRepresentation(segmentationNode, fullName);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WriteBinaryLabelmapRepresentation(vtkMRMLSegmentationNode* segmentationNode, std::string fullName)
{
  if (!segmentationNode || segmentationNode->GetSegmentation()->GetNumberOfSegments() == 0)
    {
    vtkErrorMacro("WriteBinaryLabelmapRepresentation: Invalid segmentation to write to disk");
    return 0;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();

  // Get and check master representation
  const char* masterRepresentation = segmentation->GetMasterRepresentationName();
  if (!masterRepresentation || strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationBinaryLabelmapRepresentationName()))
    {
    vtkErrorMacro("WriteBinaryLabelmapRepresentation: Invalid master representation to write as image data");
    return 0;
    }

  // Determine merged labelmap dimensions and properties
  std::string commonGeometryString = segmentation->DetermineCommonLabelmapGeometry(vtkSegmentation::EXTENT_UNION_OF_EFFECTIVE_SEGMENTS);
  vtkSmartPointer<vtkOrientedImageData> commonGeometryImage = vtkSmartPointer<vtkOrientedImageData>::New();
  vtkSegmentationConverter::DeserializeImageGeometry(commonGeometryString, commonGeometryImage, true, VTK_UNSIGNED_CHAR, 1);
  int commonGeometryExtent[6] = { 0, -1, 0, -1, 0, -1 };
  commonGeometryImage->GetExtent(commonGeometryExtent);
  if (commonGeometryExtent[0] > commonGeometryExtent[1]
    || commonGeometryExtent[2] > commonGeometryExtent[3]
    || commonGeometryExtent[4] > commonGeometryExtent[5])
    {
    // common image is empty, which cannot be written to image file
    // change it to a very small image instead
    commonGeometryExtent[0] = 0;
    commonGeometryExtent[1] = 9;
    commonGeometryExtent[2] = 0;
    commonGeometryExtent[3] = 9;
    commonGeometryExtent[4] = 0;
    commonGeometryExtent[5] = 9;
    commonGeometryImage->SetExtent(commonGeometryExtent);
    commonGeometryImage->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    }
  vtkOrientedImageDataResample::FillImage(commonGeometryImage, 0);

  vtkNew<vtkMatrix4x4> rasToIjk;
  commonGeometryImage->GetWorldToImageMatrix(rasToIjk.GetPointer());
  vtkNew<vtkMatrix4x4> ijkToRas;
  vtkMatrix4x4::Invert(rasToIjk.GetPointer(), ijkToRas.GetPointer());

  vtkNew<vtkNRRDWriter> writer;
  writer->SetFileName(fullName.c_str());
  writer->SetUseCompression(this->GetUseCompression());
  writer->SetIJKToRASMatrix(ijkToRas.GetPointer());

  // Create metadata dictionary

  // Save extent of common geometry image
  writer->SetAttribute(GetSegmentationMetaDataKey(KEY_SEGMENTATION_EXTENT).c_str(), GetImageExtentAsString(commonGeometryImage));
  // Save master representation name
  writer->SetAttribute(GetSegmentationMetaDataKey(KEY_SEGMENTATION_MASTER_REPRESENTATION).c_str(), masterRepresentation);
  // Save conversion parameters
  std::string conversionParameters = segmentation->SerializeAllConversionParameters();
  writer->SetAttribute(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONVERSION_PARAMETERS).c_str(), conversionParameters);
  // Save created representation names so that they are re-created when loading
  std::string containedRepresentationNames = this->SerializeContainedRepresentationNames(segmentation);
  writer->SetAttribute(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONTAINED_REPRESENTATION_NAMES).c_str(), containedRepresentationNames);

  vtkNew<vtkImageAppendComponents> appender;

  // Dimensions of the output 4D NRRD file: (i, j, k, segment)
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
  unsigned int segmentIndex = 0;
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt, ++segmentIndex)
    {
    std::string currentSegmentID = segmentIt->first;
    vtkSegment* currentSegment = segmentIt->second.GetPointer();

    // Get master representation from segment
    vtkSmartPointer<vtkOrientedImageData> currentBinaryLabelmap = vtkOrientedImageData::SafeDownCast(currentSegment->GetRepresentation(masterRepresentation));
    if (!currentBinaryLabelmap)
      {
      vtkErrorMacro("WriteBinaryLabelmapRepresentation: Failed to retrieve master representation from segment " << currentSegmentID);
      continue;
      }

    int currentBinaryLabelmapExtent[6] = { 0, -1, 0, -1, 0, -1 };
    currentBinaryLabelmap->GetExtent(currentBinaryLabelmapExtent);
    if (currentBinaryLabelmapExtent[0] <= currentBinaryLabelmapExtent[1]
      && currentBinaryLabelmapExtent[2] <= currentBinaryLabelmapExtent[3]
      && currentBinaryLabelmapExtent[4] <= currentBinaryLabelmapExtent[5])
      {
      // There is a valid labelmap

      // Get transformed extents of the segment in the common labelmap geometry
      vtkNew<vtkTransform> currentBinaryLabelmapToCommonGeometryImageTransform;
      vtkOrientedImageDataResample::GetTransformBetweenOrientedImages(currentBinaryLabelmap, commonGeometryImage, currentBinaryLabelmapToCommonGeometryImageTransform.GetPointer());
      int currentBinaryLabelmapExtentInCommonGeometryImageFrame[6] = { 0, -1, 0, -1, 0, -1 };
      vtkOrientedImageDataResample::TransformExtent(currentBinaryLabelmapExtent, currentBinaryLabelmapToCommonGeometryImageTransform.GetPointer(), currentBinaryLabelmapExtentInCommonGeometryImageFrame);
      for (int i = 0; i < 3; i++)
        {
        currentBinaryLabelmapExtent[i * 2] = std::max(currentBinaryLabelmapExtentInCommonGeometryImageFrame[i * 2], commonGeometryExtent[i * 2]);
        currentBinaryLabelmapExtent[i * 2 + 1] = std::min(currentBinaryLabelmapExtentInCommonGeometryImageFrame[i * 2 + 1], commonGeometryExtent[i * 2 + 1]);
        }
      // TODO: maybe calculate effective extent to make sure the data is as compact as possible? (saving may be a good time to make segments more compact)

      // Pad/resample current binary labelmap representation to common geometry
      vtkSmartPointer<vtkOrientedImageData> resampledCurrentBinaryLabelmap = vtkSmartPointer<vtkOrientedImageData>::New();
      bool success = vtkOrientedImageDataResample::ResampleOrientedImageToReferenceOrientedImage(
        currentBinaryLabelmap, commonGeometryImage, resampledCurrentBinaryLabelmap);
      if (!success)
        {
        vtkWarningMacro("WriteBinaryLabelmapRepresentation: Segment " << currentSegmentID << " cannot be resampled to common geometry!");
        continue;
        }

      // currentBinaryLabelmap smart pointer will keep the temporary labelmap valid until it is needed
      currentBinaryLabelmap = resampledCurrentBinaryLabelmap;
      if (currentBinaryLabelmap->GetScalarType() != VTK_UNSIGNED_CHAR)
        {
        vtkNew<vtkImageCast> castFilter;
        castFilter->SetInputData(resampledCurrentBinaryLabelmap);
        castFilter->SetOutputScalarType(VTK_UNSIGNED_CHAR);
        castFilter->Update();
        currentBinaryLabelmap->ShallowCopy(castFilter->GetOutput());
        }
      }
    else
      {
      // empty segment, use the commonGeometryImage (filled with 0)
      currentBinaryLabelmap = commonGeometryImage;
      }

    // Set metadata for current segment
    writer->SetAttribute(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_ID).c_str(), currentSegmentID);
    writer->SetAttribute(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_NAME).c_str(), currentSegment->GetName());
    writer->SetAttribute(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_DEFAULT_COLOR).c_str(), GetSegmentDefaultColorAsString(currentSegment));
    writer->SetAttribute(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_EXTENT).c_str(), GetImageExtentAsString(currentBinaryLabelmapExtent));
    writer->SetAttribute(GetSegmentMetaDataKey(segmentIndex, KEY_SEGMENT_TAGS).c_str(), GetSegmentTagsAsString(currentSegment));

    appender->AddInputData(currentBinaryLabelmap);
    } // For each segment


  appender->Update();

  writer->SetInputConnection(appender->GetOutputPort());
  writer->Write();
  int writeFlag = 1;
  if (writer->GetWriteError())
    {
    vtkErrorMacro("ERROR writing NRRD file " << (writer->GetFileName() == NULL ? "null" : writer->GetFileName()));
    writeFlag = 0;
    }

  return writeFlag;
}

//----------------------------------------------------------------------------
int vtkMRMLSegmentationStorageNode::WritePolyDataRepresentation(vtkMRMLSegmentationNode* segmentationNode, std::string path)
{
  if (!segmentationNode || segmentationNode->GetSegmentation()->GetNumberOfSegments() == 0)
    {
    vtkErrorMacro("WritePolyDataRepresentation: Invalid segmentation to write to disk");
    return 0;
    }
  vtkSegmentation* segmentation = segmentationNode->GetSegmentation();

  // Get and check master representation
  const char* masterRepresentation = segmentation->GetMasterRepresentationName();
  if ( !masterRepresentation
    || ( strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationClosedSurfaceRepresentationName())
      && strcmp(masterRepresentation, vtkSegmentationConverter::GetSegmentationPlanarContourRepresentationName()) ) )
    {
    vtkErrorMacro("WritePolyDataRepresentation: Invalid master representation to write as poly data");
    return 0;
    }

  // Initialize dataset to write
  vtkSmartPointer<vtkMultiBlockDataSet> multiBlockDataset = vtkSmartPointer<vtkMultiBlockDataSet>::New();
  multiBlockDataset->SetNumberOfBlocks(segmentation->GetNumberOfSegments());

  // Add segment poly datas to dataset
  vtkSegmentation::SegmentMap segmentMap = segmentation->GetSegments();
  unsigned int segmentIndex = 0;
  for (vtkSegmentation::SegmentMap::iterator segmentIt = segmentMap.begin(); segmentIt != segmentMap.end(); ++segmentIt, ++segmentIndex)
    {
    std::string currentSegmentID = segmentIt->first;
    vtkSegment* currentSegment = segmentIt->second.GetPointer();

    // Get master representation from segment
    vtkPolyData* currentPolyData = vtkPolyData::SafeDownCast(currentSegment->GetRepresentation(masterRepresentation));
    if (!currentPolyData)
      {
      vtkErrorMacro("WritePolyDataRepresentation: Failed to retrieve master representation from segment " << currentSegmentID);
      continue;
      }
    // Make temporary duplicate of the poly data so that adding the metadata does not cause invalidating the other
    // representations (which is done when the master representation is modified)
    vtkSmartPointer<vtkPolyData> currentPolyDataCopy = vtkSmartPointer<vtkPolyData>::New();
    currentPolyDataCopy->ShallowCopy(currentPolyData);

    // Set metadata for current segment

    // MasterRepresentation
    vtkSmartPointer<vtkStringArray> masterRepresentationArray = vtkSmartPointer<vtkStringArray>::New();
    masterRepresentationArray->SetNumberOfValues(1);
    masterRepresentationArray->SetValue(0,masterRepresentation);
    masterRepresentationArray->SetName(GetSegmentationMetaDataKey(KEY_SEGMENTATION_MASTER_REPRESENTATION).c_str());
    currentPolyDataCopy->GetFieldData()->AddArray(masterRepresentationArray);

    // ID
    vtkSmartPointer<vtkStringArray> idArray = vtkSmartPointer<vtkStringArray>::New();
    idArray->SetNumberOfValues(1);
    idArray->SetValue(0,currentSegmentID.c_str());
    idArray->SetName(GetSegmentMetaDataKey(SINGLE_SEGMENT_INDEX, KEY_SEGMENT_ID).c_str());
    currentPolyDataCopy->GetFieldData()->AddArray(idArray);

    // Name
    vtkSmartPointer<vtkStringArray> nameArray = vtkSmartPointer<vtkStringArray>::New();
    nameArray->SetNumberOfValues(1);
    nameArray->SetValue(0,currentSegment->GetName());
    nameArray->SetName(GetSegmentMetaDataKey(SINGLE_SEGMENT_INDEX, KEY_SEGMENT_NAME).c_str());
    currentPolyDataCopy->GetFieldData()->AddArray(nameArray);

    // DefaultColor
    vtkSmartPointer<vtkDoubleArray> defaultColorArray = vtkSmartPointer<vtkDoubleArray>::New();
    defaultColorArray->SetNumberOfComponents(3);
    defaultColorArray->SetNumberOfTuples(1);
    defaultColorArray->SetTuple(0, currentSegment->GetDefaultColor());
    defaultColorArray->SetName(GetSegmentMetaDataKey(SINGLE_SEGMENT_INDEX, KEY_SEGMENT_DEFAULT_COLOR).c_str());
    currentPolyDataCopy->GetFieldData()->AddArray(defaultColorArray);

    // Tags
    std::map<std::string,std::string> tags;
    currentSegment->GetTags(tags);
    std::stringstream ssTags;
    std::map<std::string,std::string>::iterator tagIt;
    for (tagIt=tags.begin(); tagIt!=tags.end(); ++tagIt)
      {
      ssTags << tagIt->first << ":" << tagIt->second << "|";
      }
    vtkSmartPointer<vtkStringArray> tagsArray = vtkSmartPointer<vtkStringArray>::New();
    tagsArray->SetNumberOfValues(1);
    tagsArray->SetValue(0,ssTags.str().c_str());
    tagsArray->SetName(GetSegmentMetaDataKey(SINGLE_SEGMENT_INDEX, KEY_SEGMENT_TAGS).c_str());
    currentPolyDataCopy->GetFieldData()->AddArray(tagsArray);

    // Save conversion parameters as metadata (save in each segment file)
    std::string conversionParameters = segmentation->SerializeAllConversionParameters();
    vtkSmartPointer<vtkStringArray> conversionParametersArray = vtkSmartPointer<vtkStringArray>::New();
    conversionParametersArray->SetNumberOfValues(1);
    conversionParametersArray->SetValue(0,conversionParameters);
    conversionParametersArray->SetName(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONVERSION_PARAMETERS).c_str());
    currentPolyDataCopy->GetFieldData()->AddArray(conversionParametersArray);

    // Save contained representation names as metadata (save in each segment file)
    std::string containedRepresentationNames = this->SerializeContainedRepresentationNames(segmentation);
    vtkSmartPointer<vtkStringArray> containedRepresentationNamesArray = vtkSmartPointer<vtkStringArray>::New();
    containedRepresentationNamesArray->SetNumberOfValues(1);
    containedRepresentationNamesArray->SetValue(0,containedRepresentationNames);
    containedRepresentationNamesArray->SetName(GetSegmentationMetaDataKey(KEY_SEGMENTATION_CONTAINED_REPRESENTATION_NAMES).c_str());
    currentPolyDataCopy->GetFieldData()->AddArray(containedRepresentationNamesArray);

    // Set segment poly data to dataset
    multiBlockDataset->SetBlock(segmentIndex, currentPolyDataCopy);
    }

  // Write multiblock dataset to disk
  vtkSmartPointer<vtkXMLMultiBlockDataWriter> writer = vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
  writer->SetInputData(multiBlockDataset);
  writer->SetFileName(path.c_str());
  if (this->UseCompression)
    {
    writer->SetDataModeToBinary();
    writer->SetCompressorTypeToZLib();
    }
  else
    {
    writer->SetDataModeToAscii();
    writer->SetCompressorTypeToNone();
    }
  writer->Write();

  // Add all files to storage node (multiblock dataset writes segments to individual files in a separate folder)
  this->AddPolyDataFileNames(path, segmentation);

  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::AddPolyDataFileNames(std::string path, vtkSegmentation* segmentation)
{
  if (!segmentation)
    {
    vtkErrorMacro("AddPolyDataFileNames: Invalid segmentation!");
    return;
    }

  this->AddFileName(path.c_str());

  std::string fileNameWithoutExtension = vtksys::SystemTools::GetFilenameWithoutLastExtension(path);
  std::string parentDirectory = vtksys::SystemTools::GetParentDirectory(path);
  std::string multiBlockDirectory = parentDirectory + "/" + fileNameWithoutExtension;
  for (int segmentIndex = 0; segmentIndex < segmentation->GetNumberOfSegments(); ++segmentIndex)
    {
    std::stringstream ssSegmentFilePath;
    ssSegmentFilePath << multiBlockDirectory << "/" << fileNameWithoutExtension << "_" << segmentIndex << ".vtp";
    std::string segmentFilePath = ssSegmentFilePath.str();
    this->AddFileName(segmentFilePath.c_str());
    }
}

//----------------------------------------------------------------------------
std::string vtkMRMLSegmentationStorageNode::SerializeContainedRepresentationNames(vtkSegmentation* segmentation)
{
  if (!segmentation || segmentation->GetNumberOfSegments() == 0)
    {
    vtkErrorMacro("SerializeContainedRepresentationNames: Invalid segmentation!");
    return "";
    }

  std::stringstream ssRepresentationNames;
  std::vector<std::string> containedRepresentationNames;
  segmentation->GetContainedRepresentationNames(containedRepresentationNames);
  for (std::vector<std::string>::iterator reprIt = containedRepresentationNames.begin();
    reprIt != containedRepresentationNames.end(); ++reprIt)
    {
    ssRepresentationNames << (*reprIt) << SERIALIZATION_SEPARATOR;
    }

  return ssRepresentationNames.str();
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::CreateRepresentationsBySerializedNames(vtkSegmentation* segmentation, std::string representationNames)
{
  if (!segmentation || segmentation->GetNumberOfSegments() == 0 || !segmentation->GetMasterRepresentationName())
    {
    vtkErrorMacro("CreateRepresentationsBySerializedNames: Invalid segmentation!");
    return;
    }
  if (representationNames.empty())
    {
    vtkWarningMacro("CreateRepresentationsBySerializedNames: Empty representation names list, nothing to create");
    return;
    }

  std::string masterRepresentation(segmentation->GetMasterRepresentationName());
  size_t separatorPosition = representationNames.find(SERIALIZATION_SEPARATOR);
  while (separatorPosition != std::string::npos)
    {
    std::string representationName = representationNames.substr(0, separatorPosition);

    // Only create non-master representations
    if (representationName.compare(masterRepresentation))
      {
      segmentation->CreateRepresentation(representationName);
      }

    representationNames = representationNames.substr(separatorPosition+1);
    separatorPosition = representationNames.find(SERIALIZATION_SEPARATOR);
    }

}

//----------------------------------------------------------------------------
std::string vtkMRMLSegmentationStorageNode::GetSegmentMetaDataKey(int segmentIndex, const std::string& keyName)
{
  std::stringstream key;
  key << "Segment";
  if (segmentIndex != SINGLE_SEGMENT_INDEX)
    {
    key << segmentIndex;
    }
  key << "_" << keyName;
  return key.str();
}

//----------------------------------------------------------------------------
std::string vtkMRMLSegmentationStorageNode::GetSegmentationMetaDataKey(const std::string& keyName)
{
  std::stringstream key;
  key << "Segmentation" << "_" << keyName;
  return key.str();
}

//----------------------------------------------------------------------------
std::string vtkMRMLSegmentationStorageNode::GetSegmentTagsAsString(vtkSegment* segment)
{
  std::map<std::string, std::string> tags;
  if (segment)
    {
    segment->GetTags(tags);
    }
  std::stringstream ssTagsValue;
  std::map<std::string, std::string>::iterator tagIt;
  for (tagIt = tags.begin(); tagIt != tags.end(); ++tagIt)
    {
    ssTagsValue << tagIt->first << ":" << tagIt->second << "|";
    }
  std::string tagsValue = ssTagsValue.str();
  return tagsValue;
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::SetSegmentTagsFromString(vtkSegment* segment, std::string tagsValue)
{
  std::string separatorCharacter("|");
  size_t separatorPosition = tagsValue.find(separatorCharacter);
  while (separatorPosition != std::string::npos)
    {
    std::string mapPairStr = tagsValue.substr(0, separatorPosition);
    size_t colonPosition = mapPairStr.find(":");
    if (colonPosition == std::string::npos)
      {
      continue;
      }
    segment->SetTag(mapPairStr.substr(0, colonPosition), mapPairStr.substr(colonPosition + 1));
    tagsValue = tagsValue.substr(separatorPosition + 1);
    separatorPosition = tagsValue.find(separatorCharacter);
    }
}

//----------------------------------------------------------------------------
std::string vtkMRMLSegmentationStorageNode::GetImageExtentAsString(vtkOrientedImageData* image)
{
  int extent[6] = { 0, -1, 0, -1, 0, -1 };
  if (image)
    {
    image->GetExtent(extent);
    }
  return GetImageExtentAsString(extent);
}

//----------------------------------------------------------------------------
std::string vtkMRMLSegmentationStorageNode::GetImageExtentAsString(int extent[6])
{
  std::stringstream ssExtentValue;
  ssExtentValue << extent[0] << " " << extent[1] << " " << extent[2]
    << " " << extent[3] << " " << extent[4] << " " << extent[5];
  std::string extentValue = ssExtentValue.str();
  return extentValue;
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::GetImageExtentFromString(int extent[6], std::string extentValue)
{
  std::stringstream ssExtentValue(extentValue);
  extent[0] = 0;
  extent[1] = -1;
  extent[2] = 0;
  extent[3] = -1;
  extent[4] = 0;
  extent[5] = -1;
  ssExtentValue >> extent[0] >> extent[1] >> extent[2] >> extent[3] >> extent[4] >> extent[5];
}

//----------------------------------------------------------------------------
std::string vtkMRMLSegmentationStorageNode::GetSegmentDefaultColorAsString(vtkSegment* segment)
{
  std::stringstream ssDefaultColorValue;
  double defaultColor[3] = { 0.5, 0.5, 0.5 };
  if (segment)
    {
    segment->GetDefaultColor(defaultColor);
    }
  ssDefaultColorValue << defaultColor[0] << " " << defaultColor[1] << " " << defaultColor[2];
  std::string defaultColorValue = ssDefaultColorValue.str();
  return defaultColorValue;
}

//----------------------------------------------------------------------------
void vtkMRMLSegmentationStorageNode::GetSegmentDefaultColorFromString(double defaultColor[3], std::string defaultColorValue)
{
  std::stringstream ssDefaultColorValue(defaultColorValue);
  defaultColor[0] = 0.5;
  defaultColor[1] = 0.5;
  defaultColor[2] = 0.5;
  ssDefaultColorValue >> defaultColor[0] >> defaultColor[1] >> defaultColor[2];
}
