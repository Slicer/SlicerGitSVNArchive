/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Johan Andruejol, Kitware Inc.
  and was partially funded by Allen Institute.

==============================================================================*/

#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsGenericStorageNode.h"
#include "vtkMRMLMarkupsNode.h"

#include "vtkMRMLScene.h"
#include "vtkSlicerVersionConfigure.h"

#include "vtkObjectFactory.h"
#include "vtkStringArray.h"
#include <vtksys/SystemTools.hxx>
#include "vtkVariantArray.h"
#include "vtkDoubleArray.h"

// JSON includes
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/pointer.h"
#include "rapidjson/prettywriter.h"

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsGenericStorageNode);

//---------------------------------------------------------------------------
class vtkMRMLMarkupsGenericStorageNode::vtkInternal
{
public:
  vtkInternal(vtkMRMLMarkupsGenericStorageNode* external);
  ~vtkInternal();

public:
  void TranslationMapToJSON(
    vtkMRMLMarkupsGenericStorageNode::TranslationMap& map,
    rapidjson::Document* doc);

  void JSONObjectToTranslationMap(
    const rapidjson::Value& object,
    vtkMRMLMarkupsGenericStorageNode::TranslationMap& map,
    std::string baseKey = "");

  void JSONArrayToTranslationMap(
    const rapidjson::Value& object,
    vtkMRMLMarkupsGenericStorageNode::TranslationMap& map,
    std::string baseKey = "");

  void JSONNodeToTranslationMap(
    const rapidjson::Value& object,
    vtkMRMLMarkupsGenericStorageNode::TranslationMap& map,
    std::string baseKey = "");
};

//---------------------------------------------------------------------------
// vtkInternal methods
//---------------------------------------------------------------------------
vtkMRMLMarkupsGenericStorageNode::vtkInternal
::vtkInternal(vtkMRMLMarkupsGenericStorageNode * vtkNotUsed(external))
{
}

