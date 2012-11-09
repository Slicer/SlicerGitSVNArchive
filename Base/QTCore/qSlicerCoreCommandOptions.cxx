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

// Qt includes
#include <QDebug>
#include <QDir>
#include <QSettings>

// CTK includes
#include <ctkPimpl.h>

// SlicerQt includes
#include "qSlicerCoreCommandOptions.h"
#include "qSlicerCoreApplication.h" // For disableCurrentSettings()

//-----------------------------------------------------------------------------
class qSlicerCoreCommandOptionsPrivate
{
  Q_DECLARE_PUBLIC(qSlicerCoreCommandOptions);
protected:
  qSlicerCoreCommandOptions* q_ptr;
public:
  qSlicerCoreCommandOptionsPrivate(qSlicerCoreCommandOptions& object);

  void init();

  QHash<QString, QVariant> ParsedArgs;
  QSettings                Settings;
  QString                  ExtraPythonScript;
  bool                     RunPythonAndExit;
};

//-----------------------------------------------------------------------------
// qSlicerCoreCommandOptionsPrivate methods

//-----------------------------------------------------------------------------
qSlicerCoreCommandOptionsPrivate::qSlicerCoreCommandOptionsPrivate(qSlicerCoreCommandOptions& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
void qSlicerCoreCommandOptionsPrivate::init()
{
  Q_Q(qSlicerCoreCommandOptions);
  this->RunPythonAndExit = false;
  q->setArgumentPrefix("--", "-"); // Use Unix-style argument names
  q->enableSettings("disable-settings"); // Enable QSettings support
}

//-----------------------------------------------------------------------------
// qSlicerCoreCommandOptions methods

//-----------------------------------------------------------------------------
qSlicerCoreCommandOptions::qSlicerCoreCommandOptions():Superclass()
, d_ptr(new qSlicerCoreCommandOptionsPrivate(*this))
{
  Q_D(qSlicerCoreCommandOptions);
  d->init();
}

//-----------------------------------------------------------------------------
qSlicerCoreCommandOptions::~qSlicerCoreCommandOptions()
{
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::parse(const QStringList& arguments)
{
  Q_D(qSlicerCoreCommandOptions);

  this->addArguments();

  bool ok = false;
  d->ParsedArgs = this->parseArguments(arguments, &ok);
  if (!ok)
    {
    return false;
    }

  // If first unparsed argument is python script, enable 'shebang' mode
  QStringList unparsedArguments = this->unparsedArguments();
  if (unparsedArguments.size() > 0 && unparsedArguments.at(0).endsWith(".py"))
    {
    if(!this->pythonScript().isEmpty())
      {
      qWarning() << "Ignore script specified using '--python-script'";
      }
    this->setExtraPythonScript(unparsedArguments.at(0));
    this->setRunPythonAndExit(true);
    }

  if (!d->ParsedArgs.value("c").toString().isEmpty())
    {
    this->setRunPythonAndExit(true);
    }

  return true;
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::displayHelpAndExit()const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("help").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::ignoreRest() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("ignore-rest").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::disableCLIModules() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("disable-cli-modules").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::ignoreSlicerRC()const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("ignore-slicerrc").toBool();
}

//-----------------------------------------------------------------------------
QStringList qSlicerCoreCommandOptions::additonalModulePaths()const
{
  Q_D(const qSlicerCoreCommandOptions);
  return QStringList()
      << d->ParsedArgs.value("additional-module-path").toString()
      << d->ParsedArgs.value("additional-module-paths").toStringList();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::disableLoadableModules() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("disable-loadable-modules").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::disableScriptedLoadableModules()const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("disable-scripted-loadable-modules").toBool();
}

//-----------------------------------------------------------------------------
QString qSlicerCoreCommandOptions::pythonScript() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("python-script").toString();
}

//-----------------------------------------------------------------------------
CTK_GET_CPP(qSlicerCoreCommandOptions, QString, extraPythonScript, ExtraPythonScript);
CTK_SET_CPP(qSlicerCoreCommandOptions, const QString&, setExtraPythonScript, ExtraPythonScript);

//-----------------------------------------------------------------------------
QString qSlicerCoreCommandOptions::pythonCode() const
{
  Q_D(const qSlicerCoreCommandOptions);
  QString pythonCode = d->ParsedArgs.value("python-code").toString();
  if(!pythonCode.isEmpty())
    {
    return pythonCode;
    }
  else
    {
    return d->ParsedArgs.value("c").toString();
    }
}

//-----------------------------------------------------------------------------
CTK_GET_CPP(qSlicerCoreCommandOptions, bool, runPythonAndExit, RunPythonAndExit);
CTK_SET_CPP(qSlicerCoreCommandOptions, bool, setRunPythonAndExit, RunPythonAndExit);

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::displayVersionAndExit() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("version").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::displayProgramPathAndExit() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("program-path").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::displayHomePathAndExit() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("home").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::displaySettingsPathAndExit() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("settings-path").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::verboseModuleDiscovery() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("verbose-module-discovery").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::disableMessageHandlers() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("disable-message-handlers").toBool();
}

