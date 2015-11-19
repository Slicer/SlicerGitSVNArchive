/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

#ifndef __vtkMRMLColorLogic_h
#define __vtkMRMLColorLogic_h

// MRMLLogic includes
#include "vtkMRMLAbstractLogic.h"
#include "vtkMRMLLogicWin32Header.h"

// MRML includes
class vtkMRMLColorNode;
class vtkMRMLColorTableNode;
class vtkMRMLFreeSurferProceduralColorNode;
class vtkMRMLProceduralColorNode;
class vtkMRMLPETProceduralColorNode;
class vtkMRMLdGEMRICProceduralColorNode;
class vtkMRMLColorTableNode;

// STD includes
#include <cstdlib>
#include <vector>

/// \brief MRML logic class for color manipulation.
///
/// This class manages the logic associated with reading, saving,
/// and changing propertied of the colors.
class VTK_MRML_LOGIC_EXPORT vtkMRMLColorLogic : public vtkMRMLAbstractLogic
{
public:

  /// Terminology used with DICOM Segmentation objects
  /// ftp://medical.nema.org/medical/dicom/final/sup111_ft.pdf
  struct VTK_MRML_LOGIC_EXPORT StandardTerm
    {
      StandardTerm() {}

      std::string CodeValue;
      std::string CodingSchemeDesignator;
      std::string CodeMeaning;

      StandardTerm(const std::string& value, const std::string& designator, const std::string& meaning)
      : CodeValue(value),
        CodingSchemeDesignator(designator),
        CodeMeaning(meaning)
      {
      }

      void Print(std::ostream& os);
      std::ostream& operator<<(std::ostream& os);
      void PrintSelf(std::ostream &os, vtkIndent indent);
  };

  struct VTK_MRML_LOGIC_EXPORT ColorLabelCategorization
    {
    unsigned LabelValue;
    StandardTerm SegmentedPropertyCategory;
    StandardTerm SegmentedPropertyType;
    StandardTerm SegmentedPropertyTypeModifier;
    StandardTerm AnatomicRegion;
    StandardTerm AnatomicRegionModifier;

    void Print(std::ostream& os);
    std::ostream& operator<<(std::ostream& os);
    void PrintSelf(ostream &os, vtkIndent indent);
    };


