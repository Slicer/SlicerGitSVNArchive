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

  This file was originally developed by Johan Andruejol, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// MRML includes
#include "vtkMRMLNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLUnitNode.h"

// VTK includes
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLUnitNode);

//----------------------------------------------------------------------------
vtkMRMLUnitNode::vtkMRMLUnitNode()
{
  this->HideFromEditors = 1;

  this->Prefix = 0;
  this->Suffix = 0;
  this->SecondSuffix = 0;
  this->ThirdSuffix = 0;
  this->Precision = 3;
  this->MinimumValue = VTK_DOUBLE_MIN;
  this->MaximumValue = VTK_DOUBLE_MAX;

  this->DisplayCoefficient = 1.;
  this->DisplayOffset = 0.;

  this->SetQuantity("");
  this->SetPrefix("");
  this->SetSuffix("");
  this->SetSecondSuffix("");
  this->SetThirdSuffix("");
}

//----------------------------------------------------------------------------
vtkMRMLUnitNode::~vtkMRMLUnitNode()
{
  if (this->Prefix)
    {
    delete [] this->Prefix;
    }
  if (this->Suffix)
    {
    delete [] this->Suffix;
    }
  if (this->SecondSuffix)
    {
    delete [] this->SecondSuffix;
    }
  if (this->ThirdSuffix)
    {
    delete [] this->ThirdSuffix;
    }
}

namespace
{
//----------------------------------------------------------------------------
template <typename T> std::string NumberToString(T V)
{
  std::string stringValue;
  std::stringstream strstream;
  strstream << V;
  strstream >> stringValue;
  return stringValue;
}

//----------------------------------------------------------------------------
std::string DoubleToString(double Value)
{
  return NumberToString<double>(Value);
}
}// end namespace


//----------------------------------------------------------------------------
void vtkMRMLUnitNode::WriteXML(ostream& of, int nIndent)
{
  // Write all attributes not equal to their defaults
  this->Superclass::WriteXML(of, nIndent);

  vtkIndent indent(nIndent);
  of << indent << " Quantity=\""
    << (this->GetQuantity() ? this->GetQuantity() : "") << "\"";
  of << indent << " Prefix=\"" << (this->Prefix ? this->Prefix : "") << "\"";
  of << indent << " Suffix=\"" << (this->Suffix ? this->Suffix : "") << "\"";
  of << indent << " SecondSuffix=\"" << (this->SecondSuffix ? this->SecondSuffix : "") << "\"";
  of << indent << " ThirdSuffix=\"" << (this->ThirdSuffix ? this->ThirdSuffix : "") << "\"";
  of << indent << " Precision=\"" << this->Precision << "\"";
  of << indent << " MinimumValue=\"" << this->MinimumValue << "\"";
  of << indent << " MaximumValue=\"" << this->MaximumValue << "\"";
}

namespace
{
//----------------------------------------------------------------------------
template <typename T> T StringToNumber(const char* num)
{
  std::stringstream ss;
  ss << num;
  T result;
  return ss >> result ? result : 0;
}

//----------------------------------------------------------------------------
int StringToInt(const char* str)
{
  return StringToNumber<int>(str);
}

//----------------------------------------------------------------------------
double StringToDouble(const char* str)
{
  return StringToNumber<double>(str);
}
}// end namespace

