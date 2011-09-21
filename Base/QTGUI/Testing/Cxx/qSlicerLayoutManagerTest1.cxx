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

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QApplication>

// SlicerQt includes
#include "qSlicerLayoutManager.h"

// MRML includes

// STD includes
#include <cstdlib>

int qSlicerLayoutManagerTest1(int argc, char * argv[] )
{
  QApplication app(argc, argv);
  qSlicerLayoutManager* layoutManager = new qSlicerLayoutManager(0);
  delete layoutManager;
  return EXIT_SUCCESS;
}

