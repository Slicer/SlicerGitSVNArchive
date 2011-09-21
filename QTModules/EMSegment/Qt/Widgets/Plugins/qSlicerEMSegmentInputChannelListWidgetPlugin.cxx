/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

#include "qSlicerEMSegmentInputChannelListWidgetPlugin.h"
#include "qSlicerEMSegmentInputChannelListWidget.h"

//-----------------------------------------------------------------------------
qSlicerEMSegmentInputChannelListWidgetPlugin::
  qSlicerEMSegmentInputChannelListWidgetPlugin(QObject *newParent) : QObject(newParent)
{
}

//-----------------------------------------------------------------------------
QWidget *qSlicerEMSegmentInputChannelListWidgetPlugin::createWidget(QWidget *newParent)
{
  qSlicerEMSegmentInputChannelListWidget* _widget = 
    new qSlicerEMSegmentInputChannelListWidget(newParent);
  return _widget;
}

//-----------------------------------------------------------------------------
QString qSlicerEMSegmentInputChannelListWidgetPlugin::domXml() const
{
  return "<widget class=\"qSlicerEMSegmentInputChannelListWidget\" \
          name=\"EMSegmentInputChannelListWidget\">\n"
          " <property name=\"geometry\">\n"
          "  <rect>\n"
          "   <x>0</x>\n"
          "   <y>0</y>\n"
          "   <width>200</width>\n"
          "   <height>200</height>\n"
          "  </rect>\n"
          " </property>\n"
          "</widget>\n";
}

//-----------------------------------------------------------------------------
QIcon qSlicerEMSegmentInputChannelListWidgetPlugin::icon() const
{
  return QIcon(":/Icons/table.png");
}

//-----------------------------------------------------------------------------
QString qSlicerEMSegmentInputChannelListWidgetPlugin::includeFile() const
{
  return "qSlicerEMSegmentInputChannelListWidget.h";
}

//-----------------------------------------------------------------------------
bool qSlicerEMSegmentInputChannelListWidgetPlugin::isContainer() const
{
  return false;
}

//-----------------------------------------------------------------------------
QString qSlicerEMSegmentInputChannelListWidgetPlugin::name() const
{
  return "qSlicerEMSegmentInputChannelListWidget";
}
