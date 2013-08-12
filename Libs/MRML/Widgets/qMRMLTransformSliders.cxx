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

#include "qMRMLTransformSliders.h"
#include "ui_qMRMLTransformSliders.h"

// Qt includes
#include <QStack>

// qMRML includes
#include <qMRMLUtils.h>

// MRML includes
#include "vtkMRMLLinearTransformNode.h"

// VTK includes
#include <vtkNew.h>
#include <vtkTransform.h>


//-----------------------------------------------------------------------------
class qMRMLTransformSlidersPrivate: public Ui_qMRMLTransformSliders
{
public:
  qMRMLTransformSlidersPrivate()
    {
    this->MRMLTransformNode = 0;
    }

  qMRMLTransformSliders::TransformType   TypeOfTransform;
  vtkMRMLLinearTransformNode*            MRMLTransformNode;
  QStack<qMRMLLinearTransformSlider*>    ActiveSliders;
};

// --------------------------------------------------------------------------
qMRMLTransformSliders::qMRMLTransformSliders(QWidget* _parent) : Superclass(_parent)
  , d_ptr(new qMRMLTransformSlidersPrivate)
{
  Q_D(qMRMLTransformSliders);

  d->setupUi(this);
  d->LRSlider->spinBox()->setDecimalsOption(
    ctkDoubleSpinBox::DecimalsByShortcuts | ctkDoubleSpinBox::DecimalsByKey);
  d->PASlider->spinBox()->setDecimalsOption(
    ctkDoubleSpinBox::DecimalsByShortcuts | ctkDoubleSpinBox::DecimalsByKey);
  d->ISSlider->spinBox()->setDecimalsOption(
    ctkDoubleSpinBox::DecimalsByShortcuts | ctkDoubleSpinBox::DecimalsByKey);

  this->setCoordinateReference(qMRMLTransformSliders::GLOBAL);
  this->setTypeOfTransform(qMRMLTransformSliders::TRANSLATION);

  this->connect(d->LRSlider, SIGNAL(valueChanged(double)),
                SLOT(onSliderPositionChanged(double)));
  this->connect(d->PASlider, SIGNAL(valueChanged(double)),
                SLOT(onSliderPositionChanged(double)));
  this->connect(d->ISSlider, SIGNAL(valueChanged(double)),
                SLOT(onSliderPositionChanged(double)));

  this->connect(d->MinValueSpinBox, SIGNAL(valueChanged(double)),
                SLOT(onMinimumChanged(double)));
  this->connect(d->MaxValueSpinBox, SIGNAL(valueChanged(double)),
                SLOT(onMaximumChanged(double)));
  // the default values of min and max are set in the .ui file
  this->onMinimumChanged(d->MinValueSpinBox->value());
  this->onMaximumChanged(d->MaxValueSpinBox->value());

  this->connect(d->LRSlider, SIGNAL(decimalsChanged(int)),
                SIGNAL(decimalsChanged(int)));

  // disable as there is not MRML Node associated with the widget
  this->setEnabled(false);
}

