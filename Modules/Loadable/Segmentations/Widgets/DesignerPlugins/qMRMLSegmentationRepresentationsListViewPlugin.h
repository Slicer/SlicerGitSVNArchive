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

==============================================================================*/

#ifndef __qMRMLSegmentationRepresentationsListViewPlugin_h
#define __qMRMLSegmentationRepresentationsListViewPlugin_h

#include "qSlicerSegmentationsModuleWidgetsAbstractPlugin.h"

class Q_SLICER_MODULE_SEGMENTATIONS_WIDGETS_PLUGINS_EXPORT qMRMLSegmentationRepresentationsListViewPlugin
  : public QObject
  , public qSlicerSegmentationsModuleWidgetsAbstractPlugin
{
  Q_OBJECT

public:
  qMRMLSegmentationRepresentationsListViewPlugin(QObject* parent = 0);

  QWidget *createWidget(QWidget* parent);
  QString  domXml() const;
  QString  includeFile() const;
  bool     isContainer() const;
  QString  name() const;

};

#endif
