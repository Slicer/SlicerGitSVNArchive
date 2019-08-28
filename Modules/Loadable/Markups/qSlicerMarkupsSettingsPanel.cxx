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

// Qt includes
#include <QDebug>
#include <QSettings>

// QtGUI includes
#include "qSlicerApplication.h"
#include "qSlicerMarkupsSettingsPanel.h"
#include "ui_qSlicerMarkupsSettingsPanel.h"

// Markups Logic includes
#include <vtkSlicerMarkupsLogic.h>

// Markups MRML includes

// VTK includes
#include <vtkNew.h>
#include <vtkSmartPointer.h>

// --------------------------------------------------------------------------
// qSlicerMarkupsSettingsPanelPrivate

//-----------------------------------------------------------------------------
class qSlicerMarkupsSettingsPanelPrivate: public Ui_qSlicerMarkupsSettingsPanel
{
  Q_DECLARE_PUBLIC(qSlicerMarkupsSettingsPanel);
protected:
  qSlicerMarkupsSettingsPanel* const q_ptr;

public:
  qSlicerMarkupsSettingsPanelPrivate(qSlicerMarkupsSettingsPanel& object);
  void init();

  vtkSmartPointer<vtkSlicerMarkupsLogic> MarkupsLogic;
};

// --------------------------------------------------------------------------
// qSlicerMarkupsSettingsPanelPrivate methods

// --------------------------------------------------------------------------
qSlicerMarkupsSettingsPanelPrivate
::qSlicerMarkupsSettingsPanelPrivate(qSlicerMarkupsSettingsPanel& object)
  :q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanelPrivate::init()
{
  Q_Q(qSlicerMarkupsSettingsPanel);

  this->setupUi(q);
}



// --------------------------------------------------------------------------
// qSlicerMarkupsSettingsPanel methods

// --------------------------------------------------------------------------
qSlicerMarkupsSettingsPanel::qSlicerMarkupsSettingsPanel(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerMarkupsSettingsPanelPrivate(*this))
{
  Q_D(qSlicerMarkupsSettingsPanel);
  d->init();
}

// --------------------------------------------------------------------------
qSlicerMarkupsSettingsPanel::~qSlicerMarkupsSettingsPanel()
= default;

