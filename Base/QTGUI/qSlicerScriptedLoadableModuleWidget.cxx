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
#include <QVBoxLayout>

// PythonQt includes
#include <PythonQt.h>

// SlicerQt includes
#include "qSlicerScriptedLoadableModuleWidget.h"

// Python includes

//-----------------------------------------------------------------------------
class qSlicerScriptedLoadableModuleWidgetPrivate
{
public:
  typedef qSlicerScriptedLoadableModuleWidgetPrivate Self;
  qSlicerScriptedLoadableModuleWidgetPrivate();
  virtual ~qSlicerScriptedLoadableModuleWidgetPrivate();

  enum {
    SetupMethod = 0,
    EnterMethod,
    ExitMethod
    };

  static int          APIMethodCount;
  static const char * APIMethodNames[3];

  PyObject*  PythonAPIMethods[3];
  PyObject*  PythonSelf;
  QString    PythonSource;
};

//-----------------------------------------------------------------------------
// qSlicerScriptedLoadableModuleWidgetPrivate methods

//---------------------------------------------------------------------------
int qSlicerScriptedLoadableModuleWidgetPrivate::APIMethodCount = 3;

//---------------------------------------------------------------------------
const char* qSlicerScriptedLoadableModuleWidgetPrivate::APIMethodNames[3] =
{
  "setup",
  "enter",
  "exit"
};

//-----------------------------------------------------------------------------
qSlicerScriptedLoadableModuleWidgetPrivate::qSlicerScriptedLoadableModuleWidgetPrivate()
{
  this->PythonSelf = 0;
  for (int i = 0; i < Self::APIMethodCount; ++i)
    {
    this->PythonAPIMethods[i] = 0;
    }
}

//-----------------------------------------------------------------------------
qSlicerScriptedLoadableModuleWidgetPrivate::~qSlicerScriptedLoadableModuleWidgetPrivate()
{
  if (this->PythonSelf)
    {
    Py_DECREF(this->PythonSelf);
    }
}

//-----------------------------------------------------------------------------
// qSlicerScriptedLoadableModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerScriptedLoadableModuleWidget::qSlicerScriptedLoadableModuleWidget(QWidget* parentWidget)
  :Superclass(parentWidget), d_ptr(new qSlicerScriptedLoadableModuleWidgetPrivate)
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  Q_ASSERT(layout);
  Q_UNUSED(layout);
}

//-----------------------------------------------------------------------------
qSlicerScriptedLoadableModuleWidget::~qSlicerScriptedLoadableModuleWidget()
{

}

//-----------------------------------------------------------------------------
bool qSlicerScriptedLoadableModuleWidget::setPythonSource(const QString& newPythonSource, const QString& className)
{
  Q_D(qSlicerScriptedLoadableModuleWidget);

  Q_ASSERT(newPythonSource.endsWith(".py"));

  // Open the file
  FILE* pyfile = fopen(newPythonSource.toLatin1(), "r");
  if (!pyfile)
    {
    qCritical() << "setPythonSource - File" << newPythonSource << "doesn't exist !";
    return false;
    }

  // Extract moduleName from the provided filename
  QString classNameToLoad = className;
  if (classNameToLoad.isEmpty())
    {
    QString moduleName = QFileInfo(newPythonSource).baseName();
    classNameToLoad = moduleName + "Widget";
    }
  //qDebug() << "classNameToLoad" << classNameToLoad;

  // Get a reference to the main module and global dictionary
  PyObject * main_module = PyImport_AddModule("__main__");
  PyObject * global_dict = PyModule_GetDict(main_module);

  // Load class definition if needed
  PyObject * classToInstantiate = PyDict_GetItemString(global_dict, classNameToLoad.toLatin1());
  if (!classToInstantiate)
    {
    PyRun_File(pyfile, newPythonSource.toLatin1(), Py_file_input, global_dict, global_dict);
    classToInstantiate = PyDict_GetItemString(global_dict, classNameToLoad.toLatin1());
    }

  if (!classToInstantiate)
    {
    PyErr_Print();
    PyErr_SetString(PyExc_RuntimeError,
                    QString("setPythonSource - Failed to load scripted pythonqt module class definition"
                            " %1 from %2").arg(classNameToLoad).arg(newPythonSource).toLatin1());
    PyErr_Print();
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
        << " - Failed to instantiate scripted pythonqt class"
        << classNameToLoad << classToInstantiate;
    return false;
    }

  // Retrieve API methods
  for (int i = 0; i < Pimpl::APIMethodCount; ++i)
    {
    Q_ASSERT(Pimpl::APIMethodNames[i]);
    PyObject * method = PyObject_GetAttrString(self, Pimpl::APIMethodNames[i]);
    //qDebug() << "method:" << method;
    d->PythonAPIMethods[i] = method;
    }

  d->PythonSource = newPythonSource;
  d->PythonSelf = self;

  return true;
}

//-----------------------------------------------------------------------------
void qSlicerScriptedLoadableModuleWidget::setup()
{
  Q_D(qSlicerScriptedLoadableModuleWidget);
  PyObject * method = d->PythonAPIMethods[Pimpl::SetupMethod];
  if (!method)
    {
    return;
    }
  PyObject_CallObject(method, 0);
}

//-----------------------------------------------------------------------------
void qSlicerScriptedLoadableModuleWidget::enter()
{
  Q_D(qSlicerScriptedLoadableModuleWidget);
  PyObject * method = d->PythonAPIMethods[Pimpl::EnterMethod];
  if (!method)
    {
    return;
    }
  PyObject_CallObject(method, 0);
}

//-----------------------------------------------------------------------------
void qSlicerScriptedLoadableModuleWidget::exit()
{
  Q_D(qSlicerScriptedLoadableModuleWidget);
  PyObject * method = d->PythonAPIMethods[Pimpl::ExitMethod];
  if (!method)
    {
    return;
    }
  PyObject_CallObject(method, 0);
}
