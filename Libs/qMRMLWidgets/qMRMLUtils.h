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

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qMRMLUtils_h
#define __qMRMLUtils_h

/// Qt includes
#include <QString>
#include <QVector>
#include <QIcon>
#include <QPixmap>

#include "qMRMLWidgetsExport.h"

class QStyle;
class vtkMRMLNode;
class vtkMRMLLinearTransformNode;
class vtkTransform;
class vtkMatrix4x4;
class vtkMRMLScene;
class vtkImageData;

#define compare_double(x, y) (((x-y)<0.000001) && ((x-y)>-0.000001))

class QMRML_WIDGETS_EXPORT qMRMLUtils
{
  
public:
  typedef qMRMLUtils Self; 
  
  /// 
  /// Convert a vtkMatrix to a QVector
  static void vtkMatrixToQVector(vtkMatrix4x4* matrix, QVector<double> & vector); 
  
  /// 
  static void getTransformInCoordinateSystem(vtkMRMLNode* transformNode, bool global, 
    vtkTransform* transform); 
  static void getTransformInCoordinateSystem(vtkMRMLLinearTransformNode* transformNode, 
    bool global, vtkTransform* transform);

  /// Retrieve the number of visible view node associated with \a scene
  static int countVisibleViewNode(vtkMRMLScene* scene);

  /// Create Icon using the given color
  static QPixmap createColorPixmap(QStyle * style, const QColor& color);

  /// Convert vtkImageData to QImage
  static bool vtkImageDataToQImage(vtkImageData* vtkimage, QImage& img);

  /// Convert QImage to vtkImageData
  static bool qImageToVtkImageData(const QImage& img, vtkImageData* vtkimage);

private:
  /// Not implemented
  qMRMLUtils(){}
  virtual ~qMRMLUtils(){}

};

#endif
