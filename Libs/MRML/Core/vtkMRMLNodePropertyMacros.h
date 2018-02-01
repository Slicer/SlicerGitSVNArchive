/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

#ifndef __vtkMRMLNodePropertyMacros_h
#define __vtkMRMLNodePropertyMacros_h


// Macros for writing node properties to XML
//----------------------------------------------------------------------------

#define vtkMRMLWriteXMLBeginMacro(of) \
  { \
  ostream& xmlWriteOutputStream = of;

#define vtkMRMLWriteXMLEndMacro() \
  }

#define vtkMRMLWriteXMLBooleanMacro(xmlAttributeName, propertyName) \
  xmlWriteOutputStream << " " #xmlAttributeName "=\"" << (Get##propertyName() ? "true" : "false") << "\"";

#define vtkMRMLWriteXMLStringMacro(xmlAttributeName, propertyName) \
  xmlWriteOutputStream << " " #xmlAttributeName "=\""; \
  if (Get##propertyName() != NULL) \
    { \
    xmlWriteOutputStream << vtkMRMLNode::XMLAttributeEncodeString(Get##propertyName()); \
    } \
  xmlWriteOutputStream << "\"";

#define vtkMRMLWriteXMLStdStringMacro(xmlAttributeName, propertyName) \
  xmlWriteOutputStream << " " #xmlAttributeName "=\"" << vtkMRMLNode::XMLAttributeEncodeString(Get##propertyName().c_str()) << "\""; \

#define vtkMRMLWriteXMLEnumMacro(xmlAttributeName, propertyName) \
  xmlWriteOutputStream << " " #xmlAttributeName "=\""; \
  if (Get##propertyName() != NULL) \
    { \
    xmlWriteOutputStream << vtkMRMLNode::XMLAttributeEncodeString(Get##propertyName##AsString(Get##propertyName())) << "\""; \
    } \
  xmlWriteOutputStream << "\"";

#define vtkMRMLWriteXMLIntMacro(xmlAttributeName, propertyName) \
  xmlWriteOutputStream << " " #xmlAttributeName "=\"" << Get##propertyName() << "\"";

#define vtkMRMLWriteXMLFloatMacro(xmlAttributeName, propertyName) \
  xmlWriteOutputStream << " " #xmlAttributeName "=\"" << Get##propertyName() << "\"";

#define vtkMRMLWriteXMLVectorMacro(xmlAttributeName, propertyName, vectorType, vectorSize) \
  xmlWriteOutputStream << " " #xmlAttributeName "=\""; \
  vectorType* vectorPtr = Get##propertyName(); \
  if (vectorPtr != NULL) \
    { \
    for (int i=0; i<vectorSize; i++) \
      { \
      if (i > 0) \
        { \
        xmlWriteOutputStream << " "; \
        } \
      xmlWriteOutputStream << vectorPtr[i]; \
      } \
    } \
  xmlWriteOutputStream << "\"";

// Macros for reading node properties from XML
//----------------------------------------------------------------------------

#define vtkMRMLReadXMLBeginMacro(atts) \
  { \
  const char* xmlReadAttName; \
  const char* xmlReadAttValue; \
  while (*atts != NULL) \
    { \
    xmlReadAttName = *(atts++); \
    xmlReadAttValue = *(atts++); \
    if (xmlReadAttValue == NULL) \
      { \
      break; \
      }

#define vtkMRMLReadXMLEndMacro() \
  }};

#define vtkMRMLReadXMLBooleanMacro(xmlAttributeName, propertyName) \
  else if (!strcmp(xmlReadAttName, #xmlAttributeName)) \
    { \
    this->Set##propertyName(strcmp(xmlReadAttValue,"true") ? false : true); \
    }

#define vtkMRMLReadXMLStringMacro(xmlAttributeName, propertyName) \
  else if (!strcmp(xmlReadAttName, #xmlAttributeName)) \
    { \
    this->Set##propertyName(vtkMRMLNode::XMLAttributeDecodeString(xmlReadAttValue).c_str()); \
    }

#define vtkMRMLReadXMLStdStringMacro(xmlAttributeName, propertyName) \
  else if (!strcmp(xmlReadAttName, #xmlAttributeName)) \
    { \
    this->Set##propertyName(vtkMRMLNode::XMLAttributeDecodeString(xmlReadAttValue)); \
    }

#define vtkMRMLReadXMLEnumMacro(xmlAttributeName, propertyName) \
  else if (!strcmp(xmlReadAttName, #xmlAttributeName)) \
    { \
    int propertyValue = this->Get##propertyName##FromString(vtkMRMLNode::XMLAttributeDecodeString(xmlReadAttValue).c_str()); \
    if (propertyValue >= 0) \
      { \
      this->Set##propertyName(propertyValue); \
      } \
    else \
      { \
      vtkErrorMacro("Failed to read #xmlAttributeName attribute value from string '" << xmlReadAttValue << "'"); \
      } \
    }

#define vtkMRMLReadXMLIntMacro(xmlAttributeName, propertyName) \
  else if (!strcmp(xmlReadAttName, #xmlAttributeName)) \
    { \
    vtkVariant variantValue(xmlReadAttValue); \
    bool valid = false; \
    int intValue =  variantValue.ToInt(&valid); \
    if (valid) \
      { \
      this->Set##propertyName(intValue); \
      } \
    else \
      { \
      vtkErrorMacro("Failed to read #xmlAttributeName attribute value from string '" << xmlReadAttValue << "': integer expected"); \
      } \
    }

#define vtkMRMLReadXMLFloatMacro(xmlAttributeName, propertyName) \
  else if (!strcmp(xmlReadAttName, #xmlAttributeName)) \
    { \
    vtkVariant variantValue(xmlReadAttValue); \
    bool valid = false; \
    int scalarValue =  variantValue.ToFloat(&valid); \
    if (valid) \
      { \
      this->Set##propertyName(scalarValue); \
      } \
    else \
      { \
      vtkErrorMacro("Failed to read #xmlAttributeName attribute value from string '" << xmlReadAttValue << "': float expected"); \
      } \
    }

#define vtkMRMLReadXMLVectorMacro(xmlAttributeName, propertyName, vectorType, vectorSize) \
  else if (!strcmp(xmlReadAttName, #xmlAttributeName)) \
    { \
    vectorType vectorValue[vectorSize] = {0}; \
    std::stringstream ss; \
    ss << xmlReadAttValue; \
    for (int i=0; i<vectorSize; i++) \
      { \
      vectorType val; \
      ss >> val; \
      vectorValue[i] = val; \
      } \
    this->Set##propertyName(vectorValue); \
    }

// Macros for copying node properties
//----------------------------------------------------------------------------

#define vtkMRMLCopyBeginMacro(sourceNode, nodeClassName) \
  { \
  nodeClassName* copySourceNode = nodeClassName::SafeDownCast(sourceNode); \
  if (copySourceNode != NULL) \
    {

#define vtkMRMLCopyEndMacro() \
    } \
  else \
    { \
    vtkErrorMacro("Copy failed: invalid source node"); \
    } \
  }

#define vtkMRMLCopyBooleanMacro(propertyName) \
  this->Set##propertyName(copySourceNode->Get##propertyName());

#define vtkMRMLCopyStringMacro(propertyName) \
  this->Set##propertyName(copySourceNode->Get##propertyName());

#define vtkMRMLCopyStdStringMacro(propertyName) \
  this->Set##propertyName(copySourceNode->Get##propertyName());

#define vtkMRMLCopyIntMacro(propertyName) \
  this->Set##propertyName(copySourceNode->Get##propertyName());

#define vtkMRMLCopyEnumMacro(propertyName) \
  this->Set##propertyName(copySourceNode->Get##propertyName());

#define vtkMRMLCopyFloatMacro(propertyName) \
  this->Set##propertyName(copySourceNode->Get##propertyName());

#define vtkMRMLCopyVectorMacro(propertyName, vectorType, vectorSize) \
  this->Set##propertyName(copySourceNode->Get##propertyName());

// Macros for printing node properties
//----------------------------------------------------------------------------

#define vtkMRMLPrintBeginMacro(os, indent) \
  { \
  ostream& printOutputStream = os; \
  vtkIndent printOutputIndent = indent;

#define vtkMRMLPrintEndMacro() \
  }

#define vtkMRMLPrintBooleanMacro(propertyName) \
  os << indent << #propertyName ": " << (this->Get##propertyName() ? "true" : "false")  << "\n";

#define vtkMRMLPrintStringMacro(propertyName) \
  os << indent << #propertyName ": " << (this->Get##propertyName() != NULL ? this->Get##propertyName() : "(none)")  << "\n";

#define vtkMRMLPrintStdStringMacro(propertyName) \
  os << indent << #propertyName ": " << this->Get##propertyName() << "\n";

#define vtkMRMLPrintEnumMacro(propertyName) \
  os << indent << #propertyName ": " << (Get##propertyName##AsString(Get##propertyName()))  << "\n";

#define vtkMRMLPrintIntMacro(propertyName) \
  os << indent << #propertyName ": " << this->Get##propertyName() << "\n";

#define vtkMRMLPrintFloatMacro(propertyName) \
  os << indent << #propertyName ": " << this->Get##propertyName() << "\n";

#define vtkMRMLPrintVectorMacro(propertyName, vectorType, vectorSize) \
  os << indent << #propertyName " : ["; \
  vectorType* vectorValue = this->Get##propertyName(); \
  if (vectorValue) \
    { \
    for (int i=0; i<vectorSize; i++) \
      { \
      if (i > 0) \
        { \
        os << ", "; \
        } \
      os << vectorValue[i]; \
      } \
    os << "]\n"; \
    }

#endif // __vtkMRMLNodePropertyMacros_h
