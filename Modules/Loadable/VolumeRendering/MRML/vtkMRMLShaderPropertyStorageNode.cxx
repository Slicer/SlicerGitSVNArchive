/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Simon Drouin, Brigham and Women's
  Hospital, Boston, MA.

==============================================================================*/

// VolumeRendering includes
#include "vtkMRMLShaderPropertyNode.h"
#include "vtkMRMLShaderPropertyStorageNode.h"

// MRML includes
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkMatrix3x3.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkShaderProperty.h>
#include <vtkStringArray.h>
#include <vtkUniforms.h>

// Json includes
#include <json/json.h>

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLShaderPropertyStorageNode);

//----------------------------------------------------------------------------
vtkMRMLShaderPropertyStorageNode::vtkMRMLShaderPropertyStorageNode()
{
  this->DefaultWriteFileExtension = "sp";
}

//----------------------------------------------------------------------------
vtkMRMLShaderPropertyStorageNode::~vtkMRMLShaderPropertyStorageNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLShaderPropertyStorageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkMRMLShaderPropertyStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLShaderPropertyNode");
}

namespace
{

//----------------------------------------------------------------------------
void DecodeValue(Json::Value & val, int & dec)
{
  dec = val.asInt();
}

//----------------------------------------------------------------------------
void DecodeValue(Json::Value & val, float & dec)
{
  dec = val.asFloat();
}

//----------------------------------------------------------------------------
template< typename scalarT >
void DecodeTuple(Json::Value & val, std::vector<scalarT> & tuple)
{
  tuple.resize(val.size());
  for(unsigned i = 0; i < val.size(); ++i)
    {
    scalarT vi;
    DecodeValue(val[i], vi);
    tuple[i] = vi;
    }
}

//----------------------------------------------------------------------------
template< typename scalarT >
void DecodeMatrix(Json::Value & val, std::vector<scalarT> & mat)
{
  int nbRows = val.size();
  for(int i = 0; i < nbRows; ++i)
    {
    Json::Value & row = val[i];
    int nbCols = row.size();
    for(int j = 0; j < nbCols; ++j)
      {
      scalarT vi;
      DecodeValue(row[i], vi);
      mat.push_back(vi);
      }
    }
}

//----------------------------------------------------------------------------
template< typename scalarT >
void DecodeValue(Json::Value & val, vtkUniforms::TupleType tupleType, int nbComponents, int nbTuples, std::vector<scalarT> & dec)
{
  if(nbTuples == 1)
    {
    if(tupleType == vtkUniforms::TupleTypeScalar)
      {
      scalarT v;
      DecodeValue(val["value"], v);
      dec.resize(1,v);
      }
    else if(tupleType == vtkUniforms::TupleTypeVector)
      {
      DecodeTuple(val["value"], dec);
      }
    else if(tupleType == vtkUniforms::TupleTypeMatrix)
      {
      DecodeMatrix(val["value"], dec);
      }
    }
  else
    {
    if(tupleType == vtkUniforms::TupleTypeScalar)
      {
      DecodeTuple(val["value"], dec);
      }
    else if(tupleType == vtkUniforms::TupleTypeVector)
      {
      for(int i = 0; i < val.size(); ++i)
        {
        std::vector<scalarT> tup;
        DecodeTuple(val[i], tup);
        dec.insert(dec.end(), tup.begin(), tup.end());
        }
      }
    else if(tupleType == vtkUniforms::TupleTypeMatrix)
      {
      for(int i = 0; i < val.size(); ++i)
        {
        std::vector<scalarT> mat;
        DecodeMatrix(val[i], mat);
        dec.insert(dec.end(), mat.begin(), mat.end());
        }
      }
    }
}

} // end of anonymous namespace

//----------------------------------------------------------------------------
void ReadUniforms(Json::Value & juni, vtkUniforms * uni)
{
  for(int i = 0; i < juni.size(); ++i)
    {
    Json::Value jvar = juni[i];
    std::string uName = jvar["name"].asString();
    int scalarType = vtkUniforms::StringToScalarType(jvar["scalarType"].asString());
    vtkUniforms::TupleType tupleType =  vtkUniforms::StringToTupleType(jvar["tupleType"].asString());
    int nbComponents = jvar["numberOfComponents"].asInt();
    int nbTuples = jvar["numberOfTuples"].asInt();

    if(scalarType == VTK_INT)
      {
      std::vector<int> decValues;
      DecodeValue(jvar, tupleType, nbComponents, nbTuples, decValues);
      uni->SetUniform(uName.c_str(), tupleType, nbComponents, decValues);
      }
    else if(scalarType == VTK_FLOAT)
      {
      std::vector<float> decValues;
      DecodeValue(jvar, tupleType, nbComponents, nbTuples, decValues);
      uni->SetUniform(uName.c_str(), tupleType, nbComponents, decValues);
      }
    }
}

