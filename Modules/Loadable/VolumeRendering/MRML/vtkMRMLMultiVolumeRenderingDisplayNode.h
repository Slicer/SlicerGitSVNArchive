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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care
  and CANARIE.

==============================================================================*/

#ifndef __vtkMRMLMultiVolumeRenderingDisplayNode_h
#define __vtkMRMLMultiVolumeRenderingDisplayNode_h

// Volume Rendering includes
#include "vtkMRMLVolumeRenderingDisplayNode.h"

/// \ingroup Slicer_QtModules_VolumeRendering
/// \name vtkMRMLGPURayCastGPURayCastVolumeRenderingDisplayNode
/// \brief MRML node for storing information for GPU Raycast Volume Rendering
class VTK_SLICER_VOLUMERENDERING_MODULE_MRML_EXPORT vtkMRMLMultiVolumeRenderingDisplayNode
  : public vtkMRMLVolumeRenderingDisplayNode
{
public:
  static vtkMRMLMultiVolumeRenderingDisplayNode *New();
  vtkTypeMacro(vtkMRMLMultiVolumeRenderingDisplayNode,vtkMRMLVolumeRenderingDisplayNode);
  virtual void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "MultiVolumeRendering";}

  // Description:
  // Ray cast technique
  vtkGetMacro(RaycastTechnique, int);
  vtkSetMacro(RaycastTechnique, int);

  // Description:
  // Reduce wood grain artifact to make surfaces appear smoother.
  // For example, by applying jittering on casted rays.
  vtkGetMacro(SurfaceSmoothing, bool);
  vtkSetMacro(SurfaceSmoothing, bool);

protected:
  vtkMRMLMultiVolumeRenderingDisplayNode();
  ~vtkMRMLMultiVolumeRenderingDisplayNode();
  vtkMRMLMultiVolumeRenderingDisplayNode(const vtkMRMLMultiVolumeRenderingDisplayNode&);
  void operator=(const vtkMRMLMultiVolumeRenderingDisplayNode&);

  /* techniques in GPU ray cast
   * 0: composite with directional lighting (default)
   * 2: MIP
   * 3: MINIP
   * */
  int RaycastTechnique;

  /// Make surface appearance smoother. Off by default
  bool SurfaceSmoothing;
};

#endif
