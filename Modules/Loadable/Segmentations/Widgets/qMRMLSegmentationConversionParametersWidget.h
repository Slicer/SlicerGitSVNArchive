/*==============================================================================

  Program: 3D Slicer

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

==============================================================================*/

#ifndef __qMRMLSegmentationConversionParametersWidget_h
#define __qMRMLSegmentationConversionParametersWidget_h

// Qt includes
#include <QWidget>

// Segmentations includes
#include "qSlicerSegmentationsModuleWidgetsExport.h"

#include "vtkSegmentationConverter.h"
#include "vtkSegmentationConverterRule.h"

// CTK includes
#include <ctkPimpl.h>
#include <ctkVTKObject.h>

class vtkMRMLNode;
class qMRMLSegmentationConversionParametersWidgetPrivate;
class QTableWidgetItem;
class QItemSelectionModel;

class Q_SLICER_MODULE_SEGMENTATIONS_WIDGETS_EXPORT qMRMLSegmentationConversionParametersWidget : public QWidget
{
  Q_OBJECT
  QVTK_OBJECT

  Q_PROPERTY(QString targetRepresentationName READ targetRepresentationName WRITE setTargetRepresentationName)

public:
  /// Constructor
  explicit qMRMLSegmentationConversionParametersWidget(QWidget* parent = 0);
  /// Destructor
  virtual ~qMRMLSegmentationConversionParametersWidget();

  /// Get segmentation MRML node
  vtkMRMLNode* segmentationNode();

  /// Get target representation name
  QString targetRepresentationName();

  /// Return selected path
  vtkSegmentationConverter::ConversionPathType selectedPath();

  /// Return chosen conversion parameters
  vtkSegmentationConverterRule::ConversionParameterListType conversionParameters();

  /// Set reference image geometry conversion parameter from the selected volume node \sa onGetReferenceImageGeometryFromVolumeClicked
  void setReferenceImageGeometryParameterFromVolumeNode(vtkMRMLNode* node);

signals:
  /// Emitted when conversion is done
  void conversionDone();

public slots:
  /// Set segmentation MRML node
  void setSegmentationNode(vtkMRMLNode* node);

  /// Set target representation name
  void setTargetRepresentationName(QString representationName);

protected slots:
  /// Populate paths table according to the conversion
  void populatePathsTable();

  /// Populate parameters table according to the selected path
  void populateParametersTable();

  /// Handle editing of generic conversation parameters
  void onParameterChanged(QTableWidgetItem* changedItem);

  /// Handles button click for setting reference image geometry from volume node.
  /// The button appears in the row of the reference image geometry conversion parameter, which is a special case.
  /// Pops up a dialog in which the user can select a volume node. The geometry is set when apply is clicked, sa setReferenceImageGeometryParameterFromVolumeNode
  void onSetReferenceImageGeometryFromVolumeClicked();

  /// Create selected representation
  void applyConversion();

protected:
  QScopedPointer<qMRMLSegmentationConversionParametersWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qMRMLSegmentationConversionParametersWidget);
  Q_DISABLE_COPY(qMRMLSegmentationConversionParametersWidget);
};

#endif