//----------------------------------------------------------------------------
int vtkMRMLShaderPropertyStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLShaderPropertyNode *spNode =
    vtkMRMLShaderPropertyNode::SafeDownCast(refNode);

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName.empty())
    {
    vtkErrorMacro("ReadData: File name not specified");
    return 0;
    }

  std::ifstream ifs;
  ifs.open(fullName.c_str(), ios::in);
  if (!ifs)
    {
    vtkErrorMacro("Cannot open shader property file: " << fullName);
    return 0;
    }

  // Read json file
  Json::Reader reader;
  Json::Value root;
  reader.parse(ifs, root, false);
  ifs.close();

  // Read vertex shader uniform variables
  Json::Value vertexUniforms = root["VertexUniforms"];
  ReadUniforms(vertexUniforms, spNode->GetVertexUniforms());

  // Read fragment shader uniform variables
  Json::Value fragmentUniforms = root["FragmentUniforms"];
  ReadUniforms(fragmentUniforms, spNode->GetFragmentUniforms());

  // Read shader replacements
  vtkShaderProperty * sp = spNode->GetShaderProperty();
  Json::Value shaderReplacement = root["ShaderReplacements"];
  for(int i = 0; i < shaderReplacement.size(); ++i)
    {
    Json::Value spv = shaderReplacement[i];
    std::string shaderType = spv["ShaderType"].asString();
    std::string replacementSpec = spv["ReplacementSpec"].asString();
    bool replaceFirst = spv["replaceFirst"].asBool();
    std::string replacementValue = spv["ReplacementValue"].asString();
    bool replaceAll = spv["replaceAll"].asBool();
    if(shaderType == std::string("Vertex"))
      {
      sp->AddVertexShaderReplacement(replacementSpec, replaceFirst, replacementValue, replaceAll);
      }
    else if(shaderType == std::string("Fragment"))
      {
      sp->AddFragmentShaderReplacement(replacementSpec, replaceFirst, replacementValue, replaceAll);
      }
    }

  return 1;
}

