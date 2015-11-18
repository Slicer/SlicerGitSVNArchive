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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// MRMLLogic includes
#include "vtkSlicerColorLogic.h"

// MRML includes
#include "vtkMRMLCoreTestingUtilities.h"
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkTimerLog.h>

// STD includes

#include "vtkMRMLCoreTestingMacros.h"

using namespace vtkMRMLCoreTestingUtilities;

//----------------------------------------------------------------------------
namespace
{
  bool TestDefaults();
  bool TestTerminology();
}

int vtkSlicerColorLogicTest1(int vtkNotUsed(argc), char * vtkNotUsed(argv)[])
{
  bool res = true;
  res = TestDefaults() && res;
  res = TestTerminology() && res;
  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
namespace
{

//----------------------------------------------------------------------------
bool TestDefaults()
{
  // To load the freesurfer file, SLICER_HOME is requested
  //vtksys::SystemTools::PutEnv("SLICER_HOME=..." );
  vtkNew<vtkMRMLScene> scene;
  vtkSlicerColorLogic* colorLogic = vtkSlicerColorLogic::New();

  vtkSmartPointer<vtkTimerLog> overallTimer = vtkSmartPointer<vtkTimerLog>::New();
  overallTimer->StartTimer();

  colorLogic->SetMRMLScene(scene.GetPointer());

  overallTimer->StopTimer();
  std::cout << "AddDefaultColorNodes: " << overallTimer->GetElapsedTime() << "s"
            << " " << 1. / overallTimer->GetElapsedTime() << "fps" << std::endl;
  overallTimer->StartTimer();

  colorLogic->Delete();

  std::cout << "RemoveDefaultColorNodes: " << overallTimer->GetElapsedTime() << "s"
            << " " << 1. / overallTimer->GetElapsedTime() << "fps" << std::endl;

  return true;
}

//----------------------------------------------------------------------------
// Do the terminology testing here since the Colors module populates the terminology
// file list for loading.
bool TestTerminology()
{
  vtkNew<vtkMRMLScene> scene;
  vtkNew<vtkSlicerColorLogic> colorLogic;
  // setting the scene adds the default color nodes
  colorLogic->SetMRMLScene(scene.GetPointer());

  std::cout << "Testing terminology:" << std::endl;

  std::string lutName = "GenericAnatomyColors";
  if (!colorLogic->TerminologyExists(lutName))
    {
    std::cerr << "Line " << __LINE__
              << ": adding default colors failed to add the terminology for "
              << lutName << std::endl;
    return false;
    }

  std::cout << "Testing PrintCategorizationFromLabel:" << std::endl;
  colorLogic->PrintCategorizationFromLabel(1, lutName.c_str());

  std::cout << "Testing LookupCategorizationFromLabel:" << std::endl;
  vtkMRMLColorLogic::ColorLabelCategorization labelCat;
  if (!colorLogic->LookupCategorizationFromLabel(1, labelCat, lutName.c_str()))
    {
    std::cerr << "Line " << __LINE__
              << ": failed to look up terminology categorization from label 1 for "
              << lutName << std::endl;
    return false;
    }
  labelCat.Print(std::cout);
  if (labelCat.SegmentedPropertyType.CodeMeaning.length() == 0 ||
      labelCat.SegmentedPropertyType.CodeMeaning.compare("Tissue") != 0)
    {
    std::cerr << "Line " << __LINE__
              << ": for label 1, code meaning does not match 'Tissue': '"
              <<  labelCat.SegmentedPropertyType.CodeMeaning.c_str()
              << "'" << std::endl;
    return false;
    }

  // create a new terminology
  std::string pelvisLUTName = "PelvisColor";
  std::cout << "Creating a terminology for lut named " << pelvisLUTName << std::endl;
  std::string regionValue = "T-02480";
  std::string regionMeaning = "Skin of abdomen";
  std::string regionScheme= "SRT";
  std::string catValue = "T-D0050";
  std::string catMeaning = "Tissue";
  std::string catScheme = "SRT";
  std::string typeValue = "T-01000";
  std::string typeMeaning = "Skin";
  std::string typeScheme = "SRT";
  std::string regionModMeaning, regionModValue, regionModScheme, modMeaning, modValue, modScheme;
  std::string sep = std::string(":");
  std::string region = regionValue + sep + regionScheme + sep + regionMeaning;
  std::string category = catValue + sep + catScheme + sep + catMeaning;
  std::string segmentedPropertyType = typeValue + sep + typeScheme + sep + typeMeaning;

  std::cout << "Testing AddTermToTerminology:" << std::endl;
  if (!colorLogic->AddTermToTerminology(pelvisLUTName, 1,
                                        catValue, catScheme, catMeaning,
                                        typeValue, typeScheme, typeMeaning,
                                        modMeaning, modScheme, modValue,
                                        regionValue, regionScheme, regionMeaning,
                                        regionModValue, regionModScheme, regionModMeaning))
    {
    std::cerr << "Line " << __LINE__
              << ": failed to add a new terminology for "
              << pelvisLUTName
              << std::endl;
    return false;
    }
  // check that both terminologies still exist
  if (!colorLogic->TerminologyExists(lutName) ||
      !colorLogic->TerminologyExists(pelvisLUTName))
    {
    std::cerr << "Line " << __LINE__
              << ": after adding a new terminology for "
              << pelvisLUTName
              << ", lost one or both terminologies for it and "
              << lutName
              << std::endl;
    return false;
    }

  // get the label categorisation
  vtkMRMLColorLogic::ColorLabelCategorization pelvisLabelCat;
  std::cout << "Testing new terminology LookupCategorizationFromLabel:" << std::endl;
  if (!colorLogic->LookupCategorizationFromLabel(1, pelvisLabelCat, pelvisLUTName.c_str()))
    {
    std::cerr << "Line " << __LINE__
              << ": failed on trying to look up terminology categorization "
              << " for label 1 for "
              << pelvisLUTName << std::endl;
    return false;
    }
  pelvisLabelCat.Print(std::cout);
  // check the strings
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.AnatomicRegion.CodeMeaning.c_str(),
                   regionMeaning.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.AnatomicRegion.CodeValue.c_str(),
                   regionValue.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.AnatomicRegion.CodingSchemeDesignator.c_str(),
                   regionScheme.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.AnatomicRegionModifier.CodeMeaning.c_str(),
                   regionModMeaning.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.AnatomicRegionModifier.CodeValue.c_str(),
                   regionModValue.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.AnatomicRegionModifier.CodingSchemeDesignator.c_str(),
                   regionModScheme.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyCategory.CodeMeaning.c_str(),
                   catMeaning.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyCategory.CodeValue.c_str(),
                   catValue.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyCategory.CodingSchemeDesignator.c_str(),
                   catScheme.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyType.CodeMeaning.c_str(),
                   typeMeaning.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyType.CodeValue.c_str(),
                   typeValue.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyType.CodingSchemeDesignator.c_str(),
                   typeScheme.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyTypeModifier.CodeMeaning.c_str(),
                   modMeaning.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyTypeModifier.CodeValue.c_str(),
                   modValue.c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology",
                   pelvisLabelCat.SegmentedPropertyTypeModifier.CodingSchemeDesignator.c_str(),
                   modScheme.c_str()))
    {
    return false;
    }
  // check the utility methods to access the values
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   regionMeaning.c_str(),
                   colorLogic->GetAnatomicRegionCodeMeaning(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   regionValue.c_str(),
                   colorLogic->GetAnatomicRegionCodeValue(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   regionScheme.c_str(),
                   colorLogic->GetAnatomicRegionCodingSchemeDesignator(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   regionModMeaning.c_str(),
                   colorLogic->GetAnatomicRegionModifierCodeMeaning(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   regionModValue.c_str(),
                   colorLogic->GetAnatomicRegionModifierCodeValue(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   regionModScheme.c_str(),
                   colorLogic->GetAnatomicRegionModifierCodingSchemeDesignator(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   catMeaning.c_str(),
                   colorLogic->GetSegmentedPropertyCategoryCodeMeaning(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   catValue.c_str(),
                   colorLogic->GetSegmentedPropertyCategoryCodeValue(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   catScheme.c_str(),
                   colorLogic->GetSegmentedPropertyCategoryCodingSchemeDesignator(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   typeMeaning.c_str(),
                   colorLogic->GetSegmentedPropertyTypeCodeMeaning(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   typeValue.c_str(),
                   colorLogic->GetSegmentedPropertyTypeCodeValue(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   typeScheme.c_str(),
                   colorLogic->GetSegmentedPropertyTypeCodingSchemeDesignator(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   modMeaning.c_str(),
                   colorLogic->GetSegmentedPropertyTypeModifierCodeMeaning(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   modValue.c_str(),
                   colorLogic->GetSegmentedPropertyTypeModifierCodeValue(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology accessors",
                   modScheme.c_str(),
                   colorLogic->GetSegmentedPropertyTypeModifierCodingSchemeDesignator(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }

  // check the utility method to get the concatenated strings
  if (!CheckString(__LINE__, "TestTerminology concat strings",
                   category.c_str(),
                   colorLogic->GetSegmentedPropertyCategory(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology concat strings",
                   segmentedPropertyType.c_str(),
                   colorLogic->GetSegmentedPropertyType(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology concat strings",
                   region.c_str(),
                   colorLogic->GetAnatomicRegion(1, pelvisLUTName.c_str()).c_str()))
    {
    return false;
    }
  // check the utility methods getting concatenated terminology strings where the values haven't been set
  std::string regionMods = colorLogic->GetAnatomicRegionModifier(1, pelvisLUTName.c_str());
  std::string typeMods = colorLogic->GetSegmentedPropertyTypeModifier(1, pelvisLUTName.c_str());
  if (!CheckString(__LINE__, "TestTerminology concat empty region mod strings",
                   regionMods.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology concat empty type mod strings",
                   typeMods.c_str(), ""))
    {
    return false;
    }

  // check the method to get a terminology via strings
  std::string termValue = colorLogic->GetTerminologyFromLabel("AnatomicRegion", "CodeMeaning", 1, pelvisLUTName.c_str());
  if (!CheckString(__LINE__, "TestTerminology string accessors",
                   regionMeaning.c_str(), termValue.c_str()))
    {
    return false;
    }

  // look for a label that doesn't have a terminology
  std::cout << "Testing new terminology for known missing label:" << std::endl;
  vtkMRMLColorLogic::ColorLabelCategorization missingPelvisLabelCat;
  if (colorLogic->LookupCategorizationFromLabel(100, missingPelvisLabelCat, pelvisLUTName.c_str()))
    {
    std::cerr << "Line " << __LINE__
              << ": failed on trying to look up missing terminology categorization "
              << " from label 100 for "
              << pelvisLUTName << std::endl;
    return false;
    }
  missingPelvisLabelCat.Print(std::cout);
  // check all are empty strings
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.AnatomicRegion.CodeMeaning.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.AnatomicRegion.CodeValue.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.AnatomicRegion.CodingSchemeDesignator.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.AnatomicRegionModifier.CodeMeaning.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.AnatomicRegionModifier.CodeValue.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.AnatomicRegionModifier.CodingSchemeDesignator.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyCategory.CodeMeaning.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyCategory.CodeValue.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyCategory.CodingSchemeDesignator.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyType.CodeMeaning.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyType.CodeValue.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyType.CodingSchemeDesignator.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyTypeModifier.CodeMeaning.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyTypeModifier.CodeValue.c_str(), ""))
    {
    return false;
    }
  if (!CheckString(__LINE__, "TestTerminology missing label",
                   missingPelvisLabelCat.SegmentedPropertyTypeModifier.CodingSchemeDesignator.c_str(), ""))
    {
    return false;
    }

  return true;
}

}