// --------------------------------------------------------------------------
qMRMLTransformSliders::~qMRMLTransformSliders()
{
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setCoordinateReference(CoordinateReferenceType _coordinateReference)
{
  Q_D(qMRMLTransformSliders);

  qMRMLLinearTransformSlider::CoordinateReferenceType ref = qMRMLLinearTransformSlider::GLOBAL;
  if (_coordinateReference == LOCAL)
    {
    ref = qMRMLLinearTransformSlider::LOCAL;
    }
  d->LRSlider->setCoordinateReference(ref);
  d->PASlider->setCoordinateReference(ref);
  d->ISSlider->setCoordinateReference(ref);
}

// --------------------------------------------------------------------------
qMRMLTransformSliders::CoordinateReferenceType qMRMLTransformSliders::coordinateReference() const
{
  Q_D(const qMRMLTransformSliders);

  // Assumes settings of the sliders are all the same
  qMRMLLinearTransformSlider::CoordinateReferenceType ref =
    d->LRSlider->coordinateReference();
  return (ref == qMRMLLinearTransformSlider::GLOBAL) ? GLOBAL : LOCAL;
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setTypeOfTransform(TransformType _typeOfTransform)
{
  Q_D(qMRMLTransformSliders);

  if (d->TypeOfTransform == _typeOfTransform) { return; }
  if (_typeOfTransform == qMRMLTransformSliders::TRANSLATION)
    {
    d->LRSlider->setTypeOfTransform(qMRMLLinearTransformSlider::TRANSLATION_LR);
    d->PASlider->setTypeOfTransform(qMRMLLinearTransformSlider::TRANSLATION_PA);
    d->ISSlider->setTypeOfTransform(qMRMLLinearTransformSlider::TRANSLATION_IS);
    }
  else if (_typeOfTransform == qMRMLTransformSliders::ROTATION)
    {
    d->LRSlider->setTypeOfTransform(qMRMLLinearTransformSlider::ROTATION_LR);
    d->PASlider->setTypeOfTransform(qMRMLLinearTransformSlider::ROTATION_PA);
    d->ISSlider->setTypeOfTransform(qMRMLLinearTransformSlider::ROTATION_IS);

    // Range of Rotation sliders should be fixed to (-180,180)
    this->setRange(-180.00, 180.00);
    }
  d->TypeOfTransform = _typeOfTransform;
}

// --------------------------------------------------------------------------
qMRMLTransformSliders::TransformType qMRMLTransformSliders::typeOfTransform() const
{
  Q_D(const qMRMLTransformSliders);
  return d->TypeOfTransform;
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setMRMLTransformNode(vtkMRMLNode* node)
{
  this->setMRMLTransformNode(vtkMRMLLinearTransformNode::SafeDownCast(node));
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setMRMLTransformNode(vtkMRMLLinearTransformNode* transformNode)
{
  Q_D(qMRMLTransformSliders);

  this->qvtkReconnect(d->MRMLTransformNode, transformNode,
                      vtkMRMLTransformableNode::TransformModifiedEvent,
                      this, SLOT(onMRMLTransformNodeModified(vtkObject*)));

  this->onMRMLTransformNodeModified(transformNode);

  d->LRSlider->setMRMLTransformNode(transformNode);
  d->PASlider->setMRMLTransformNode(transformNode);
  d->ISSlider->setMRMLTransformNode(transformNode);

  // If the node is NULL, any action on the widget is meaningless, this is why
  // the widget is disabled
  this->setEnabled(transformNode != 0);
  d->MRMLTransformNode = transformNode;
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::onMRMLTransformNodeModified(vtkObject* caller)
{
  vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast(caller);
  if (!transformNode)
    {
    return;
    }
  Q_ASSERT(transformNode);

  // If the type of transform is ROTATION, do not modify
  if(this->typeOfTransform() == qMRMLTransformSliders::ROTATION)
    {
    return;
    }

  vtkNew<vtkTransform> transform;
  qMRMLUtils::getTransformInCoordinateSystem(transformNode,
      this->coordinateReference() == qMRMLTransformSliders::GLOBAL, transform.GetPointer());

  vtkMatrix4x4 * matrix = transform->GetMatrix();
  Q_ASSERT(matrix);
  if (!matrix) { return; }

  //Extract the min/max values from the matrix
  //Change them if the matrix changed externally(python, cli, etc.)
  QPair<double, double> minmax = this->extractMinMaxTranslationValue(matrix, 0.0);
  if(minmax.first < this->minimum())
    {
    minmax.first = minmax.first - 0.3 * fabs(minmax.first);
    this->setMinimum(minmax.first);
    }
  if(minmax.second > this->maximum())
    {
    minmax.second = minmax.second + 0.3 * fabs(minmax.second);
    this->setMaximum(minmax.second);
    }
}

// --------------------------------------------------------------------------
CTK_GET_CPP(qMRMLTransformSliders, vtkMRMLLinearTransformNode*, mrmlTransformNode, MRMLTransformNode);

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setTitle(const QString& _title)
{
  Q_D(qMRMLTransformSliders);
  d->SlidersGroupBox->setTitle(_title);
}

// --------------------------------------------------------------------------
QString qMRMLTransformSliders::title()const
{
  Q_D(const qMRMLTransformSliders);
  return d->SlidersGroupBox->title();
}

// --------------------------------------------------------------------------
int qMRMLTransformSliders::decimals()const
{
  Q_D(const qMRMLTransformSliders);
  return d->LRSlider->decimals();
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setDecimals(int newDecimals)
{
  Q_D(qMRMLTransformSliders);
  // setting the decimals to LRSlider will propagate to the other widgets.
  d->LRSlider->setDecimals(newDecimals);
}

// --------------------------------------------------------------------------
double qMRMLTransformSliders::minimum()const
{
  Q_D(const qMRMLTransformSliders);
  return d->MinValueSpinBox->value();
}

// --------------------------------------------------------------------------
double qMRMLTransformSliders::maximum()const
{
  Q_D(const qMRMLTransformSliders);
  return d->MaxValueSpinBox->value();
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setMinimum(double min)
{
  Q_D(qMRMLTransformSliders);
  d->MinValueSpinBox->setValue(min);
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setMaximum(double max)
{
  Q_D(qMRMLTransformSliders);
  d->MaxValueSpinBox->setValue(max);
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setRange(double min, double max)
{
  Q_D(qMRMLTransformSliders);

  // Could be optimized here by blocking signals on spinboxes and manually
  // call the setRange method on the sliders. Does it really worth it ?
  d->MinValueSpinBox->setValue(min);
  d->MaxValueSpinBox->setValue(max);
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::onMinimumChanged(double min)
{
  Q_D(qMRMLTransformSliders);

  d->LRSlider->setMinimum(min);
  d->PASlider->setMinimum(min);
  d->ISSlider->setMinimum(min);

  emit this->rangeChanged(min, this->maximum());
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::onMaximumChanged(double max)
{
  Q_D(qMRMLTransformSliders);

  d->LRSlider->setMaximum(max);
  d->PASlider->setMaximum(max);
  d->ISSlider->setMaximum(max);

  emit this->rangeChanged(this->minimum(), max);
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setMinMaxVisible(bool visible)
{
  Q_D(qMRMLTransformSliders);
  d->MinMaxWidget->setVisible(visible);
}

// --------------------------------------------------------------------------
bool qMRMLTransformSliders::isMinMaxVisible()const
{
  Q_D(const qMRMLTransformSliders);
  return d->MinMaxWidget->isVisibleTo(
    const_cast<qMRMLTransformSliders*>(this));
}

// --------------------------------------------------------------------------
double qMRMLTransformSliders::singleStep()const
{
  Q_D(const qMRMLTransformSliders);
  // Assumes settings of the sliders are all the same
  return d->PASlider->singleStep();
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setSingleStep(double step)
{
  Q_D(qMRMLTransformSliders);

  d->LRSlider->setSingleStep(step);
  d->PASlider->setSingleStep(step);
  d->ISSlider->setSingleStep(step);
}

// --------------------------------------------------------------------------
QString qMRMLTransformSliders::lrLabel()const
{
  Q_D(const qMRMLTransformSliders);
  return d->LRLabel->text();
}

// --------------------------------------------------------------------------
QString qMRMLTransformSliders::paLabel()const
{
  Q_D(const qMRMLTransformSliders);
  return d->PALabel->text();
}

// --------------------------------------------------------------------------
QString qMRMLTransformSliders::isLabel()const
{
  Q_D(const qMRMLTransformSliders);
  return d->ISLabel->text();
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setLRLabel(const QString& label)
{
  Q_D(qMRMLTransformSliders);
  d->LRLabel->setText(label);
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setPALabel(const QString& label)
{
  Q_D(qMRMLTransformSliders);
  d->PALabel->setText(label);
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::setISLabel(const QString& label)
{
  Q_D(qMRMLTransformSliders);
  d->ISLabel->setText(label);
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::reset()
{
  Q_D(qMRMLTransformSliders);

  d->LRSlider->reset();
  d->PASlider->reset();
  d->ISSlider->reset();
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::resetUnactiveSliders()
{
  Q_D(qMRMLTransformSliders);

  if (!d->ActiveSliders.contains(d->LRSlider))
    {
    bool blocked = d->LRSlider->blockSignals(true);
    d->LRSlider->reset();
    d->LRSlider->blockSignals(blocked);
    }
  if (!d->ActiveSliders.contains(d->PASlider))
    {
    bool blocked = d->PASlider->blockSignals(true);
    d->PASlider->reset();
    d->PASlider->blockSignals(blocked);
    }
  if (!d->ActiveSliders.contains(d->ISSlider))
    {
    bool blocked = d->ISSlider->blockSignals(true);
    d->ISSlider->reset();
    d->ISSlider->blockSignals(blocked);
    }
}

// --------------------------------------------------------------------------
void qMRMLTransformSliders::onSliderPositionChanged(double position)
{
  Q_D(qMRMLTransformSliders);
  qMRMLLinearTransformSlider* slider =
    qobject_cast<qMRMLLinearTransformSlider*>(this->sender());
  Q_ASSERT(slider);
  d->ActiveSliders.push(slider);

  if (this->typeOfTransform() == qMRMLTransformSliders::ROTATION)
    {
    // When a rotation slider is manipulated, the other rotation sliders are
    // reset to 0. Resetting the other sliders should no fire any event.
    this->resetUnactiveSliders();
    }
  slider->applyTransformation(position);
  emit this->valuesChanged();

  d->ActiveSliders.pop();
}

//-----------------------------------------------------------------------------
QPair<double, double> qMRMLTransformSliders::extractMinMaxTranslationValue(
                                             vtkMatrix4x4 * mat, double pad)
{
  QPair<double, double> minmax;
  if (!mat)
    {
    Q_ASSERT(mat);
    return minmax;
    }
  for (int i=0; i <3; i++)
    {
    minmax.first = qMin(minmax.first, mat->GetElement(i,3));
    minmax.second = qMax(minmax.second, mat->GetElement(i,3));
    }
  double range = minmax.second - minmax.first;
  minmax.first = minmax.first - pad * range;
  minmax.second = minmax.second + pad * range;
  return minmax;
}

