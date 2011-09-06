/*=auto=========================================================================

 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) 
 All Rights Reserved.

 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.

 Program:   3D Slicer

=========================================================================auto=*/

#ifndef __qSlicerBaseQTGUIPythonQtDecorators_h
#define __qSlicerBaseQTGUIPythonQtDecorators_h

// Qt includes
#include <QObject>

// PythonQt includes
#include <PythonQt.h>

// CTK includes
#include <ctkAbstractPythonManager.h>

// SlicerQt includes
#include "qSlicerAbstractModuleRepresentation.h"
#include "qSlicerAbstractModuleWidget.h"
#include "qSlicerAbstractModule.h"
#include "qSlicerPythonManager.h"
#include "qSlicerCommandOptions.h"

#include "qSlicerBaseQTGUIExport.h"


// NOTE:
//
// For decorators it is assumed that the methods will never be called
// with the self argument as NULL.  The self argument is the first argument
// for non-static methods.
//

class Q_SLICER_BASE_QTGUI_EXPORT qSlicerBaseQTGUIPythonQtDecorators : public QObject
{
  Q_OBJECT

public:

  qSlicerBaseQTGUIPythonQtDecorators(ctkAbstractPythonManager* pythonManager)
    {
    Q_ASSERT(pythonManager);
    pythonManager->registerClassForPythonQt(&qSlicerAbstractModuleWidget::staticMetaObject);
    pythonManager->registerClassForPythonQt(&qSlicerPythonManager::staticMetaObject);
    pythonManager->registerClassForPythonQt(&qSlicerCommandOptions::staticMetaObject);
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

#endif