#define WriteValue(VTKType, Type)                                            \
  case VTKType:                                                              \
    {                                                                        \
    rapidjson::Pointer(key.c_str()).Set(*doc, value.To##Type());             \
    break;                                                                   \
    }

//----------------------------------------------------------------------------
void vtkMRMLMarkupsGenericStorageNode::vtkInternal::TranslationMapToJSON(
  vtkMRMLMarkupsGenericStorageNode::TranslationMap& map, rapidjson::Document* doc)
{
  typedef vtkMRMLMarkupsGenericStorageNode::TranslationMap::const_iterator IteratorType;
  for (IteratorType it = map.begin(); it != map.end(); ++it)
    {
    std::string key = it->first;
    vtkVariant value = it->second;
    if (value.IsValid())
      {
      switch (value.GetType())
        {
        WriteValue(VTK_FLOAT, Float)
        WriteValue(VTK_DOUBLE, Double)
        WriteValue(VTK_CHAR, Char)
        WriteValue(VTK_UNSIGNED_CHAR, UnsignedChar)
        WriteValue(VTK_SIGNED_CHAR, SignedChar)
        WriteValue(VTK_SHORT, Short)
        WriteValue(VTK_UNSIGNED_SHORT, UnsignedShort)
        WriteValue(VTK_INT, Int)
        WriteValue(VTK_UNSIGNED_INT, UnsignedInt)
        default:
          {
          rapidjson::Pointer(key.c_str()).Set(*doc, value.ToString().c_str());
          }
        }
      }
    else
      {
      rapidjson::Pointer(key.c_str()).Create(*doc);
      }
    }
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsGenericStorageNode::vtkInternal::JSONObjectToTranslationMap(
  const rapidjson::Value& node,
  vtkMRMLMarkupsGenericStorageNode::TranslationMap& map,
  std::string baseKey)
{
  for (rapidjson::Value::ConstMemberIterator childNode = node.MemberBegin();
    childNode != node.MemberEnd(); ++childNode)
    {
    std::stringstream key;
    key << baseKey << "/" << childNode->name.GetString();
    this->JSONNodeToTranslationMap(childNode->value, map, key.str());
    }
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsGenericStorageNode::vtkInternal::JSONArrayToTranslationMap(
  const rapidjson::Value& node,
  vtkMRMLMarkupsGenericStorageNode::TranslationMap& map,
  std::string baseKey)
{
  for (rapidjson::SizeType i = 0; i < node.Size(); ++i)
    {
    std::stringstream key;
    key << baseKey << "/" << i ;
    this->JSONNodeToTranslationMap(node[i], map, key.str());
    }
}

#define ReadValue(Type)                                                      \
  else if (node.Is##Type())                                                  \
    {                                                                        \
    map[baseKey] = node.Get##Type();                                         \
    }

//----------------------------------------------------------------------------
void vtkMRMLMarkupsGenericStorageNode::vtkInternal::JSONNodeToTranslationMap(
  const rapidjson::Value& node,
  vtkMRMLMarkupsGenericStorageNode::TranslationMap& map,
  std::string baseKey)
{
  if (node.IsArray())
    {
    this->JSONArrayToTranslationMap(node, map, baseKey);
    }
  else if (node.IsObject())
    {
    this->JSONObjectToTranslationMap(node, map, baseKey);
    }
  else
    {
    if (node.IsNull())
      {
      map[baseKey] = vtkVariant();
      }
    ReadValue(String)
    ReadValue(Double)
    ReadValue(Float)
    ReadValue(Int)
    ReadValue(Uint)
    ReadValue(Int64)
    ReadValue(Uint64)
    ReadValue(Bool)
    }
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsGenericStorageNode::vtkMRMLMarkupsGenericStorageNode()
{
  this->DefaultWriteFileExtension = "markups.json";
}

//----------------------------------------------------------------------------
vtkMRMLMarkupsGenericStorageNode::~vtkMRMLMarkupsGenericStorageNode()
{
}

//----------------------------------------------------------------------------
bool vtkMRMLMarkupsGenericStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLMarkupsNode");
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsGenericStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  if (!refNode)
    {
    vtkErrorMacro("ReadDataInternal: null reference node!");
    return 0;
    }

  std::string fullName = this->GetFullNameFromFileName();

  if (fullName.empty())
    {
    vtkErrorMacro("vtkMRMLMarkupsGenericStorageNode: File name not specified");
    return 0;
    }

  // cast the input node
  vtkMRMLMarkupsNode *markupsNode =
    vtkMRMLMarkupsNode::SafeDownCast(refNode);
  if (!markupsNode)
    {
    return 0;
    }

  // get the display node
  vtkMRMLMarkupsDisplayNode *displayNode = NULL;
  vtkMRMLDisplayNode * mrmlNode = markupsNode->GetDisplayNode();
  if (mrmlNode)
    {
    displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(mrmlNode);
    }
  if (!displayNode)
    {
    vtkWarningMacro("vtkMRMLMarkupsGenericStorageNode: no display node!");
    if (this->GetScene())
      {
      vtkWarningMacro("vtkMRMLMarkupsGenericStorageNode: adding a new display node.");
      displayNode = vtkMRMLMarkupsDisplayNode::New();
      this->GetScene()->AddNode(displayNode);
      markupsNode->SetAndObserveDisplayNodeID(displayNode->GetID());
      displayNode->Delete();
      }
    }

  rapidjson::Document* markupsRoot = new rapidjson::Document;
  FILE *file = fopen(fullName.c_str(), "r");
  if (!file)
    {
    vtkErrorMacro(
      "vtkMRMLMarkupsGenericStorageNode: Failed to load from file '" << fullName << "'");
    return 0;
    }

  char buffer[4096];
  rapidjson::FileReadStream readStream(file, buffer, sizeof(buffer));
  if (markupsRoot->ParseStream(readStream).HasParseError())
    {
    vtkErrorMacro(
      "vtkMRMLMarkupsGenericStorageNode: Failed to load from file '" << fullName << "'");
    fclose(file);
    return 0;
    }
  fclose(file);

  TranslationMap markupsMap;
  rapidjson::Value rootValue = markupsRoot->GetObject();
  this->Internal->JSONNodeToTranslationMap(rootValue, markupsMap);
  if (!this->ReadMarkupsNodeFromTranslationMap(markupsNode, markupsMap))
    {
    vtkErrorMacro("vtkMRMLMarkupsGenericStorageNode: Error while converting JSON to markup")
      return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsGenericStorageNode::ReadMarkupsNodeFromTranslationMap(
  vtkMRMLMarkupsNode* markupsNode, TranslationMap& markupsMap)
{
  if (!markupsNode)
    {
    return 0;
    }

  int success = 1;

  // Locked
  markupsNode->SetLocked(markupsMap["/Locked"].ToInt());
  // MarkupLabelFormat
  markupsNode->SetMarkupLabelFormat(markupsMap["/MarkupLabelFormat"].ToString());
  // TextList
  size_t textList_size = markupsMap["/TextList_Count"].ToInt();
  for (size_t i = 0; i < textList_size; ++i)
    {
    std::stringstream key;
    key << "/TextList/" << i;
    markupsNode->AddText(markupsMap[key.str()].ToString());
    }
  // Markups
  size_t markups_Size = markupsMap["/Markups_Count"].ToInt();
  for (size_t i = 0; i < markups_Size; ++i)
    {
    if (!markupsNode->MarkupExists(i))
      {
      Markup m;
      markupsNode->InitMarkup(&m);
      markupsNode->AddMarkup(m);
      }

    std::stringstream subKey;
    subKey << "/Markups/" << i << "/";

    success &=  this->ReadNthMarkupFromTranslationMap(
        i, subKey.str(), markupsNode, markupsMap);
    }
  return success;
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsGenericStorageNode::ReadNthMarkupFromTranslationMap(
  int n, std::string key,
  vtkMRMLMarkupsNode* markupsNode, TranslationMap& markupsMap)
{
  if (!markupsNode || !markupsNode->MarkupExists(n))
    {
    return 0;
    }

  Markup* markup = markupsNode->GetNthMarkup(n);

  // ID
  markup->ID = markupsMap[key + "ID"].ToString();
  // Label
  markup->Label = markupsMap[key + "Label"].ToString();
  // Description
  markup->Description = markupsMap[key + "Description"].ToString();
  // AssociatedNodeID
  markup->AssociatedNodeID = markupsMap[key + "AssociatedNodeID"].ToString();
  // Points
  size_t points_size = markupsMap[key + "Points_Count"].ToInt();
  for (size_t i = 0; i < points_size; ++i)
    {
    std::stringstream pointsKey;
    pointsKey << key << "Points/" << i;

    markup->points.push_back(
      vtkVector3d(
        markupsMap[pointsKey.str() + "/x"].ToDouble(),
        markupsMap[pointsKey.str() + "/y"].ToDouble(),
        markupsMap[pointsKey.str() + "/z"].ToDouble())
      );
    }
  // OrientationWXYZ
  for (int i = 0; i < 4; ++i)
    {
    std::stringstream orientationKey;
    orientationKey << key << "OrientationWXYZ/" << i;
    markup->OrientationWXYZ[i] = markupsMap[orientationKey.str()].ToDouble();
   }
  // NOTE: vtkVariant stores bool as a char, which gives weird result in JSON.
  // -> Force the bool to int.
  // Selected
  markup->Selected = markupsMap[key + "Selected"].ToInt();
  // Selected
  markup->Locked = markupsMap[key + "Locked"].ToInt() != 0;
  // Selected
  markup->Visibility = markupsMap[key + "Visibility"].ToInt();

  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsGenericStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName.empty())
    {
    vtkErrorMacro("vtkMRMLMarkupsGenericStorageNode: File name not specified");
    return 0;
    }

  // cast the input node
  vtkMRMLMarkupsNode *markupsNode = vtkMRMLMarkupsNode::SafeDownCast(refNode);
  if (!markupsNode)
    {
    vtkErrorMacro(
      "vtkMRMLMarkupsGenericStorageNode: uUnable to cast input node "
      << refNode->GetID() << " to a known markups node");
    return 0;
    }

  TranslationMap markupMap;
  if (!this->WriteMarkupsNodeToTranslationMap(markupsNode, markupMap))
    {
    vtkErrorMacro("vtkMRMLMarkupsGenericStorageNode: Error while converting markup to JSON")
    return 0;
    }

  rapidjson::Document* markupsRoot = new rapidjson::Document;
  this->Internal->TranslationMapToJSON(markupMap, markupsRoot);
  FILE *file = fopen(fullName.c_str(), "w");
  if (!file)
    {
    vtkErrorMacro(
      "vtkMRMLMarkupsGenericStorageNode: Failed to write to file '" << fullName << "'");
    return 0;
    }

  char writeBuffer[4096];
  rapidjson::FileWriteStream writeStream(file, writeBuffer, sizeof(writeBuffer));
  rapidjson::Writer<rapidjson::FileWriteStream> writer(writeStream);
  if (!markupsRoot->Accept(writer))
    {
    vtkErrorMacro(
      "vtkMRMLMarkupsGenericStorageNode: Error while writing to file '" << fullName << "'");
    fclose(file);
    return 0;
    }

  fclose(file);
  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsGenericStorageNode::WriteMarkupsNodeToTranslationMap(
  vtkMRMLMarkupsNode* markupsNode, TranslationMap& markupsMap)
{
  if (!markupsNode)
    {
    return 0;
    }

  int success = 1;

  // Locked
  markupsMap["/Locked"] = markupsNode->GetLocked();
  // MarkupLabelFormat
  markupsMap["/MarkupLabelFormat"] = markupsNode->GetMarkupLabelFormat().c_str();
  // TextList
  markupsMap["/TextList_Count"] = markupsNode->GetNumberOfTexts();
  if (markupsNode->GetNumberOfTexts() == 0)
    {
    // Empty variant to signal that the list is empty
    markupsMap["/TextList/0"] = vtkVariant();
    }
  else
    {
    for (int i = 0; i < markupsNode->GetNumberOfTexts(); ++i)
      {
      std::stringstream key;
      key << "/TextList/" << i;
      markupsMap[key.str().c_str()] = markupsNode->GetText(i);
      }
    }
  // Markups
  markupsMap["/Markups_Count"] = markupsNode->GetNumberOfMarkups();
  if (markupsNode->GetNumberOfMarkups() == 0)
    {
    // Empty variant to signal that the list is empty
    markupsMap["/Markups/0"] = vtkVariant();
    }
  else
    {
    for (int i = 0; i < markupsNode->GetNumberOfMarkups(); ++i)
      {
      std::stringstream subKey;
      subKey << "/Markups/" << i << "/";

      success &= this->WriteNthMarkupToTranslationMap(
        i, subKey.str(), markupsNode, markupsMap);
      }
    }

  return success;
}

//----------------------------------------------------------------------------
int vtkMRMLMarkupsGenericStorageNode::WriteNthMarkupToTranslationMap(
  int n, std::string key,
  vtkMRMLMarkupsNode* markupsNode, TranslationMap& markupsMap)
{
  if (!markupsNode || !markupsNode->MarkupExists(n))
    {
    return 0;
    }

  Markup* markup = markupsNode->GetNthMarkup(n);

  // ID
  markupsMap[key + "ID"] = markup->ID.c_str();
  // Label
  markupsMap[key + "Label"] = markup->Label.c_str();
  // Description
  markupsMap[key + "Description"] = markup->Description.c_str();
  // AssociatedNodeID
  markupsMap[key + "AssociatedNodeID"] = markup->AssociatedNodeID.c_str();
  // Points
  markupsMap[key + "Points_Count"] = markup->points.size();
  if (markup->points.size() == 0)
    {
    // Empty variant to signal that the list is empty
    markupsMap[key + "Points/0"] = vtkVariant();
    }
  else
    {
    for (size_t i = 0; i < markup->points.size(); ++i)
      {
      std::stringstream pointsKey;
      pointsKey << key << "Points/" << i << "/x";
      markupsMap[pointsKey.str()] = markup->points[i][0];
      pointsKey.str("");
      pointsKey << key << "Points/" << i << "/y";
      markupsMap[pointsKey.str()] = markup->points[i][1];
      pointsKey.str("");
      pointsKey << key << "Points/" << i << "/z";
      markupsMap[pointsKey.str()] = markup->points[i][2];
      }
    }

  // OrientationWXYZ
  for (int i = 0; i < 4; ++i)
    {
    std::stringstream orientationKey;
    orientationKey << key << "OrientationWXYZ/" << i;
    markupsMap[orientationKey.str()] = markup->OrientationWXYZ[i];
    }

  // NOTE: vtkVariant stores bool as a char, which gives weird result in JSON.
  // -> Force the bool to int.
  // Selected
  markupsMap[key + "Selected"] = static_cast<unsigned int>(markup->Selected);
  // Selected
  markupsMap[key + "Locked"] = static_cast<unsigned int>(markup->Locked);
  // Selected
  markupsMap[key + "Visibility"] = static_cast<unsigned int>(markup->Visibility);

  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsGenericStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Markups (.markups.json)");
  this->SupportedReadFileTypes->InsertNextValue("Markups (.json)");
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsGenericStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Markups (.markups.json)");
  this->SupportedWriteFileTypes->InsertNextValue("Markups (.json)");
}
