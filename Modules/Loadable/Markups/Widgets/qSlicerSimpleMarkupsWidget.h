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

  This file was originally developed by Matthew Holden, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __qSlicerSimpleMarkupsWidget_h
#define __qSlicerSimpleMarkupsWidget_h

// Qt includes
#include "qSlicerWidget.h"

#include "qMRMLUtils.h"

// Markups Widgets includes
#include "qSlicerMarkupsModuleWidgetsExport.h"
#include "ui_qSlicerSimpleMarkupsWidget.h"


class qSlicerSimpleMarkupsWidgetPrivate;

/// \ingroup Slicer_QtModules_CreateModels
class Q_SLICER_MODULE_MARKUPS_WIDGETS_EXPORT 
qSlicerSimpleMarkupsWidget : public qSlicerWidget
{
  Q_OBJECT
  Q_PROPERTY(bool enterPlaceModeOnNodeChange READ enterPlaceModeOnNodeChange WRITE setEnterPlaceModeOnNodeChange)
  Q_PROPERTY(bool jumpToSliceEnabled READ jumpToSliceEnabled WRITE setJumpToSliceEnabled)
  Q_PROPERTY(bool nodeSelectorVisible READ nodeSelectorVisible WRITE setNodeSelectorVisible)
  Q_PROPERTY(bool optionsVisible READ optionsVisible WRITE setOptionsVisible)

public:
  typedef qSlicerWidget Superclass;
  qSlicerSimpleMarkupsWidget(QWidget *parent=0);
  virtual ~qSlicerSimpleMarkupsWidget();

  /// Get the currently selected markups node.
  Q_INVOKABLE vtkMRMLNode* currentNode() const;

  /// Deprecated. Use currentNode() instead.
  Q_INVOKABLE vtkMRMLNode* getCurrentNode();

  /// Get the markups table widget
  Q_INVOKABLE QTableWidget* tableWidget() const;

  /// Accessors to control place mode behavior
  void setEnterPlaceModeOnNodeChange(bool);
  bool enterPlaceModeOnNodeChange() const;
  
  /// If enabled then the fiducial will be shown in all slice views when a fiducial is selected
  void setJumpToSliceEnabled(bool);
  bool jumpToSliceEnabled() const;

  /// Show/hide the markup node selector widget.
  void setNodeSelectorVisible(bool);
  bool nodeSelectorVisible() const;

  /// Show/hide options (place, activate, color, etc).
  void setOptionsVisible(bool);
  bool optionsVisible() const;

public slots:

  /// Set the currently selected markups node.
  void setCurrentNode(vtkMRMLNode* currentNode);

  void setMRMLScene(vtkMRMLScene* scene);

  /// Set the default name of the markups node created in the combo box.
  void setNodeBaseName(QString newNodeBaseName);

  /// Get the selected color of the currently selected markups node.
  void getNodeColor(QColor color);
  /// Set the selected color of the currently selected markups node.
  void setNodeColor(QColor color);
  /// Set the default color that is assigned to newly created markups nodes in the combo box.
  void setDefaultNodeColor(QColor color);

  /// Scrolls to and selects the Nth fiducial in the table of fiducials.
  void highlightNthFiducial(int n);

  /// Set the currently selected markups node to be the active markups node in the Slicer scene.
  void activate();

  /// Set the currently selected markups node to be the active markups node in the Slicer scene. The argument \a place is true then also interaction mode is set to place mode.
  void placeActive(bool place);

protected slots:
  /// Update the currently selected markups node to have its selected color changed.
  void onColorButtonChanged(QColor);
  /// Toggle the visibility of the markups in the viewers.
  void onVisibilityButtonClicked();
  /// Toggle whether the current markups node is locked.
  void onLockButtonClicked();
  /// Delete a fiducial from the list.
  void onDeleteButtonClicked();
  /// Set the current node to be the active node in the scene and start place mode.
  void onPlaceButtonClicked();
  /// Make the currently selected markups node the active markups node in the scene.
  void onActiveButtonClicked();

  /// Update the widget when a different markups node is selected by the combo box.
  void onMarkupsFiducialNodeChanged();
  /// Setup a newly created markups node - add display node, set color.
  void onMarkupsFiducialNodeAdded(vtkMRMLNode*);
  /// Create context menu for the table displaying the currently selected markups node.
  void onMarkupsFiducialTableContextMenu(const QPoint& position);

  /// Edit the name or position of the currently selected markups node.
  void onMarkupsFiducialEdited(int row, int column);

  /// Clicked on a fiducial or used keyboard to move between fiducials in the table.
  void onMarkupsFiducialSelected(int row, int column);

  /// Update the GUI to reflect the currently selected markups node.
  void updateWidget();

signals:

  /// The signal is emitted when a different markup node is selected.
  void markupsFiducialNodeChanged();

  /// This signal is emitted when a different markup point is selected in the table.
  /// The argument \a markupIndex is the index of the selected markup.
  void currentMarkupsFiducialSelectionChanged(int markupIndex);

  /// The signal is emitted when currently selected markups node has been activated. If place mode is activated then clicking on a view adds a fiducial to the active markup node.
  void markupsFiducialActivated();

  /// This signal is emitted when place mode has changed. This is independent of whether the currently selected markups node is active.
  void markupsFiducialPlaceModeChanged();

  /// This signal is emitted when place mode for the active markup is activated or deactivated.
  /// The argument \a placeActive is true if the currently selected markups node is active and in place mode.
  /// The argument \a placeActive is false if the currently selected markups node is not active or not in place mode. 
  void activeMarkupsFiducialPlaceModeChanged(bool placeActive);

  /// This signal is emitted if updates to the widget have finished.
  /// It is called after fiducials are changed (added, position modified, etc).
  void updateFinished();
protected:
  QScopedPointer<qSlicerSimpleMarkupsWidgetPrivate> d_ptr;

  virtual void setup();

  /// Monitor for changes in the interaction and selection nodes.
  void connectInteractionAndSelectionNodes();

private:
  Q_DECLARE_PRIVATE(qSlicerSimpleMarkupsWidget);
  Q_DISABLE_COPY(qSlicerSimpleMarkupsWidget);

};

#endif