//----------------------------------------------------------------------------
void vtkMRMLUnitNode::ReadXMLAttributes(const char** atts)
{
  int disabledModify = this->StartModify();
  this->Superclass::ReadXMLAttributes(atts);

  const char* attName;
  const char* attValue;
  while (*atts != NULL)
    {
    attName = *(atts++);
    attValue = *(atts++);

    if (!strcmp(attName, "Quantity"))
      {
      this->SetQuantity(attValue);
      }
    else if (!strcmp(attName, "Prefix"))
      {
      this->SetPrefix(attValue);
      }
    else if (!strcmp(attName, "Suffix"))
      {
      this->SetSuffix(attValue);
      }
    else if (!strcmp(attName, "SecondSuffix"))
      {
      this->SetSecondSuffix(attValue);
      }
    else if (!strcmp(attName, "ThirdSuffix"))
      {
      this->SetThirdSuffix(attValue);
      }
    else if (!strcmp(attName, "Precision"))
      {
      int precision = StringToInt(attValue);
      this->SetPrecision(precision);
      }
    else if (!strcmp(attName, "MinimumValue"))
      {
      double min = StringToDouble(attValue);
      this->SetMinimumValue(min);
      }
    else if (!strcmp(attName, "MaximumValue"))
      {
      double max = StringToDouble(attValue);
      this->SetMaximumValue(max);
      }
    else if (!strcmp(attName, "DisplayCoefficient"))
      {
      double coef = StringToDouble(attValue);
      this->SetDisplayCoefficient(coef);
      }
    else if (!strcmp(attName, "DisplayOffset"))
      {
      double offset = StringToDouble(attValue);
      this->SetDisplayOffset(offset);
      }
    }
  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
const char* vtkMRMLUnitNode::GetQuantity()
{
  return this->GetAttribute("Quantity");
}

//----------------------------------------------------------------------------
void vtkMRMLUnitNode::SetQuantity(const char* name)
{
  // Quantity uses attributes internally so it can be easily access by the GUI
  // (qMRMLComboBox for example).
  this->SetAttribute("Quantity", name);
}

//----------------------------------------------------------------------------
double vtkMRMLUnitNode::GetDisplayValueFromValue(double value)
{
  return (this->DisplayCoefficient * value) + this->DisplayOffset;
}

//----------------------------------------------------------------------------
double vtkMRMLUnitNode::GetValueFromDisplayValue(double value)
{
  if (this->DisplayCoefficient)
    {
    vtkWarningMacro("Invalid display coefficient");
    return 0.;
    }
  return (value - this->DisplayOffset) / this->DisplayCoefficient;
}

//----------------------------------------------------------------------------
const char* vtkMRMLUnitNode::GetDisplayStringFromValue(double value)
{
  const double displayValue = this->GetDisplayValueFromValue(value);
  std::string displayValueString = this->GetDisplayValueStringFromDisplayValue(displayValue);
  return this->GetDisplayStringFromDisplayValueString(displayValueString.c_str());
}

//----------------------------------------------------------------------------
const char *vtkMRMLUnitNode::GetDisplayStringFromValue(double value[3])
{
  const double firstDisplayValue = this->GetDisplayValueFromValue(value[0]);
  const double secondDisplayValue = value[1];
  const double thirdDisplayValue = value[2];

  std::string firstDisplayValueString = DoubleToString(firstDisplayValue);
  std::string secondDisplayValueString = DoubleToString(secondDisplayValue);
  std::string thirdDisplayValueString = this->GetDisplayValueStringFromDisplayValue(thirdDisplayValue);

  return this->GetDisplayStringFromDisplayValueString(firstDisplayValueString.c_str(),
                                                      secondDisplayValueString.c_str(),
                                                      thirdDisplayValueString.c_str());
}

//----------------------------------------------------------------------------
const char* vtkMRMLUnitNode::GetDisplayValueStringFromDisplayValue(double displayValue)
{
  std::stringstream strstream;
  strstream.setf(ios::fixed,ios::floatfield);
  strstream.precision(this->Precision);
  strstream << displayValue;
  strstream >> this->LastValueString;
  return this->LastValueString.c_str();
}

//----------------------------------------------------------------------------
const char* vtkMRMLUnitNode::GetDisplayStringFromDisplayValueString(const char* value)
{
  this->LastDisplayString =
    this->WrapValueWithPrefixAndSuffix(std::string(value));
  return this->LastDisplayString.c_str();
}

//----------------------------------------------------------------------------
const char* vtkMRMLUnitNode::GetDisplayStringFromDisplayValueString(const char* firstDisplayValueString,
                                                                    const char* secondDisplayValueString,
                                                                    const char* thirdDisplayValueString)
{
  this->LastDisplayString =
    this->WrapValueWithPrefixAndSuffix(std::string(firstDisplayValueString),
                                       std::string(secondDisplayValueString),
                                       std::string(thirdDisplayValueString));
  return this->LastDisplayString.c_str();
}

//----------------------------------------------------------------------------
const char* vtkMRMLUnitNode::GetDisplayStringFormat()
{
  std::stringstream strstream;
  strstream << this->GetPrefix();
  strstream << "%";
  strstream << "-"; // left justify
  strstream << "#"; // force decimal point
  strstream << floor(log10(1. + fabs(this->GetMaximumValue()))); // padd
  strstream << "." << this->GetPrecision(); // decimals
  strstream << "g";
  strstream << this->Suffix;
  if(strcmp(this->SecondSuffix, "") != 0)
    {
    strstream << "0";
    strstream << this->SecondSuffix;
    }
  if(strcmp(this->ThirdSuffix, "") != 0)
    {
    strstream << "0";
    strstream << this->ThirdSuffix;
    }
  strstream >> this->LastDisplayString;
  return this->LastDisplayString.c_str();
}

//----------------------------------------------------------------------------
std::string vtkMRMLUnitNode::WrapValueWithPrefix(const std::string& value) const
{
  std::string wrappedString = "";
  if (this->Prefix)
    {
    wrappedString = std::string(this->Prefix) + " ";
    }
  return wrappedString + value;
}

//----------------------------------------------------------------------------
std::string vtkMRMLUnitNode::WrapValueWithSuffix(const std::string& value) const
{
  std::string wrappedString = "";
  if (this->Suffix)
    {
    wrappedString = " " + std::string(this->Suffix);
    }
  return value + wrappedString;
}

//----------------------------------------------------------------------------
std::string vtkMRMLUnitNode::WrapValueWithPrefixAndSuffix(const std::string& value) const
{
  return this->WrapValueWithPrefix(this->WrapValueWithSuffix(value));
}

//----------------------------------------------------------------------------
std::string vtkMRMLUnitNode::WrapValueWithSuffix(const std::string &firstValue,
                                                 const std::string &secondValue,
                                                 const std::string &thirdValue) const
{
  std::string firstWrappedString = "";
  std::string secondWrappedString = "";
  std::string thirdWrappedString = "";
  if (this->Suffix and this->SecondSuffix and this->ThirdSuffix)
    {
    firstWrappedString = std::string(this->Suffix) + " ";
    secondWrappedString = std::string(this->SecondSuffix) + " ";
    thirdWrappedString = std::string(this->ThirdSuffix);
    }
  return firstValue + firstWrappedString + secondValue + secondWrappedString + thirdValue + thirdWrappedString;
}

//----------------------------------------------------------------------------
std::string vtkMRMLUnitNode::WrapValueWithPrefixAndSuffix(const std::string &firstValue,
                                                          const std::string &secondValue,
                                                          const std::string &thirdValue) const
{
  return this->WrapValueWithPrefix(this->WrapValueWithSuffix(firstValue, secondValue, thirdValue));
}


//----------------------------------------------------------------------------
void vtkMRMLUnitNode::SetName(const char* name)
{
  this->Superclass::SetName(name);
  this->SetSingletonTag(name);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, ID
void vtkMRMLUnitNode::Copy(vtkMRMLNode *anode)
{
  vtkMRMLUnitNode *node = vtkMRMLUnitNode::SafeDownCast(anode);
  if (!node)
    {
    return;
    }

  int disabledModify = this->StartModify();
  this->Superclass::Copy(anode);

  this->SetQuantity(node->GetQuantity());
  this->SetPrefix(node->GetPrefix());
  this->SetSuffix(node->GetSuffix());
  this->SetSecondSuffix(node->GetSecondSuffix());
  this->SetThirdSuffix(node->GetThirdSuffix());
  this->SetPrecision(node->GetPrecision());
  this->SetMinimumValue(node->GetMinimumValue());
  this->SetMaximumValue(node->GetMaximumValue());
  this->SetDisplayCoefficient(node->GetDisplayCoefficient());
  this->SetDisplayOffset(node->GetDisplayOffset());

  this->EndModify(disabledModify);
}

//----------------------------------------------------------------------------
void vtkMRMLUnitNode::Reset(vtkMRMLNode* defaultNode)
{
  if (this->GetSingletonTag() != 0)
    {
    return;
    }
  this->Superclass::Reset(defaultNode);
}

//----------------------------------------------------------------------------
void vtkMRMLUnitNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Quantity: " <<
    (this->GetQuantity() ? this->GetQuantity() : "(none)") << "\n";
  os << indent << "Prefix: " <<
    (this->Prefix ? this->Prefix : "(none)") << "\n";
  os << indent << "Suffix: " <<
    (this->Suffix ? this->Suffix : "(none)") << "\n";
  os << indent << "SecondSuffix: " <<
    (this->SecondSuffix ? this->SecondSuffix : "(none)") << "\n";
  os << indent << "ThirdSuffix: " <<
    (this->ThirdSuffix ? this->ThirdSuffix : "(none)") << "\n";
  os << indent << "Precision: " << this->Precision << "\n";
  os << indent << "MinimumValue: " << this->MinimumValue << "\n";
  os << indent << "MaximumValue: " << this->MaximumValue << "\n";
}
