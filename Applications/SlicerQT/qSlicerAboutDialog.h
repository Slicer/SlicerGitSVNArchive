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

#ifndef __qSlicerAboutDialog_h
#define __qSlicerAboutDialog_h

// Qt includes
#include <QDialog>

// CTK includes
#include <ctkPimpl.h>

// SlicerQt includes
#include "qSlicerQTExport.h"

class qSlicerAboutDialogPrivate;

/// Pre-request that a qSlicerApplication has been instanced
class Q_SLICERQT_EXPORT qSlicerAboutDialog :
  public QDialog
{
  Q_OBJECT
public:
  qSlicerAboutDialog(QWidget *parentWidget = 0);
  virtual ~qSlicerAboutDialog();

protected:
  QScopedPointer<qSlicerAboutDialogPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerAboutDialog);
  Q_DISABLE_COPY(qSlicerAboutDialog);
};

#endif
