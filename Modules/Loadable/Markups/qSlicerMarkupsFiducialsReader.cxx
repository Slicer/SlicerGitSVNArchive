/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// SlicerQt includes
#include "qSlicerMarkupsFiducialsReader.h"

// Logic includes
#include "vtkSlicerMarkupsLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Annotations
//-----------------------------------------------------------------------------
qSlicerMarkupsFiducialsReader::qSlicerMarkupsFiducialsReader(QObject* _parent)
  : Superclass(_parent)
{
}

//-----------------------------------------------------------------------------
qSlicerMarkupsFiducialsReader
::qSlicerMarkupsFiducialsReader(vtkSlicerMarkupsLogic* logic, QObject* _parent)
  : Superclass(logic, _parent)
{
}

//-----------------------------------------------------------------------------
qSlicerMarkupsFiducialsReader::~qSlicerMarkupsFiducialsReader()
{
}

//-----------------------------------------------------------------------------
QString qSlicerMarkupsFiducialsReader::description()const
{
  return "MarkupsFiducials";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerMarkupsFiducialsReader::fileType()const
{
  return QString("MarkupsFiducials");
}

//-----------------------------------------------------------------------------
QStringList qSlicerMarkupsFiducialsReader::extensions()const
{
  return QStringList()
    << "Markups Fiducials (*.fcsv)"
    << " Annotation Fiducial (*.acsv)";
}

//-----------------------------------------------------------------------------
char* qSlicerMarkupsFiducialsReader
::load(const QString& filename, const QString& name)
{
  return this->markupsLogic()->LoadMarkupsFiducials(
    filename.toLatin1(), name.toLatin1());
}
