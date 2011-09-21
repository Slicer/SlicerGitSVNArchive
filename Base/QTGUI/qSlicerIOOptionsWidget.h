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

#ifndef __qSlicerIOOptionsWidget_h
#define __qSlicerIOOptionsWidget_h

/// QtCore includes
#include "qSlicerBaseQTGUIExport.h"
#include "qSlicerIOOptions.h"
#include "qSlicerWidget.h"

class Q_SLICER_BASE_QTGUI_EXPORT qSlicerIOOptionsWidget
  : public qSlicerWidget, public qSlicerIOOptions
{
  Q_OBJECT
public:
  explicit qSlicerIOOptionsWidget(QWidget* parent = 0);
  virtual ~qSlicerIOOptionsWidget();

  /// Returns true if the options have been set and if they are
  /// meaningful
  virtual bool isValid()const;

public slots:
  virtual void setFileName(const QString& fileName);
  virtual void setFileNames(const QStringList& fileNames);

signals:
  void validChanged(bool);
};

#endif
