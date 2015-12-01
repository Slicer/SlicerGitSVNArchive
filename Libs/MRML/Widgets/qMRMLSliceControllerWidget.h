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

#ifndef __qMRMLSliceControllerWidget_h
#define __qMRMLSliceControllerWidget_h

// qMRMLWidget includes
#include "qMRMLViewControllerBar.h"
#include <vtkVersion.h>

class QButtonGroup;
class qMRMLSliceControllerWidgetPrivate;
class vtkAlgorithmOutput;
class vtkCollection;
class vtkImageData;
class vtkMRMLNode;
class vtkMRMLScene;
class vtkMRMLSliceCompositeNode;
class vtkMRMLSliceLogic;
class vtkMRMLSliceNode;

/// qMRMLSliceControllerWidget offers controls to a slice view (vtkMRMLSliceNode
///  and vtkMRMLSliceCompositeNode). It internally creates a slice logic that
/// can be changed.
///
/// To be valid, it needs at least a MRML scene and a MRML slice node:
/// \code
/// qMRMLSliceControllerWidget controllerWidget;
/// controllerWidget.setSliceViewName("Red");
/// controllerWidget.setSliceViewLabel("R");
/// controllerWidget.setSliceViewColor(Qt::red);
/// controllerWidget.setMRMLScene(scene);
/// controllerWidget.setMRMLSliceNode(sliceNode);
/// \endcode
class QMRML_WIDGETS_EXPORT qMRMLSliceControllerWidget
  : public qMRMLViewControllerBar
{
  Q_OBJECT
  Q_PROPERTY(QString sliceViewName READ sliceViewName WRITE setSliceViewName)
  Q_PROPERTY(QString sliceViewLabel READ sliceViewLabel WRITE setSliceViewLabel)
  Q_PROPERTY(bool moreButtonVisibility READ isMoreButtonVisible WRITE setMoreButtonVisible)
public:
  /// Superclass typedef
  typedef qMRMLViewControllerBar Superclass;

  /// Constructors
  explicit qMRMLSliceControllerWidget(QWidget* parent = 0);
  virtual ~qMRMLSliceControllerWidget();

  /// Are the slices linked to each other
  bool isLinked()const;

  /// Is the view a compare view
  bool isCompareView()const;

  /// Get slice orientation
  /// \sa setSliceOrientation(QString);
  QString sliceOrientation()const;

  /// Get imageData from the slice logic.
  /// Returns 0 if there is no volume assigned to
  /// Background, Foreground or LabelMap.
  /// Or if the only volume assigned doesn't have have
  /// a display node or its display node image data is 0.
  vtkAlgorithmOutput* imageDataConnection()const;

  /// Get \a sliceNode
  /// \sa setMRMLSliceCompositeNode();
  vtkMRMLSliceNode* mrmlSliceNode()const;

  /// Get sliceCompositeNode
  /// \sa vtkMRMLSliceLogic::GetSliceCompositeNode();
  vtkMRMLSliceCompositeNode* mrmlSliceCompositeNode()const;

  /// Set slice view name
  /// \note SliceViewName should be set before setMRMLSliceNode() is called
  /// "Red" by default.
  void setSliceViewName(const QString& newSliceViewName);

  /// Get slice view name
  QString sliceViewName()const;

  /// Return the color associated to the slice view
  Q_INVOKABLE static QColor sliceViewColor(const QString& sliceViewName);

  /// Convenience function to set the abbreviated name for the slice view.
  /// This is equivalent to call vtkMRMLSliceNode::SetLayoutLabel()
  /// If no SliceNode is set, this is a no-op.
  /// \sa setMRMLSliceNode(), vtkMRMLSliceNode::SetLayoutLabel()
  void setSliceViewLabel(const QString& newSliceViewLabel);

  /// Get the abbreviated slice view name.
  /// \sa setSliceViewLabel(), vtkMRMLSliceNode::GetLayoutLabel()
  QString sliceViewLabel()const;

  /// Set the color for the slice view
  void setSliceViewColor(const QColor& newSliceViewColor);

  /// Get the color for the slice view (as a string)
  QColor sliceViewColor()const;

  /// Set slice offset range
  void setSliceOffsetRange(double min, double max);

  /// Set slice offset \a resolution (increment)
  void setSliceOffsetResolution(double resolution);

  /// Get SliceLogic
  vtkMRMLSliceLogic* sliceLogic()const;

  /// Set \a newSliceLogic
  /// Use if two instances of the controller need to observe the same logic.
  void setSliceLogic(vtkMRMLSliceLogic * newSliceLogic);

  /// Set controller widget group
  /// All controllers of a same group will be set visible or hidden if at least
  /// one of the sliceCollapsibleButton of the group is clicked.
  void setControllerButtonGroup(QButtonGroup* group);

  /// TODO:
  /// Ideally the slice logics should be retrieved by the sliceLogic
  /// until then, we manually set them.
  void setSliceLogics(vtkCollection* logics);

public slots:

  virtual void setMRMLScene(vtkMRMLScene* newScene);

  /// Set a new SliceNode.
  void setMRMLSliceNode(vtkMRMLSliceNode* newSliceNode);

  /// Set a new imageData.
  void setImageDataConnection(vtkAlgorithmOutput* newImageDataConnection);

  /// \sa fitSliceToBackground();
  void setSliceViewSize(const QSize& newSize);

  /// Fit slices to background. A No-op if no SliceView has been set.
  /// \sa setSliceView();
  void fitSliceToBackground();

  /// Set slice orientation.
  /// \note Orientation could be either "Axial, "Sagittal", "Coronal" or "Reformat".
  void setSliceOrientation(const QString& orientation);

  /// Set slice \a offset. Used to set a single value.
  void setSliceOffsetValue(double offset);

  /// Set slice offset. Used when events will come is rapid succession.
  void trackSliceOffsetValue(double offset);

  /// Set slice visible.
  void setSliceVisible(bool visible);

  /// Link/Unlink the slice controls across all slice viewer
  void setSliceLink(bool linked);

  /// Set the link mode to hot linked. When on, slice interactions affect other
  /// slices immediately. When off, slice interactions affect other
  /// slices after the interaction completes.
  void setHotLinked(bool hot);


  // Advanced options
  /// Set the visibility of the MoreButton which allows to show the advanced
  /// controls.
  void setMoreButtonVisible(bool visible);
  /// Get the visibility of the MoreButton which allows to show the advanced
  /// controls.
  bool isMoreButtonVisible() const;

  void moveBackgroundComboBox(bool move);

  /// Rotate to volume plane
  void rotateSliceToBackground();

  void setLabelMapHidden(bool hide);
  void setForegroundHidden(bool hide);
  void setBackgroundHidden(bool hide);

  /// Label opacity
  void setLabelMapOpacity(double opacity);
  void setForegroundOpacity(double opacity);
  void setBackgroundOpacity(double opacity);

  /// Label outline
  void showLabelOutline(bool show);
  /// Reformat widget
  void showReformatWidget(bool show);
  void lockReformatWidgetToCamera(bool lock);
  /// Compositing
  void setCompositing(int mode);
  void setCompositingToAlphaBlend();
  void setCompositingToReverseAlphaBlend();
  void setCompositingToAdd();
  void setCompositingToSubtract();
  /// Slice spacing
  void setSliceSpacingMode(bool automatic);
  void setSliceSpacing(double spacing);
  void setSliceFOV(double fov);

  /// Slice Model
  void setSliceModelMode(int mode);
  void setSliceModelModeVolumes();
  void setSliceModelMode2D();
  void setSliceModelMode2D_Volumes();
  void setSliceModelModeVolumes_2D();
  void setSliceModelModeCustom();

  void setSliceModelFOV(int index, double fov);
  void setSliceModelFOVX(double fov);
  void setSliceModelFOVY(double fov);

  void setSliceModelOrigin(int index, double fov);
  void setSliceModelOriginX(double fov);
  void setSliceModelOriginY(double fov);

  void setSliceModelDimension(int index, int dim);
  void setSliceModelDimensionX(int dim);
  void setSliceModelDimensionY(int dim);

  // Lightbox
  void setLightbox(int rows, int columns);
  void setLightboxTo1x1();
  void setLightboxTo1x2();
  void setLightboxTo1x3();
  void setLightboxTo1x4();
  void setLightboxTo1x6();
  void setLightboxTo1x8();
  void setLightboxTo2x2();
  void setLightboxTo3x3();
  void setLightboxTo6x6();
  // interpolation
  void setForegroundInterpolation(bool nearestNeighbor);
  void setBackgroundInterpolation(bool nearestNeighbor);

signals:

  /// This signal is emitted when the given \a imageData is modified.
  void imageDataConnectionChanged(vtkAlgorithmOutput * imageDataConnection);
  void renderRequested();

protected:
  /// Constructor allowing derived class to specify a specialized pimpl.
  ///
  /// \note You are responsible to call init() in the constructor of
  /// derived class. Doing so ensures the derived class is fully
  /// instantiated in case virtual method are called within init() itself.
  qMRMLSliceControllerWidget(qMRMLSliceControllerWidgetPrivate* obj,
                             QWidget* parent);

private:
  Q_DECLARE_PRIVATE(qMRMLSliceControllerWidget);
  Q_DISABLE_COPY(qMRMLSliceControllerWidget);
};

#endif