  /// The Usual vtk class functions
  static vtkMRMLColorLogic *New();
  vtkTypeMacro(vtkMRMLColorLogic,vtkMRMLAbstractLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// \brief Add default color nodes.
  ///
  /// The default color nodes are singleton and are not included in the
  /// the saved scene.
  ///
  /// This function enables the vtkMRMLScene::BatchProcessState.
  ///
  /// The type of default nodes along with their properties are listed
  /// in the table below:
  ///
  /// | Family                    | Category                 | Type                           | Node name                      | Singleton Tag                         | Node ID                                                     |
  /// | ------------------------- | ------------------------ | ------------------------------ | ------------------------------ | ------------------------------------- | ----------------------------------------------------------- |
  /// | ColorTable                | Discrete                 | Labels                         | Labels                         | Labels                                | vtkMRMLColorTableNodeLabels                                 |
  /// | ColorTable                | Discrete                 | FullRainbow                    | FullRainbow                    | FullRainbow                           | vtkMRMLColorTableNodeFullRainbow                            |
  /// | ColorTable                | Discrete                 | Grey                           | Grey                           | Grey                                  | vtkMRMLColorTableNodeGrey                                   |
  /// | ColorTable                | Discrete                 | Iron                           | Iron                           | Iron                                  | vtkMRMLColorTableNodeIron                                   |
  /// | ColorTable                | Discrete                 | Rainbow                        | Rainbow                        | Rainbow                               | vtkMRMLColorTableNodeRainbow                                |
  /// | ColorTable                | Discrete                 | Ocean                          | Ocean                          | Ocean                                 | vtkMRMLColorTableNodeOcean                                  |
  /// | ColorTable                | Discrete                 | Desert                         | Desert                         | Desert                                | vtkMRMLColorTableNodeDesert                                 |
  /// | ColorTable                | Discrete                 | InvertedGrey                   | InvertedGrey                   | InvertedGrey                          | vtkMRMLColorTableNodeInvertedGrey                           |
  /// | ColorTable                | Discrete                 | ReverseRainbow                 | ReverseRainbow                 | ReverseRainbow                        | vtkMRMLColorTableNodeReverseRainbow                         |
  /// | ColorTable                | Discrete                 | fMRI                           | fMRI                           | fMRI                                  | vtkMRMLColorTableNodefMRI                                   |
  /// | ColorTable                | Discrete                 | fMRIPA                         | fMRIPA                         | fMRIPA                                | vtkMRMLColorTableNodefMRIPA                                 |
  /// | ColorTable                | Discrete                 | Random                         | Random                         | Random                                | vtkMRMLColorTableNodeRandom                                 |
  /// | ColorTable                | Discrete                 | Red                            | Red                            | Red                                   | vtkMRMLColorTableNodeRed                                    |
  /// | ColorTable                | Discrete                 | Green                          | Green                          | Green                                 | vtkMRMLColorTableNodeGreen                                  |
  /// | ColorTable                | Discrete                 | Blue                           | Blue                           | Blue                                  | vtkMRMLColorTableNodeBlue                                   |
  /// | ColorTable                | Discrete                 | Yellow                         | Yellow                         | Yellow                                | vtkMRMLColorTableNodeYellow                                 |
  /// | ColorTable                | Discrete                 | Cyan                           | Cyan                           | Cyan                                  | vtkMRMLColorTableNodeCyan                                   |
  /// | ColorTable                | Discrete                 | Magenta                        | Magenta                        | Magenta                               | vtkMRMLColorTableNodeMagenta                                |
  /// | ColorTable                | Discrete                 | Warm1                          | Warm1                          | Warm1                                 | vtkMRMLColorTableNodeWarm1                                  |
  /// | ColorTable                | Discrete                 | Warm2                          | Warm2                          | Warm2                                 | vtkMRMLColorTableNodeWarm2                                  |
  /// | ColorTable                | Discrete                 | Warm3                          | Warm3                          | Warm3                                 | vtkMRMLColorTableNodeWarm3                                  |
  /// | ColorTable                | Discrete                 | Cool1                          | Cool1                          | Cool1                                 | vtkMRMLColorTableNodeCool1                                  |
  /// | ColorTable                | Discrete                 | Cool2                          | Cool2                          | Cool2                                 | vtkMRMLColorTableNodeCool2                                  |
  /// | ColorTable                | Discrete                 | Cool3                          | Cool3                          | Cool3                                 | vtkMRMLColorTableNodeCool3                                  |
  /// | ColorTable                | Shade                    | WarmShade1                     | WarmShade1                     | WarmShade1                            | vtkMRMLColorTableNodeWarmShade1                             |
  /// | ColorTable                | Shade                    | WarmShade2                     | WarmShade2                     | WarmShade2                            | vtkMRMLColorTableNodeWarmShade2                             |
  /// | ColorTable                | Shade                    | WarmShade3                     | WarmShade3                     | WarmShade3                            | vtkMRMLColorTableNodeWarmShade3                             |
  /// | ColorTable                | Shade                    | CoolShade1                     | CoolShade1                     | CoolShade1                            | vtkMRMLColorTableNodeCoolShade1                             |
  /// | ColorTable                | Shade                    | CoolShade2                     | CoolShade2                     | CoolShade2                            | vtkMRMLColorTableNodeCoolShade2                             |
  /// | ColorTable                | Shade                    | CoolShade3                     | CoolShade3                     | CoolShade3                            | vtkMRMLColorTableNodeCoolShade3                             |
  /// | ColorTable                | Tint                     | WarmTint1                      | WarmTint1                      | WarmTint1                             | vtkMRMLColorTableNodeWarmTint1                              |
  /// | ColorTable                | Tint                     | WarmTint2                      | WarmTint2                      | WarmTint2                             | vtkMRMLColorTableNodeWarmTint2                              |
  /// | ColorTable                | Tint                     | WarmTint3                      | WarmTint3                      | WarmTint3                             | vtkMRMLColorTableNodeWarmTint3                              |
  /// | ColorTable                | Tint                     | CoolTint1                      | CoolTint1                      | CoolTint1                             | vtkMRMLColorTableNodeCoolTint1                              |
  /// | ColorTable                | Tint                     | CoolTint2                      | CoolTint2                      | CoolTint2                             | vtkMRMLColorTableNodeCoolTint2                              |
  /// | ColorTable                | Tint                     | CoolTint3                      | CoolTint3                      | CoolTint3                             | vtkMRMLColorTableNodeCoolTint3                              |
  /// | ProceduralColor           | Discrete                 | RandomIntegers                 | RandomIntegers                 | RandomIntegers                        | vtkMRMLProceduralColorNodeRandomIntegers                    |
  /// | ProceduralColor           | Continuous               | RedGreenBlue                   | RedGreenBlue                   | RedGreenBlue                          | vtkMRMLProceduralColorNodeRedGreenBlue                      |
  /// | FreeSurferProceduralColor | FreeSurfer               | Heat                           | Heat                           | Heat                                  | vtkMRMLFreeSurferProceduralColorNodeHeat                    |
  /// | FreeSurferProceduralColor | FreeSurfer               | BlueRed                        | BlueRed                        | BlueRed                               | vtkMRMLFreeSurferProceduralColorNodeBlueRed                 |
  /// | FreeSurferProceduralColor | FreeSurfer               | RedBlue                        | RedBlue                        | RedBlue                               | vtkMRMLFreeSurferProceduralColorNodeRedBlue                 |
  /// | FreeSurferProceduralColor | FreeSurfer               | RedGreen                       | RedGreen                       | RedGreen                              | vtkMRMLFreeSurferProceduralColorNodeRedGreen                |
  /// | FreeSurferProceduralColor | FreeSurfer               | GreenRed                       | GreenRed                       | GreenRed                              | vtkMRMLFreeSurferProceduralColorNodeGreenRed                |
  /// | ColorTable                | FreeSurfer               | File                           | FreeSurferLabels               | File                                  | vtkMRMLColorTableNodeFile                                   |
  /// | PETProceduralColor        | PET                      | PET-Heat                       | PET-Heat                       | PET-Heat                              | vtkMRMLPETProceduralColorNodePET-Heat                       |
  /// | PETProceduralColor        | PET                      | PET-Rainbow                    | PET-Rainbow                    | PET-Rainbow                           | vtkMRMLPETProceduralColorNodePET-Rainbow                    |
  /// | PETProceduralColor        | PET                      | PET-MaximumIntensityProjection | PET-MaximumIntensityProjection | PET-MaximumIntensityProjection        | vtkMRMLPETProceduralColorNodePET-MaximumIntensityProjection |
  /// | dGEMRICProceduralColor    | Cartilage MRI            | dGEMRIC-1.5T                   | dGEMRIC-1.5T                   | dGEMRIC-1.5T                          | vtkMRMLdGEMRICProceduralColorNodedGEMRIC-1.5T               |
  /// | dGEMRICProceduralColor    | Cartilage MRI            | dGEMRIC-3T                     | dGEMRIC-3T                     | dGEMRIC-3T                            | vtkMRMLdGEMRICProceduralColorNodedGEMRIC-3T                 |
  /// | ColorTable                | None                     | File                           | GenericColors                  | FileGenericColors.txt                 | vtkMRMLColorTableNodeFileGenericColors.txt                  |
  /// | ColorTable                | Default Labels from File | File                           | SPL-BrainAtlas-2009-ColorFile  | FileSPL-BrainAtlas-2009-ColorFile.txt | vtkMRMLColorTableNodeFileSPL-BrainAtlas-2009-ColorFile.txt  |
  /// | ColorTable                | Default Labels from File | File                           | MediumChartColors              | FileMediumChartColors.txt             | vtkMRMLColorTableNodeFileMediumChartColors.txt              |
  /// | ColorTable                | Default Labels from File | File                           | ColdToHotRainbow               | FileColdToHotRainbow.txt              | vtkMRMLColorTableNodeFileColdToHotRainbow.txt               |
  /// | ColorTable                | None                     | File                           | GenericAnatomyColors           | FileGenericAnatomyColors.txt          | vtkMRMLColorTableNodeFileGenericAnatomyColors.txt           |
  /// | ColorTable                | Default Labels from File | File                           | PelvisColor                    | FilePelvisColor.txt                   | vtkMRMLColorTableNodeFilePelvisColor.txt                    |
  /// | ColorTable                | Default Labels from File | File                           | AbdomenColors                  | FileAbdomenColors.txt                 | vtkMRMLColorTableNodeFileAbdomenColors.txt                  |
  /// | ColorTable                | Default Labels from File | File                           | Slicer3_2010_Label_Colors      | FileSlicer3_2010_Label_Colors.txt     | vtkMRMLColorTableNodeFileSlicer3_2010_Label_Colors.txt      |
  /// | ColorTable                | Default Labels from File | File                           | 64Color-Nonsemantic            | File64Color-Nonsemantic.txt           | vtkMRMLColorTableNodeFile64Color-Nonsemantic.txt            |
  /// | ColorTable                | Default Labels from File | File                           | DarkBrightChartColors          | FileDarkBrightChartColors.txt         | vtkMRMLColorTableNodeFileDarkBrightChartColors.txt          |
  /// | ColorTable                | Default Labels from File | File                           | SPL-BrainAtlas-2012-ColorFile  | FileSPL-BrainAtlas-2012-ColorFile.txt | vtkMRMLColorTableNodeFileSPL-BrainAtlas-2012-ColorFile.txt  |
  /// | ColorTable                | Default Labels from File | File                           | LightPaleChartColors           | FileLightPaleChartColors.txt          | vtkMRMLColorTableNodeFileLightPaleChartColors.txt           |
  /// | ColorTable                | Default Labels from File | File                           | Slicer3_2010_Brain_Labels      | FileSlicer3_2010_Brain_Labels.txt     | vtkMRMLColorTableNodeFileSlicer3_2010_Brain_Labels.txt      |
  /// | ColorTable                | Default Labels from File | File                           | HotToColdRainbow               | FileHotToColdRainbow.txt              | vtkMRMLColorTableNodeFileHotToColdRainbow.txt               |
  /// | ColorTable                | Default Labels from File | File                           | SPL-BrainAtlas-ColorFile       | FileSPL-BrainAtlas-ColorFile.txt      | vtkMRMLColorTableNodeFileSPL-BrainAtlas-ColorFile.txt       |
  ///
  /// \note The table has been generated using Libs/MRML/Core/Documentation/generate_default_color_node_property_table.py
  ///
  /// \sa vtkMRMLNode::GetSingletonTag(), vtkMRMLScene::Commit()
  /// \sa RemoveDefaultColorNodes()
  ///
  /// \sa AddLabelsNode()
  /// \sa AddDefaultTableNodes()
  /// \sa AddDefaultProceduralNodes()
  /// \sa AddFreeSurferNodes()
  /// \sa AddPETNodes()
  /// \sa AddDGEMRICNodes()
  /// \sa AddDefaultFileNodes()
  /// \sa AddUserFileNodes()
  virtual void AddDefaultColorNodes();

  /// \brief Remove default color nodes.
  ///
  /// \sa AddDefaultColorNodes()
  virtual void RemoveDefaultColorNodes();

  /// Return the default color table node id for a given type
  static const char * GetColorTableNodeID(int type);

  /// Return the default freesurfer color node id for a given type
  static const char * GetFreeSurferColorNodeID(int type);

  /// Return the default dGEMRIC color node id for a given type
  static const char * GetdGEMRICColorNodeID(int type);

  /// Return the default PET color node id for a given type
  static const char * GetPETColorNodeID(int type);

  /// \brief Return a default color node id for a procedural color node.
  ///
  /// \warning You are responsible to delete the returned string.
  static const char * GetProceduralColorNodeID(const char *name);

  /// \brief Return a default color node id for a file based node,
  /// based on the file name.
  ///
  /// \warning You are responsible to delete the returned string.
  static const char * GetFileColorNodeID(const char *fileName);
  static std::string  GetFileColorNodeSingletonTag(const char * fileName);

  /// Return a default color node id for a freesurfer label map volume
  virtual const char * GetDefaultFreeSurferLabelMapColorNodeID();

  /// Return a default color node id for a volume
  virtual const char * GetDefaultVolumeColorNodeID();

  /// Return a default color node id for a label map
  virtual const char * GetDefaultLabelMapColorNodeID();

  /// Return a default color node id for the editor
  virtual const char * GetDefaultEditorColorNodeID();

  /// Return a default color node id for a model
  virtual const char * GetDefaultModelColorNodeID();

  /// Return a default color node id for a chart
  virtual const char * GetDefaultChartColorNodeID();

  /// Add a file to the input list Files, checking first for null, duplicates
  void AddColorFile(const char *fileName, std::vector<std::string> *Files);

  /// Load in a color file, creating a storage node. Returns a pointer to the
  /// created node on success, 0 on failure (no file, invalid color file). The
  /// name of the created color node is \a nodeName if specified or
  /// the fileName otherwise. Try first to load it as a color table
  /// node, then if that fails, as a procedural color node. It calls
  /// CreateFileNode or CreateProceduralFileNode which are also used
  /// for the built in color nodes, so it has to unset some flags: set
  /// the category to File, turn save with scene on on the node and
  /// it's storage node, turn off hide from editors, remove the
  /// singleton tag.
  /// \sa CreateFileNode, CreateProceduralFileNode
  vtkMRMLColorNode* LoadColorFile(const char *fileName, const char *nodeName = NULL);

  /// Get/Set the user defined paths where to look for extra colour files
  vtkGetStringMacro(UserColorFilePaths);
  vtkSetStringMacro(UserColorFilePaths);

  /// Returns a vtkMRMLColorTableNode copy (type = vtkMRMLColorTableNode::User)
  /// of the \a color node. The node is not added to the scene and you are
  /// responsible for deleting it.
  static vtkMRMLColorTableNode* CopyNode(vtkMRMLColorNode* colorNode, const char* copyName);

  /// Returns a vtkMRMLProceduralColorNode copy (type = vtkMRMLColorTableNode::User)
  /// of the \a color node. The node is not added to the scene and you are
  /// responsible for deleting it. If there is no color transfer function on the
  /// input node, for example if it's a color table node, it will return a
  /// procedural node with a blank color transfer function.
  static vtkMRMLProceduralColorNode* CopyProceduralNode(vtkMRMLColorNode* colorNode, const char* copyName);

  /// Return the label's terminology in this color table.
  bool LookupCategorizationFromLabel(int label, ColorLabelCategorization&, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  /// \brief Returns a string with the terminology term requested, or an empty string if not found.
  ///
  /// Valid values for categorization are SegmentedPropertyCategory, SegmentedPropertyType,
  /// SegmentedPropertyTypeModifier, AnatomicRegion, AnatomicRegionModifier.
  /// Valid values for standard term are CodeValue, CodeMeaning, CodingSchemeDesignator
  ///
  /// \sa LookupCategorizationFromLabel
  std::string GetTerminologyFromLabel(const std::string& categorization,
                                      const std::string& standardTerm, int label,
                                      const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  /// Print the terminology for this label in the color table to standard output.
  bool PrintCategorizationFromLabel(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);

  /// Utility methods to look up the terminology for the SegmentedPropertyCategory
  /// for a specific label in a color node.
  /// Returns an empty string if not found or defined.
  std::string GetSegmentedPropertyCategoryCodeValue(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetSegmentedPropertyCategoryCodeMeaning(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetSegmentedPropertyCategoryCodingSchemeDesignator(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  /// Utility method that returns a colon separated triplet for the
  /// SegmentedPropertyCategory for this label in this color node.
  /// Returns an empty string if all elements not found or defined, otherwise a
  /// color separated tuple of the format CodeValue:CodingSchemeDesignator:CodeMeaning
  std::string GetSegmentedPropertyCategory(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);

  /// Utility methods to look up the terminology for the SegmentedPropertyType
  /// for a specific label in a color node.
  /// Returns an empty string if not found or defined.
  std::string GetSegmentedPropertyTypeCodeValue(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetSegmentedPropertyTypeCodeMeaning(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetSegmentedPropertyTypeCodingSchemeDesignator(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  /// Utility method that returns a colon separated triplet for the
  /// SegmentedPropertyType for this label in this color node.
  /// Returns an empty string if all elements not found or defined, otherwise a
  /// color separated tuple of the format CodeValue:CodingSchemeDesignator:CodeMeaning
  std::string GetSegmentedPropertyType(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);

  /// Utility methods to look up the terminology for the SegmentedPropertyTypeModifier
  /// for a specific label in a color node.
  /// Returns an empty string if not found or defined.
  std::string GetSegmentedPropertyTypeModifierCodeValue(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetSegmentedPropertyTypeModifierCodeMeaning(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetSegmentedPropertyTypeModifierCodingSchemeDesignator(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  /// Utility method that returns a colon separated triplet for the
  /// SegmentedPropertyTypeModifier for this label in this color node.
  /// Returns an empty string if all elements not found or defined, otherwise a
  /// color separated tuple of the format CodeValue:CodingSchemeDesignator:CodeMeaning
  std::string GetSegmentedPropertyTypeModifier(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);

  /// Utility methods to look up the terminology for the AnatomicRegion
  /// for a specific label in a color node.
  /// Returns an empty string if not found or defined.
  std::string GetAnatomicRegionCodeValue(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetAnatomicRegionCodeMeaning(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetAnatomicRegionCodingSchemeDesignator(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  /// Utility method that returns a colon separated triplet for the
  /// AnatomicRegion for this label in this color node.
  /// Returns an empty string if all elements not found or defined, otherwise a
  /// color separated tuple of the format CodeValue:CodingSchemeDesignator:CodeMeaning
  std::string GetAnatomicRegion(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);

  /// Utility methods to look up the terminology for the AnatomicRegionModifier
  /// for a specific label in a color node.
  /// Returns an empty string if not found or defined.
  std::string GetAnatomicRegionModifierCodeValue(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetAnatomicRegionModifierCodeMeaning(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  std::string GetAnatomicRegionModifierCodingSchemeDesignator(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);
  /// Utility method that returns a colon separated triplet for the
  /// AnatomicRegionModifier for this label in this color node.
  /// Returns an empty string if all elements not found or defined, otherwise a
  /// color separated tuple of the format CodeValue:CodingSchemeDesignator:CodeMeaning
  std::string GetAnatomicRegionModifier(int label, const char *lutName = vtkMRMLColorLogic::DEFAULT_TERMINOLOGY_NAME);

  /// Create a new empty terminology for this LUT and try to associate it with a color
  /// node of the same name.
  /// Returns true on success, false if lutName is empty or can't associate the new
  /// terminology with a color node
  /// \sa AssociateTerminologyWithColorNode
  bool CreateNewTerminology(std::string lutName);
  /// Return true if a terminology mapping already exists for this lutName,
  // false if empty name or none found.
  bool TerminologyExists(std::string lutName);
  /// Link the color node with the terminology
  /// Returns true on success, false if empty lut name, unable to find the color node
  /// \sa CreateNewTerminology
  bool AssociateTerminologyWithColorNode(std::string lutName);
  /// For this label value, construct standard terms (CodeValue (*Value),
  /// CodingSchemeDesignator (*SchemeDesignator), CodeMeaning (*Meaning)) for
  /// SegmentedPropertyCategory (category*), SegmentedPropertyType (type*),
  /// SegmentedPropertyTypeModifier (modifier*), AnatomicRegion
  /// (region*) and AnatomicRegionModifier (regionModifier*), then add them to the terminology
  /// associated with the lutName.
  /// Returns the result of AddTermToTerminologyMapping
  /// \sa AddTermToTerminologyMapping
  bool AddTermToTerminology(std::string lutName, int labelValue,
                            std::string categoryValue,
                            std::string categorySchemeDesignator,
                            std::string categoryMeaning,
                            std::string typeValue,
                            std::string typeSchemeDesignator,
                            std::string typeMeaning,
                            std::string modifierValue,
                            std::string modifierSchemeDesignator,
                            std::string modifierMeaning,
                            std::string regionValue,
                            std::string regionSchemeDesignator,
                            std::string regionMeaning,
                            std::string regionModifierValue,
                            std::string regionModifierSchemeDesignator,
                            std::string regionModifierMeaning);

  // public for python wrapping of the methods where it's used to initialize parameters
  static const char *DEFAULT_TERMINOLOGY_NAME;

protected:
  vtkMRMLColorLogic();
  virtual ~vtkMRMLColorLogic();
  // disable copy constructor and operator
  vtkMRMLColorLogic(const vtkMRMLColorLogic&);
  void operator=(const vtkMRMLColorLogic&);

  /// Reimplemented to listen to specific scene events
  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);

  /// Called when the scene fires vtkMRMLScene::NewSceneEvent.
  /// We add the default LUTs.
  virtual void OnMRMLSceneNewEvent();

  vtkMRMLColorTableNode* CreateLabelsNode();
  vtkMRMLColorTableNode* CreateDefaultTableNode(int type);
  vtkMRMLProceduralColorNode* CreateRandomNode();
  vtkMRMLProceduralColorNode* CreateRedGreenBlueNode();
  vtkMRMLFreeSurferProceduralColorNode* CreateFreeSurferNode(int type);
  vtkMRMLColorTableNode* CreateFreeSurferFileNode(const char* fileName);
  vtkMRMLPETProceduralColorNode* CreatePETColorNode(int type);
  vtkMRMLdGEMRICProceduralColorNode* CreatedGEMRICColorNode(int type);
  vtkMRMLColorTableNode* CreateDefaultFileNode(const std::string& colorname);
  vtkMRMLColorTableNode* CreateUserFileNode(const std::string& colorname);
  vtkMRMLColorTableNode* CreateFileNode(const char* fileName);
  vtkMRMLProceduralColorNode* CreateProceduralFileNode(const char* fileName);

  void AddLabelsNode();
  void AddDefaultTableNode(int i);
  void AddDefaultProceduralNodes();
  void AddFreeSurferNode(int type);
  void AddFreeSurferFileNode(vtkMRMLFreeSurferProceduralColorNode* basicFSNode);
  void AddPETNode(int type);
  void AddDGEMRICNode(int type);
  void AddDefaultFileNode(int i);
  void AddUserFileNode(int i);

  void AddDefaultTableNodes();
  void AddFreeSurferNodes();
  void AddPETNodes();
  void AddDGEMRICNodes();
  void AddDefaultFileNodes();
  void AddUserFileNodes();

  virtual std::vector<std::string> FindDefaultColorFiles();
  virtual std::vector<std::string> FindUserColorFiles();
  virtual std::vector<std::string> FindDefaultTerminologyColorFiles();

  /// Get the list default terminology color files then initialise
  /// terminology mappings for each one.
  /// \sa FindDefaultTerminologyColorFiles, InitializeTerminologyMappingFromFile
  void AddDefaultTerminologyColors();

  /// For this labelValue, add the passed in terms of region, region modifier, category,
  /// type, modifier to the terminology associated with the lutName. Will create the
  /// terminology for the lutName if it doesn't exist already.
  /// Returns true on success, false if lutName is empty
  /// \sa TerminologyExists
  bool AddTermToTerminologyMapping(std::string lutName, int labelValue,
                                   StandardTerm category, StandardTerm type,
                                   StandardTerm modifier,
                                   StandardTerm region, StandardTerm regionModifier);

  /// Create a new terminology mapping from the given file.
  /// Returns true on success, false if unable to open the file, add terms, or associate
  /// the terminology with a color node.
  /// \sa CreateNewTerminology, AddTermToTerminologyMapping, AssociateTerminologyWithColorNode
  bool InitializeTerminologyMappingFromFile(std::string mapFile);

  /// Return the ID of a node that doesn't belong to a scene.
  /// It is the concatenation of the node class name and its type.
  static const char * GetColorNodeID(vtkMRMLColorNode* colorNode);

  /// a vector holding discovered default colour files, found in the
  /// Resources/ColorFiles directory, white space separated with:
  /// int name r g b a
  /// with rgba in the range 0-255
  std::vector<std::string> ColorFiles;

  /// a vector holding discovered default terminology files that are
  /// linked with default Slicer color files (not all color files
  /// have terminology files). Found in the Terminology subdirectory
  /// of the ColorFiles directory, they are comma separated value files
  /// with:
  /// Integer Label,Text Label,Segmented Property Category -- CID 7150++,Segmented Property Type,Segmented Property Type Modifier,Color
  /// Integer Label is a number
  /// Text Label is the name of the color
  /// Segmented Property * is as defined by the terminology standard, inside brackets
  /// Color is rgb(r;g;b) where each of r, g, b are in teh range 0-255
  /// The first non commented line in the file gives the name of the Slicer LUT,
  /// for example:
  /// SlicerLUT=GenericAnatomyColors
  std::vector<std::string> TerminologyColorFiles;

  /// a vector holding discovered user defined colour files, found in the
  /// UserColorFilesPath directories.
  std::vector<std::string> UserColorFiles;
  /// a string holding delimiter separated (; on win32, : else) paths where to
  /// look for extra colour files, set from the return value of
  /// vtkMRMLApplication::GetColorFilePaths
  char *UserColorFilePaths;

  /// Mappings used for terminology color look ups
  typedef std::map<int,ColorLabelCategorization> ColorCategorizationMapType;
  std::map<std::string, ColorCategorizationMapType> ColorCategorizationMaps;

  static std::string TempColorNodeID;

  std::string RemoveLeadAndTrailSpaces(std::string);
  bool ParseTerm(const std::string, StandardTerm&);
};

#endif
