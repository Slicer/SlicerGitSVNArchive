from __main__ import ctk
from __main__ import qt
from __main__ import slicer
from __main__ import vtk

#
# ScriptedLoadableExtensionTemplate
#

class ScriptedLoadableExtensionTemplate:
    def __init__(self, parent):
        parent.title = "Scripted Loadable Extension Template"
        parent.categories = ["Examples"]
        parent.dependencies = []
        parent.contributors = ["Jean-Christophe Fillion-Robin (Kitware), \
        Steve Pieper (Isomics)"] # replace with "Firstname Lastname (Org)"
        parent.helpText = """Example of scripted loadable extension."""
        parent.acknowledgementText = """
        This file was originally developed by Jean-Christophe Fillion-Robin, \
        Kitware Inc. and Steve Pieper, Isomics, Inc. and was partially funded \
        by NIH grant 3P41RR013218-12S1.
        """ # replace with organization, grant and thanks.
        self.parent = parent

#
# qScriptedLoadableExtensionTemplateWidget
#

class ScriptedLoadableExtensionTemplateWidget:
    def __init__(self, parent=None):
        if parent is None:
            self.parent = slicer.qMRMLWidget()
            self.parent.setLayout(qt.QVBoxLayout())
            self.parent.setMRMLScene(slicer.mrmlScene)
        else:
            self.parent = parent
        self.layout = self.parent.layout()
        if parent is None:
            self.setup()
            self.parent.show()

    def setup(self):
        # Instantiate and connect widgets ...

        # reload button
        # (use this during development, but remove it when delivering
        #    your module to users)
        ### START DEVELOPMENT TOOL ###
        self.reloadButton = qt.QPushButton("Reload")
        self.reloadButton.toolTip = "Reload this module."
        self.reloadButton.name = "ScriptedLoadableExtensionTemplate Reload"
        self.layout.addWidget(self.reloadButton)
        self.reloadButton.connect('clicked()', self.onReload)
        ###        END TOOL        ###

        # Collapsible button
        dummyCollapsibleButton = ctk.ctkCollapsibleButton()
        dummyCollapsibleButton.text = "A collapsible button"
        self.layout.addWidget(dummyCollapsibleButton)

        # Layout within the dummy collapsible button
        dummyFormLayout = qt.QFormLayout(dummyCollapsibleButton)

        # HelloWorld button
        helloWorldButton = qt.QPushButton("Hello world")
        helloWorldButton.toolTip = "Print 'Hello world' in standard ouput."
        dummyFormLayout.addWidget(helloWorldButton)
        helloWorldButton.connect('clicked(bool)', self.onHelloWorldButtonClicked)

        # Add vertical spacer
        self.layout.addStretch(1)

        # Set local var as instance attribute
        self.helloWorldButton = helloWorldButton

    def onHelloWorldButtonClicked(self):
        print "Hello World !"

    def onReload(self, moduleName="ScriptedLoadableExtensionTemplate"):
        """ Generic development reload method for any scripted module.
            ModuleWizard will subsitute correct default moduleName.
            DEVELOPMENT TOOL
        """
        import imp
        import os
        import sys

        import slicer


        widgetName = moduleName + "Widget"
        # reload the source code
        # - set source file path
        # - load the module to the global space
        filePath = eval('slicer.modules.%s.path' % moduleName.lower())
        p = os.path.dirname(filePath)
        if not sys.path.__contains__(p):
            sys.path.insert(0, p)
        fp = open(filePath, "r")
        globals()[moduleName] = imp.load_module(moduleName, fp, filePath,
                                                ('.py', 'r', imp.PY_SOURCE))
        fp.close()
        # rebuild the widget
        # - find and hide the existing widget
        # - create a new widget in the existing parent
        parent = slicer.util.findChildren(name='%s Reload' % moduleName)[0].parent()
        for child in parent.children():
            try:
                child.hide()
            except AttributeError:
                pass
        globals()[widgetName.lower()] = eval('globals()["%s"].%s(parent)' %
                                             (moduleName, widgetName))
        globals()[widgetName.lower()].setup()