namespace
{

//----------------------------------------------------------------------------
template< typename T >
Json::Value EncodeMatrix(const std::vector<T> & mat, int n)
{
  Json::Value val;
  for(int i = 0; i < n; ++i)
    {
    Json::Value row;
    for(int j = 0; j < n; ++j)
      {
      row[j] = mat[i*n+j];
      }
    val[i] = row;
    }
  return val;
}

//----------------------------------------------------------------------------
template< typename T >
Json::Value EncodeTuple(const std::vector<T> & tuple, int n )
{
  Json::Value val;
  val.resize(n);
  for(int i = 0; i < n; ++i)
    {
    val[i] = tuple[i];
    }
  return val;
}

//----------------------------------------------------------------------------
template< typename scalarT >
void EncodeVar(int nbComponents, int nbTuples, vtkUniforms::TupleType tt, Json::Value & val, const std::vector<scalarT> & values)
{
  if(nbTuples == 1)
    {
    if(tt == vtkUniforms::TupleTypeScalar)
      {
      val["value"] = values[0];
      }
    else if(tt == vtkUniforms::TupleTypeVector)
      {
      val["value"] = EncodeTuple(values,nbComponents);
      }
    else if(tt == vtkUniforms::TupleTypeMatrix)
      {
      int matWidth = static_cast<int>(sqrt(nbComponents));
      val["value"] = EncodeMatrix(values, matWidth);
      }
    }
  else
    {
    if(tt == vtkUniforms::TupleTypeScalar)
      {
      val["value"] = EncodeTuple(values,nbTuples);
      }
    else if(tt == vtkUniforms::TupleTypeVector)
      {
      val["value"].resize(nbTuples);
      for(int i = 0; i < nbTuples; ++i)
        {
        val["value"][i] = EncodeTuple(values,nbComponents);
        }
      }
    else if(tt == vtkUniforms::TupleTypeMatrix)
    {
      int matWidth = static_cast<int>(sqrt(nbComponents));
      val["value"].resize(nbTuples);
      for(int i = 0; i < nbTuples; ++i)
        {
        val["value"][i] = EncodeMatrix(values, matWidth);
        }
      }
    }
}

//----------------------------------------------------------------------------
void WriteUniforms(vtkUniforms * vu, Json::Value & root)
{
  root.resize(vu->GetNumberOfUniforms());
  for(int i = 0; i < vu->GetNumberOfUniforms(); ++i)
    {
    std::string uName = vu->GetNthUniformName(i);
    Json::Value u;

    vtkUniforms::TupleType tupleType = vu->GetUniformTupleType(uName.c_str());
    vtkIdType nbTuples = vu->GetUniformNumberOfTuples(uName.c_str());
    vtkIdType nbComponents = vu->GetUniformNumberOfComponents(uName.c_str());
    int scalarType = vu->GetUniformScalarType(uName.c_str());

    u["name"] = uName;
    u["scalarType"] = vtkUniforms::ScalarTypeToString(scalarType);
    u["tupleType"] = vtkUniforms::TupleTypeToString(tupleType);
    u["numberOfComponents"] = nbComponents;
    u["numberOfTuples"] = nbTuples;

    if(scalarType == VTK_INT)
      {
      std::vector<int> uv;
      if(vu->GetUniform(uName.c_str(), uv))
        {
        EncodeVar(nbComponents, nbTuples, tupleType, u, uv);
        }
      }
    else if(scalarType == VTK_FLOAT)
      {
      std::vector<float> uv;
      if(vu->GetUniform(uName.c_str(), uv))
        {
        EncodeVar(nbComponents, nbTuples, tupleType, u, uv);
        }
      }
    root[i] = u;
    }
}

} // end of anonymous namespace

//----------------------------------------------------------------------------
int vtkMRMLShaderPropertyStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLShaderPropertyNode *spNode = vtkMRMLShaderPropertyNode::SafeDownCast(refNode);

  std::string fullName =  this->GetFullNameFromFileName();
  if (fullName.empty())
    {
    vtkErrorMacro("vtkMRMLShaderPropertyStorageNode: File name not specified");
    return 0;
    }

  std::ofstream ofs;
  ofs.open(fullName.c_str(), ios::out);

  if (!ofs)
    {
    vtkErrorMacro("Cannot open shader property file: " << fullName);
    return 0;
    }

  Json::Value root;

  // Collect vertex uniforms json structures
  Json::Value vertexUniforms;
  WriteUniforms(spNode->GetVertexUniforms(), vertexUniforms);
  root["VertexUniforms"] = vertexUniforms;

  // Collect fragment uniforms json structures
  Json::Value fragmentUniforms;
  WriteUniforms(spNode->GetFragmentUniforms(), fragmentUniforms);
  root["FragmentUniforms"] = fragmentUniforms;

  // Collect shader replacements in json structures
  vtkShaderProperty * sp = spNode->GetShaderProperty();
  int nbRep = sp->GetNumberOfShaderReplacements();
  Json::Value rep;
  rep.resize(nbRep);
  for(int i = 0; i < nbRep; ++i)
    {
    Json::Value spv;
    spv["ShaderType"] = sp->GetNthShaderReplacementTypeAsString(i);
    std::string replacementSpec;
    bool replaceFirst = false;
    std::string replacementValue;
    bool replaceAll = false;
    sp->GetNthShaderReplacement(i, replacementSpec, replaceFirst, replacementValue, replaceAll);
    spv["ReplacementSpec"] = replacementSpec;
    spv["replaceFirst"] = replaceFirst;
    spv["ReplacementValue"] = replacementValue;
    spv["replaceAll"] = replaceAll;
    rep[i] = spv;
    }
  root["ShaderReplacements"] = rep;

  // Write the file
  Json::StyledStreamWriter writer;
  writer.write(ofs, root);

  ofs.close();

  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLShaderPropertyStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Shader Property (.sp)");
}

//----------------------------------------------------------------------------
void vtkMRMLShaderPropertyStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Shader Property (.sp)");
  this->SupportedWriteFileTypes->InsertNextValue("Shader Property (.*)");
}
