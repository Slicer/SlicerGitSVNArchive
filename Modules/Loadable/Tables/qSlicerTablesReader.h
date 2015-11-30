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

  This file was originally developed by Kevin Wang, PMH.
  and was partially funded by OCAIRO and Sparkit.

==============================================================================*/

#ifndef __qSlicerTablesReader
#define __qSlicerTablesReader

// SlicerQt includes
#include "qSlicerFileReader.h"

class qSlicerTablesReaderPrivate;
class vtkSlicerTablesLogic;

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_DoubleArray
class qSlicerTablesReader
  : public qSlicerFileReader
{
  Q_OBJECT
public:
  typedef qSlicerFileReader Superclass;
  qSlicerTablesReader(QObject* parent = 0);
  qSlicerTablesReader(vtkSlicerTablesLogic* logic,
                       QObject* parent = 0);
  virtual ~qSlicerTablesReader();

  vtkSlicerTablesLogic* logic()const;
  void setLogic(vtkSlicerTablesLogic* logic);

  virtual QString description()const;
  virtual IOFileType fileType()const;
  virtual QStringList extensions()const;

  virtual bool load(const IOProperties& properties);
protected:
  QScopedPointer<qSlicerTablesReaderPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerTablesReader);
  Q_DISABLE_COPY(qSlicerTablesReader);
};

#endif
