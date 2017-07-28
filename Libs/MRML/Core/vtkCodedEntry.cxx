/*=auto=========================================================================

Portions (c) Copyright 2017 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

#include "vtkCodedEntry.h"

// VTK include
#include <vtkObjectFactory.h>

// STD include
#include <sstream>

vtkStandardNewMacro(vtkCodedEntry);

//----------------------------------------------------------------------------
vtkCodedEntry::vtkCodedEntry()
: CodingSchemeDesignator(NULL)
, CodeValue(NULL)
, CodeMeaning(NULL)
{
}

//----------------------------------------------------------------------------
vtkCodedEntry::~vtkCodedEntry()
{
  this->Initialize();
}

//----------------------------------------------------------------------------
void vtkCodedEntry::Initialize()
{
  this->SetCodingSchemeDesignator(NULL);
  this->SetCodeValue(NULL);
  this->SetCodeMeaning(NULL);
}

;

//----------------------------------------------------------------------------
void vtkCodedEntry::SetSchemeValueMeaning(const std::string& scheme,
  const std::string& value, const std::string& meaning)
{
  this->SetCodingSchemeDesignator(scheme.c_str());
  this->SetCodeValue(value.c_str());
  this->SetCodeMeaning(meaning.c_str());
}

//----------------------------------------------------------------------------
void vtkCodedEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
  os << indent << "CodingSchemeDesignator: " << (this->CodingSchemeDesignator?this->CodingSchemeDesignator:"(none)") << "\n";
  os << indent << "CodeValue: " << (this->CodeValue?this->CodeValue:"(none)") << "\n";
  os << indent << "CodeMeaning: " << (this->CodeMeaning?this->CodeMeaning:"(none)") << "\n";
}

//----------------------------------------------------------------------------
void vtkCodedEntry::Copy(vtkCodedEntry* aType)
{
  if (!aType)
    {
    return;
    }
  this->SetCodingSchemeDesignator(aType->GetCodingSchemeDesignator());
  this->SetCodeValue(aType->GetCodeValue());
  this->SetCodeMeaning(aType->GetCodeMeaning());
}

//----------------------------------------------------------------------------
std::string vtkCodedEntry::GetAsPrintableString()
{
  std::string printable = std::string("(")
    + (this->CodingSchemeDesignator ? this->CodingSchemeDesignator : "(none)") + ", "
    + (this->CodeValue ? this->CodeValue : "(none)") + ", \""
    + (this->CodeMeaning ? this->CodeMeaning : "") + "\")";
  return printable;
}

//----------------------------------------------------------------------------
std::string vtkCodedEntry::GetAsString()
{
  std::string str;
  if (this->CodingSchemeDesignator)
    {
    str += "CodingSchemeDesignator:";
    str += this->CodingSchemeDesignator;
    }
  if (this->CodeValue)
    {
    if (!str.empty())
      {
      str += "|";
      }
    str += "CodeValue:";
    str += this->CodeValue;
    }
  if (this->CodeMeaning)
    {
    if (!str.empty())
      {
      str += "|";
      }
    str += "CodeMeaning:";
    str += this->CodeMeaning;
    }
  return str;
}

//----------------------------------------------------------------------------
bool vtkCodedEntry::SetFromString(const std::string& content)
{
  this->Initialize();
  bool success = true;
  std::stringstream attributes(content);
  std::string attribute;
  while (std::getline(attributes, attribute, '|'))
    {
    int colonIndex = attribute.find(':');
    std::string name = attribute.substr(0, colonIndex);
    std::string value = attribute.substr(colonIndex + 1);
    if (name == "CodingSchemeDesignator")
      {
      this->SetCodingSchemeDesignator(value.c_str());
      }
    else if (name == "CodeValue")
      {
      this->SetCodeValue(value.c_str());
      }
    else if (name == "CodeMeaning")
      {
      this->SetCodeMeaning(value.c_str());
      }
    else
      {
      vtkWarningMacro("Parsing coded entry string failed: unknown name " << name << " in " + content);
      success = false;
      }
    }
  if (this->GetCodingSchemeDesignator() == NULL)
    {
    vtkWarningMacro("Parsing coded entry string failed: CodingSchemeDesignator is not specified in " + content);
    success = false;
    }
  if (this->GetCodeValue() == NULL)
    {
    vtkWarningMacro("Parsing coded entry string failed: CodeValue is not specified in " + content);
    success = false;
    }
  // CodeMeaning is optional
  return success;
}
