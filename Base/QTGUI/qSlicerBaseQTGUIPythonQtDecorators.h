/*=auto=========================================================================

 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH)
 All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Program:   3D Slicer

=========================================================================auto=*/

#ifndef __qSlicerBaseQTGUIPythonQtDecorators_h
#define __qSlicerBaseQTGUIPythonQtDecorators_h

// PythonQt includes
#include <PythonQt.h>

// SlicerQt includes
#include "qSlicerAbstractModuleRepresentation.h"
#include "qSlicerAbstractModuleWidget.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerCommandOptions.h"
#include "qSlicerPythonManager.h"

#include "qSlicerBaseQTGUIExport.h"


// NOTE:
//
// For decorators it is assumed that the methods will never be called
// with the self argument as NULL.  The self argument is the first argument
// for non-static methods.
//

class qSlicerBaseQTGUIPythonQtDecorators : public QObject
{
  Q_OBJECT

public:

  qSlicerBaseQTGUIPythonQtDecorators()
    {
    PythonQt::self()->registerClass(&qSlicerAbstractModuleWidget::staticMetaObject);
    PythonQt::self()->registerClass(&qSlicerPythonManager::staticMetaObject);
    PythonQt::self()->registerClass(&qSlicerCommandOptions::staticMetaObject);
    // Note: Use registerCPPClassForPythonQt to register pure Cpp classes
    }

public slots:

  //----------------------------------------------------------------------------
  // qSlicerAbstractModule

  //----------------------------------------------------------------------------
  qSlicerAbstractModuleWidget* widgetRepresentation(qSlicerAbstractModule* _module)
    {
    return dynamic_cast<qSlicerAbstractModuleWidget*>(_module->widgetRepresentation());
    }

  //----------------------------------------------------------------------------
  // qSlicerAbstractModule

  //----------------------------------------------------------------------------
  qSlicerAbstractModule* module(qSlicerAbstractModuleWidget * _moduleWidget)
    {
    return dynamic_cast<qSlicerAbstractModule*>(_moduleWidget->module());
    }
};

//-----------------------------------------------------------------------------
void initqSlicerBaseQTGUIPythonQtDecorators()
{
  PythonQt::self()->addDecorators(new qSlicerBaseQTGUIPythonQtDecorators);
}

#endif
