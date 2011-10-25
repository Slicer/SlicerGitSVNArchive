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

#ifndef __qSlicerUtils_h
#define __qSlicerUtils_h

#include <QString>

#include "qSlicerBaseQTCoreExport.h"

class Q_SLICER_BASE_QTCORE_EXPORT qSlicerUtils
{

public:
  typedef qSlicerUtils Self;

  ///
  /// Return true if the string name ends with one of these executable extension:
  /// ".bat", ".com", ".sh", ".csh", ".tcsh", ".pl", ".py", ".tcl", ".m", ".exe"
  /// \note The comparison is case insensitive
  static bool isExecutableName(const QString& name);

  /// Returns \a true if the \a filePath matches the CLI executable file name requirements
  static bool isCLIExecutable(const QString& filePath);

  /// Returns \a true if the \a filePath matches the CLI loadable module file name requirements.
  /// \note Associated \a fileName is expected to match the following
  /// regular expression: "(lib)?.+Lib\\.(dll|DLL|so|dylib)"
  static bool isCLILoadableModule(const QString& filePath);

  /// Return \a true if \a filePath matches the loadable module file name requirements.
  /// \note Associated \a fileName is expected to match the following
  /// regular expression: "(lib)?qSlicer.+Module\\.(so, dll, dylib)"
  static bool isLoadableModule(const QString& filePath);

  ///
  /// Look for target file in build intermediate directory.
  /// On windows, the intermediate directory includes: . Debug RelWithDebInfo Release MinSizeRel
  /// And it return the first matched directory
  /// On the other plateform, this function just return the directory passed as a first argument
  static QString searchTargetInIntDir(const QString& directory, const QString& target);

  /// This function returns an empty string on all plateform expected windows
  /// where it returns ".exe"
  static QString executableExtension();

  /// This function returns ".dll. on windows, ".so" on linux and ".dylib" on Mac
  //static QString libraryExtension();

  /// Extract module name given a library name
  /// For example:
  ///  on linux, libThresholdLib.so -> Threshold
  ///  on mac, libThresholdLib.{dylib, bundle, so} -> Threshold
  ///  on windows, ThresholdLib.dll -> Threshold
  static QString extractModuleNameFromLibraryName(const QString& libraryName);

  /// Extract module name givew a \a className
  /// For example:
  ///  qSlicerThresholdModule -> Threshold
  static QString extractModuleNameFromClassName(const QString& className);

  /// Return \a true if the plugin identified with its \a filePath is loaded from an install tree.
  /// \warning Since internally the function looks for the existence of CMakeCache.txt, it will
  /// return an incorrect result if the plugin is installed in the build tree of
  /// an other project.
  static bool isPluginInstalled(const QString& filePath, const QString& applicationHomeDir);

  /// Return the path without the intermediate directory or return \a path if there is no
  /// expected "IntDir".
  /// \a subDirWithoutIntDir corresponds to N last compononent of the path excluding
  /// the "IntDir".
  /// \example
  /// <table border="1">
  ///   <tr>
  ///     <td><b>Path</b></td>
  ///     <td><b>subDirWithoutIntDir</b></td>
  ///     <td><b>Return</b></td>
  ///     <td><b>IntDir</b></td>
  ///   </tr>
  ///   <tr>
  ///     <td>/path/to/lib/module/Foo</td>
  ///     <td>lib/module</td>
  ///     <td>/path/to/lib/module</td>
  ///     <td>Foo</td>
  ///   </tr>
  ///   <tr>
  ///     <td>/path/to/lib/module/Release</td>
  ///     <td>lib/module</td>
  ///     <td>/path/to/lib/module</td>
  ///     <td>Release</td>
  ///   </tr>
  ///   <tr>
  ///     <td>/path/to/lib/module/Release/</td>
  ///     <td>lib/module</td>
  ///     <td>/path/to/lib/module</td>
  ///     <td>Release</td>
  ///   </tr>
  ///   <tr>
  ///     <td>/path/to/lib/module/Release</td>
  ///     <td>module</td>
  ///     <td>/path/to/lib/module</td>
  ///     <td>Release</td>
  ///   </tr>
  ///   <tr>
  ///     <td>/path/to/lib/module/Release</td>
  ///     <td><i>(Empty string)</i></td>
  ///     <td>/path/to/lib/module/Release</td>
  ///     <td><i>(Empty string)</i></td>
  ///   </tr>
  ///   <tr>
  ///     <td>/path/to/lib/module/Release/foo.txt</td>
  ///     <td>lib/module</td>
  ///     <td>/path/to/lib/module/Release</td>
  ///     <td>foo.txt</td>
  ///   </tr>
  ///   <tr>
  ///     <td>/path/to/lib/module/Release</td>
  ///     <td>/path/to/lib/module</td>
  ///     <td>/path/to/lib/module</td>
  ///     <td>Release</td>
  ///   </tr>
  ///   <tr>
  ///     <td>/path/to/Foo.app/Contents/MacOSX</td>
  ///     <td>bin</td>
  ///     <td>/path/to/Foo.app/Contents/MacOSX</td>
  ///     <td><i>(Empty string)</i></td>
  ///   </tr>
  /// </table>
  static QString pathWithoutIntDir(const QString& path, const QString& subDirWithoutIntDir);
  static QString pathWithoutIntDir(const QString& path, const QString& subDirWithoutIntDir, QString& intDir);

  /// Return \a true if the \a path ends with \a relativePath
  static bool pathEndsWith(const QString& path, const QString& relativePath);

private:
  /// Not implemented
  qSlicerUtils(){}
  virtual ~qSlicerUtils(){}

};

#endif
