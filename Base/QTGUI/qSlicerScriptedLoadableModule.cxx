/*==============================================================================

  Program: 3D Slicer

  Copyright (c) 2010 Kitware Inc.

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

// Qt includes
#include <QFileInfo>

// CTK includes

// PythonQt includes
#include <PythonQt.h>

// Slicer includes
#include "qSlicerScriptedLoadableModule.h"
#include "qSlicerScriptedLoadableModuleWidget.h"
#include "vtkSlicerScriptedLoadableModuleLogic.h"

// VTK includes

// Python includes

//-----------------------------------------------------------------------------
class qSlicerScriptedLoadableModulePrivate
{
public:
  qSlicerScriptedLoadableModulePrivate();
  virtual ~qSlicerScriptedLoadableModulePrivate();
  
  QString Title;
  QString Category;
  QString Contributor;
  QString HelpText;
  QString AcknowledgementText;
  QIcon   Icon;
  bool   Hidden;
  QVariantMap   Extensions;
  
  PyObject * PythonSelf;
  QString    PythonSource;
};

//-----------------------------------------------------------------------------
// qSlicerScriptedLoadableModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerScriptedLoadableModulePrivate::qSlicerScriptedLoadableModulePrivate()
{
  this->PythonSelf = 0;
  this->Hidden = false;
}

//-----------------------------------------------------------------------------
qSlicerScriptedLoadableModulePrivate::~qSlicerScriptedLoadableModulePrivate()
{
  if (this->PythonSelf)
    {
    Py_DECREF(this->PythonSelf);
    }
}


//-----------------------------------------------------------------------------
// qSlicerScriptedLoadableModule methods

//-----------------------------------------------------------------------------
qSlicerScriptedLoadableModule::qSlicerScriptedLoadableModule(QObject* _parentObject)
  : Superclass(_parentObject)
  , d_ptr(new qSlicerScriptedLoadableModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerScriptedLoadableModule::~qSlicerScriptedLoadableModule()
{
}

//-----------------------------------------------------------------------------
bool qSlicerScriptedLoadableModule::setPythonSource(const QString& newPythonSource)
{
  Q_D(qSlicerScriptedLoadableModule);

  Q_ASSERT(newPythonSource.endsWith(".py"));

  // Open the file
#ifdef _WIN32
  FILE* pyfile = PyRun_OpenFile(newPythonSource.toLatin1());
#else
  FILE* pyfile = fopen(newPythonSource.toLatin1(), "r");
#endif
  if (!pyfile)
    {
    qCritical() << "SetPythonSource - File" << newPythonSource << "doesn't exist !";
    return false;
    }

  // Extract moduleName from the provided filename
  QString moduleName = QFileInfo(newPythonSource).baseName();
  QString className = moduleName;
  //qDebug() << "className" << className;

  // Get a reference to the main module and global dictionary
  PyObject * main_module = PyImport_AddModule("__main__");
  PyObject * global_dict = PyModule_GetDict(main_module);

  // Load class definition if needed
  PyObject * classToInstantiate = PyDict_GetItemString(global_dict, className.toLatin1());
  if (!classToInstantiate)
    {
    PyDict_SetItemString(global_dict, "__name__", PyString_FromString(className.toLatin1()));
    PyRun_File(pyfile, newPythonSource.toLatin1(), Py_file_input, global_dict, global_dict);
    classToInstantiate = PyDict_GetItemString(global_dict, className.toLatin1());
    PyDict_SetItemString(global_dict, "__name__", PyString_FromString("__main__"));
    }
#ifdef _WIN32
  PyRun_CloseFile(pyfile);
#else
  fclose(pyfile);
#endif

  if (!classToInstantiate)
    {
    qCritical()
        << "Failed to load scripted pythonqt module class definition"
        << className << "from" << newPythonSource;
    return false;
    }

  //qDebug() << "classToInstantiate:" << classToInstantiate;

  PyObject * wrappedThis = PythonQt::self()->priv()->wrapQObject(this);
  if (!wrappedThis)
    {
    qCritical() << "setPythonSource" << newPythonSource
        << "- Failed to wrap" << this->metaObject()->className();
    return false;
    }

  PyObject * arguments = PyTuple_New(1);
  PyTuple_SET_ITEM(arguments, 0, wrappedThis);

  // Attempt to instantiate the associated python class
  PyObject * self = PyInstance_New(classToInstantiate, arguments, 0);
  Py_DECREF(arguments);
  if (!self)
    {
    qCritical() << "setPythonSource" << newPythonSource
        << "- Failed to instantiate scripted pythonqt class" << className << classToInstantiate;
    return false;
    }

  d->PythonSource = newPythonSource;
  d->PythonSelf = self;

  return true;
}

//-----------------------------------------------------------------------------
//PyObject* qSlicerScriptedLoadableModule::pythonInstance() const
//{
//  Q_D(const qSlicerScriptedLoadableModule);
//  return d->PythonSelf;
//}

//-----------------------------------------------------------------------------
void qSlicerScriptedLoadableModule::setup()
{
  #ifndef QT_NO_DEBUG
  Q_D(qSlicerScriptedLoadableModule);
  #endif
  Q_ASSERT(d != 0);
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation* qSlicerScriptedLoadableModule::createWidgetRepresentation()
{
  Q_D(qSlicerScriptedLoadableModule);

  QScopedPointer<qSlicerScriptedLoadableModuleWidget> widget(new qSlicerScriptedLoadableModuleWidget);
  bool ret = widget->setPythonSource(d->PythonSource);
  if (!ret)
    {
    return 0;
    }

  return widget.take();
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerScriptedLoadableModule::createLogic()
{
//  Q_D(qSlicerScriptedLoadableModule);
  
  vtkSlicerScriptedLoadableModuleLogic* logic = vtkSlicerScriptedLoadableModuleLogic::New();

//  bool ret = logic->SetPythonSource(d->PythonSource.toStdString());
//  if (!ret)
//    {
//    logic->Delete();
//    return 0;
//    }

  return logic;
}

//-----------------------------------------------------------------------------
CTK_SET_CPP(qSlicerScriptedLoadableModule, const QString&, setTitle, Title)
CTK_GET_CPP(qSlicerScriptedLoadableModule, QString, title, Title)

//-----------------------------------------------------------------------------
CTK_SET_CPP(qSlicerScriptedLoadableModule, const QString&, setCategory, Category)
CTK_GET_CPP(qSlicerScriptedLoadableModule, QString, category, Category)

//-----------------------------------------------------------------------------
CTK_SET_CPP(qSlicerScriptedLoadableModule, const QString&, setContributor, Contributor)
CTK_GET_CPP(qSlicerScriptedLoadableModule, QString, contributor, Contributor)

//-----------------------------------------------------------------------------
CTK_SET_CPP(qSlicerScriptedLoadableModule, const QString&, setHelpText, HelpText)
CTK_GET_CPP(qSlicerScriptedLoadableModule, QString, helpText, HelpText)

//-----------------------------------------------------------------------------
CTK_SET_CPP(qSlicerScriptedLoadableModule, const QString&, setAcknowledgementText, AcknowledgementText)
CTK_GET_CPP(qSlicerScriptedLoadableModule, QString, acknowledgementText, AcknowledgementText)

//-----------------------------------------------------------------------------
CTK_SET_CPP(qSlicerScriptedLoadableModule, const QVariantMap&, setExtensions, Extensions)
CTK_GET_CPP(qSlicerScriptedLoadableModule, QVariantMap, extensions, Extensions)

//-----------------------------------------------------------------------------
CTK_SET_CPP(qSlicerScriptedLoadableModule, const QIcon&, setIcon, Icon)
CTK_GET_CPP(qSlicerScriptedLoadableModule, QIcon, icon, Icon)

//-----------------------------------------------------------------------------
CTK_SET_CPP(qSlicerScriptedLoadableModule, bool, setHidden, Hidden)
CTK_GET_CPP(qSlicerScriptedLoadableModule, bool, isHidden, Hidden)
