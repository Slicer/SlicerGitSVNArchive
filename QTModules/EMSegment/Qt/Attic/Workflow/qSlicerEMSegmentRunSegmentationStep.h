/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by
    Danielle Pace and Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#ifndef __qSlicerEMSegmentRunSegmentationStep_h
#define __qSlicerEMSegmentRunSegmentationStep_h

// CTK includes
#include <ctkPimpl.h>

// EMSegment includes
#include "qSlicerEMSegmentWorkflowWidgetStep.h"

#include "qSlicerEMSegmentModuleExport.h"

class qSlicerEMSegmentRunSegmentationStepPrivate;

/// \ingroup Slicer_QtModules_EMSegment
class Q_SLICER_QTMODULES_EMSEGMENT_EXPORT qSlicerEMSegmentRunSegmentationStep : public qSlicerEMSegmentWorkflowWidgetStep
{
  Q_OBJECT

public:

  static const QString StepId;

  typedef qSlicerEMSegmentWorkflowWidgetStep Superclass;
  explicit qSlicerEMSegmentRunSegmentationStep(ctkWorkflow* newWorkflow, QWidget* parent = 0);
  virtual ~qSlicerEMSegmentRunSegmentationStep();

public slots:

  virtual void validate(const QString& desiredBranchId = QString());

  virtual void createUserInterface();

  virtual void showUserInterface();

  virtual void onEntry(const ctkWorkflowStep* comingFrom,
                       const ctkWorkflowInterstepTransition::InterstepTransitionType transitionType);

  virtual void onExit(const ctkWorkflowStep* goingTo,
                      const ctkWorkflowInterstepTransition::InterstepTransitionType transitionType);

protected:
  QScopedPointer<qSlicerEMSegmentRunSegmentationStepPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerEMSegmentRunSegmentationStep);
  Q_DISABLE_COPY(qSlicerEMSegmentRunSegmentationStep);

};

#endif