// --------------------------------------------------------------------------
vtkSlicerMarkupsLogic* qSlicerMarkupsSettingsPanel
::markupsLogic()const
{
  Q_D(const qSlicerMarkupsSettingsPanel);
  return d->MarkupsLogic;
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel
::setMarkupsLogic(vtkSlicerMarkupsLogic* logic)
{
  Q_D(qSlicerMarkupsSettingsPanel);

  qvtkReconnect(d->MarkupsLogic, logic, vtkCommand::ModifiedEvent,
                this, SLOT(onMarkupsLogicModified()));
  d->MarkupsLogic = logic;

  this->onMarkupsLogicModified();

  this->registerProperty("Markups/GlyphType", this,
                         "defaultGlyphType", SIGNAL(defaultGlyphTypeChanged(QString)));
  this->registerProperty("Markups/SelectedColor", this,
                         "defaultSelectedColor", SIGNAL(defaultSelectedColorChanged(QColor)));
  this->registerProperty("Markups/UnselectedColor", this,
                         "defaultUnselectedColor", SIGNAL(defaultUnselectedColorChanged(QColor)));
  this->registerProperty("Markups/GlyphScale", this,
                         "defaultGlyphScale", SIGNAL(defaultGlyphScaleChanged(double)));
  this->registerProperty("Markups/TextScale", this,
                         "defaultTextScale", SIGNAL(defaultTextScaleChanged(double)));
  this->registerProperty("Markups/Opacity", this,
                         "defaultOpacity", SIGNAL(defaultOpacityChanged(double)));
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel
::onMarkupsLogicModified()
{
  Q_D(qSlicerMarkupsSettingsPanel);
  this->readDefaultMarkupsDisplaySettings();

}

//-----------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::readDefaultMarkupsDisplaySettings()
{
  QSettings settings;
  if (settings.contains("Markups/GlyphType"))
    {
      setDefaultGlyphType(settings.value("Markups/GlyphType").toString().toLatin1());
    }
  if (settings.contains("Markups/SelectedColor"))
    {
      QVariant variant = settings.value("Markups/SelectedColor");
      QColor qcolor = variant.value<QColor>();
      setDefaultSelectedColor(qcolor);
    }
  if (settings.contains("Markups/UnselectedColor"))
    {
      QVariant variant = settings.value("Markups/UnselectedColor");
      QColor qcolor = variant.value<QColor>();
      setDefaultUnselectedColor(qcolor);
    }
  if (settings.contains("Markups/GlyphScale"))
    {
      setDefaultGlyphScale(settings.value("Markups/GlyphScale").toDouble());
    }
  if (settings.contains("Markups/TextScale"))
    {
      setDefaultTextScale(settings.value("Markups/TextScale").toDouble());
    }
  if (settings.contains("Markups/Opacity"))
    {
      setDefaultOpacity(settings.value("Markups/Opacity").toDouble());
    }

// --------------------------------------------------------------------------
QString qSlicerMarkupsSettingsPanel::defaultGlyphType()const
{
  Q_D(const qSlicerMarkupsSettingsPanel);

  int currentIndex  = d->defaultGlyphTypeComboBox->currentIndex();
  QString glyphType;
  if (currentIndex != -1)
    {
    glyphType =
      d->defaultGlyphTypeComboBox->itemText(currentIndex);
    }
  return glyphType;
}

// --------------------------------------------------------------------------
QColor qSlicerMarkupsSettingsPanel::defaultUnselectedColor()const
{
  Q_D(const qSlicerMarkupsSettingsPanel);

  QColor color = d->defaultUnselectedColorPickerButton->color();

  return color;
}

// --------------------------------------------------------------------------
QColor qSlicerMarkupsSettingsPanel::defaultSelectedColor()const
{
  Q_D(const qSlicerMarkupsSettingsPanel);

  QColor color = d->defaultSelectedColorPickerButton->color();

  return color;
}

// --------------------------------------------------------------------------
double qSlicerMarkupsSettingsPanel::defaultGlyphScale()const
{
  Q_D(const qSlicerMarkupsSettingsPanel);

  double glyphScale = d->defaultGlyphScaleSliderWidget->value();

  return glyphScale;
}

// --------------------------------------------------------------------------
double qSlicerMarkupsSettingsPanel::defaultTextScale()const
{
  Q_D(const qSlicerMarkupsSettingsPanel);

  double textScale = d->defaultTextScaleSliderWidget->value();

  return textScale;
}

// --------------------------------------------------------------------------
double qSlicerMarkupsSettingsPanel::defaultOpacity()const
{
  Q_D(const qSlicerMarkupsSettingsPanel);

  double opacity = d->defaultOpacitySliderWidget->value();

  return opacity;
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::setDefaultGlyphType(const QString& glyphType)
{
  Q_D(qSlicerMarkupsSettingsPanel);

  int glyphTypeIndex = d->defaultGlyphTypeComboBox->findData(glyphType);

  if (glyphTypeIndex != -1)
    {
    d->defaultGlyphTypeComboBox->setCurrentIndex(glyphTypeIndex);
    }
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::setDefaultUnselectedColor(const QColor color)
{
  Q_D(qSlicerMarkupsSettingsPanel);

  d->defaultUnselectedColorPickerButton->setColor(color);
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::setDefaultSelectedColor(const QColor color)
{
  Q_D(qSlicerMarkupsSettingsPanel);

  d->defaultSelectedColorPickerButton->setColor(color);
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::setDefaultGlyphScale(const double glyphScale)
{
  Q_D(qSlicerMarkupsSettingsPanel);

  d->defaultGlyphScaleSliderWidget->setValue(glyphScale);
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::setDefaultTextScale(const double glyphScale)
{
  Q_D(qSlicerMarkupsSettingsPanel);

  d->defaultTextScaleSliderWidget->setValue(glyphScale);
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::setDefaultOpacity(const double opacity)
{
  Q_D(qSlicerMarkupsSettingsPanel);

  d->defaultOpacitySliderWidget->setValue(opacity);
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::onDefaultGlyphTypeChanged(int index)
{
//   Q_D(qSlicerMarkupsSettingsPanel);
  Q_UNUSED(index);

  this->updateMarkupsLogicDefaultGlyphType();
  emit defaultGlyphTypeChanged(this->defaultGlyphType());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::updateMarkupsLogicDefaultGlyphType()
{
  Q_D(qSlicerMarkupsSettingsPanel);

  if (d->MarkupsLogic == nullptr)
    {
    return;
    }
  // disable it for now; if we want a settings panel then use the same pattern that is used for default view options
  // d->MarkupsLogic->SetDefaultMarkupsDisplayNodeGlyphTypeFromString(this->defaultGlyphType().toLatin1());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::onDefaultUnselectedColorChanged(QColor color)
{
  Q_UNUSED(color);
  this->updateMarkupsLogicDefaultUnselectedColor();
  emit defaultUnselectedColorChanged(this->defaultUnselectedColor());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::updateMarkupsLogicDefaultUnselectedColor()
{
  Q_D(qSlicerMarkupsSettingsPanel);

  QColor qcolor = this->defaultUnselectedColor();

  double color[3];

  color[0] = qcolor.redF();
  color[1] = qcolor.greenF();
  color[2] = qcolor.blueF();

  // disable it for now; if we want a settings panel then use the same pattern that is used for default view options
  // d->MarkupsLogic->SetDefaultMarkupsDisplayNodeColor(color);
  Q_UNUSED(color);
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::onDefaultSelectedColorChanged(QColor color)
{
  Q_UNUSED(color);
  this->updateMarkupsLogicDefaultSelectedColor();
  emit defaultSelectedColorChanged(this->defaultSelectedColor());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::updateMarkupsLogicDefaultSelectedColor()
{
  Q_D(qSlicerMarkupsSettingsPanel);

  QColor qcolor = this->defaultSelectedColor();

  double color[3];

  color[0] = qcolor.redF();
  color[1] = qcolor.greenF();
  color[2] = qcolor.blueF();

  // disable it for now; if we want a settings panel then use the same pattern that is used for default view options
  // d->MarkupsLogic->SetDefaultMarkupsDisplayNodeSelectedColor(color);
  Q_UNUSED(color);
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::onDefaultGlyphScaleChanged(double scale)
{
  Q_UNUSED(scale);
  this->updateMarkupsLogicDefaultGlyphScale();
  emit defaultGlyphScaleChanged(this->defaultGlyphScale());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::updateMarkupsLogicDefaultGlyphScale()
{
  Q_D(qSlicerMarkupsSettingsPanel);

  if (d->MarkupsLogic == nullptr)
    {
    return;
    }
  // disable it for now; if we want a settings panel then use the same pattern that is used for default view options
  // d->MarkupsLogic->SetDefaultMarkupsDisplayNodeGlyphScale(this->defaultGlyphScale());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::onDefaultTextScaleChanged(double scale)
{
  Q_UNUSED(scale);
  this->updateMarkupsLogicDefaultTextScale();
  emit defaultTextScaleChanged(this->defaultTextScale());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::updateMarkupsLogicDefaultTextScale()
{
  Q_D(qSlicerMarkupsSettingsPanel);

  if (d->MarkupsLogic == nullptr)
    {
    return;
    }
  // disable it for now; if we want a settings panel then use the same pattern that is used for default view options
  // d->MarkupsLogic->SetDefaultMarkupsDisplayNodeTextScale(this->defaultTextScale());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::onDefaultOpacityChanged(double opacity)
{
  Q_UNUSED(opacity);
  this->updateMarkupsLogicDefaultOpacity();
  emit defaultOpacityChanged(this->defaultOpacity());
}

// --------------------------------------------------------------------------
void qSlicerMarkupsSettingsPanel::updateMarkupsLogicDefaultOpacity()
{
  Q_D(qSlicerMarkupsSettingsPanel);

  if (d->MarkupsLogic == nullptr)
    {
    return;
    }
  // disable it for now; if we want a settings panel then use the same pattern that is used for default view options
  // d->MarkupsLogic->SetDefaultMarkupsDisplayNodeOpacity(this->defaultOpacity());
}