//-----------------------------------------------------------------------------
QString qSlicerCoreCommandOptions::tempDirectory() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->Settings.value("temp-directory", QVariant(QDir::tempPath())).toString();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::settingsEnabled() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("disable-settings").toBool();
}

//-----------------------------------------------------------------------------
bool qSlicerCoreCommandOptions::isTestingEnabled() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs.value("testing").toBool();
}

//-----------------------------------------------------------------------------
void qSlicerCoreCommandOptions::addArguments()
{
  this->addArgument("ignore-rest", "-", QVariant::Bool,
                    "Ignores the rest of the labeled arguments following this flag.",
                    QVariant(false), true);

  this->addArgument("help", "h", QVariant::Bool,
                    "Display available command line arguments.");

  this->addArgument("testing", "", QVariant::Bool,
                    "Activate testing mode. Doesn't use user settings in that mode.",
                    QVariant(false));

#ifdef Slicer_USE_PYTHONQT
  this->addArgument("disable-python", "", QVariant::Bool,
                    "Disable python support. This is equivalent to build the application with Slicer_USE_PYTHONQT=OFF.");

  this->addArgument("python-script", "", QVariant::String,
                    "Python script to execute after slicer loads.");

  this->addArgument("python-code", "", QVariant::String,
                    "Python code to execute after slicer loads.");

  this->addArgument("", "c", QVariant::String,
                    "Python code to execute after slicer loads. By default, no modules are loaded and Slicer exits afterward.");

  this->addArgument("ignore-slicerrc", "", QVariant::Bool,
                    "Do not load the Slicer resource file (~/.slicerrc.py).");
#endif

  this->addArgument("additional-module-path", "", QVariant::String,
                    "Additional module path to consider when searching for modules to load.");

  this->addArgument("additional-module-paths", "", QVariant::StringList,
                    "List of additional module path to consider when searching for modules to load.");

#ifdef Slicer_BUILD_CLI_SUPPORT
  this->addArgument("disable-cli-modules", "", QVariant::Bool,
                    "Disables the loading of Command Line Modules.");
#endif

  this->addArgument("disable-loadable-modules", "", QVariant::Bool,
                    "Disables the loading of Loadable Modules.");

#ifdef Slicer_USE_PYTHONQT
  this->addArgument("disable-scripted-loadable-modules", "", QVariant::Bool,
                    "Disables the loading of Scripted Loadable Modules.");
#endif

  this->addArgument("version", "", QVariant::Bool,
                    "Displays version information and exits.");

  this->addArgument("program-path", "", QVariant::Bool,
                    "Displays application program path and exits.");

  this->addArgument("home", "", QVariant::Bool,
                    "Displays home path and exits.");

  this->addArgument("settings-path", "", QVariant::Bool,
                    "Displays settings path and exits.");

  this->addArgument("verbose-module-discovery", "", QVariant::Bool,
                    "Enable verbose output during module discovery process.");

  this->addArgument("disable-settings", "", QVariant::Bool,
                    "Start application ignoring user settings.");

  this->addArgument("disable-message-handlers", "", QVariant::Bool,
                    "Start application disabling the 'terminal' message handlers.");
}

//-----------------------------------------------------------------------------
QHash<QString, QVariant> qSlicerCoreCommandOptions::parsedArgs() const
{
  Q_D(const qSlicerCoreCommandOptions);
  return d->ParsedArgs;
}
