import imp
import json
import os
import sys
import types

not_loaded = 'not loaded'

def library_loader(module_name):
  #print("Loading %s" % module_name)
  fp, pathname, description = imp.find_module(module_name)
  module = imp.load_module(module_name, fp, pathname, description)
  return module

class LazyModule(types.ModuleType):
  """Subclass of ModuleType that implements a custom __getattribute__ method
  to allow lazy-loading of attributes from slicer sub-modules."""

  def __init__(self, name):
    types.ModuleType.__init__(self, name)
    self.__lazy_attributes = {}
    #print("__lazy_attributes: %s" % len(self.__lazy_attributes))

  def _update_lazy_attributes(self, lazy_attributes):
    self.__lazy_attributes.update(lazy_attributes)
    for k in lazy_attributes:
      setattr(self, k, not_loaded)

  def __getattribute__(self, attr):
    value = types.ModuleType.__getattribute__(self, attr)
    #print("__getattribute__ %s" % (attr))
    if value is not_loaded:
      module_name = self.__lazy_attributes[attr]

      module = library_loader(module_name)
      namespace = module.__dict__

      # Load into 'namespace' first, then self.__dict__ (via setattr) to
      # prevent the warnings about overwriting the 'NotLoaded' values
      # already in self.__dict__ we would get if we just update
      # self.__dict__.
      for k, v in namespace.items():
        if not k.startswith('_'):
          setattr(self, k, v)
      value = namespace[attr]
    return value

def writeModuleAttributeFile(module_name, config_dir='.'):
  try:
    exec("import %s as module" % module_name)
  except ImportError as details:
    print("%s [skipped: failed to import: %s]" % (module_name, details))
    return
  attributes = []
  for attr in dir(module):
    if not attr.startswith('__'):
      attributes.append(attr)
  filename = os.path.join(config_dir, "%s.json" % module_name)
  with open(filename, 'w') as output:
    print("%s [done: %s]" % (module_name, filename))
    output.write(json.dumps({"attributes":attributes}, indent=4))

def updateLazyModule(module, input_module_names=[], config_dir=None):
  if isinstance(module, basestring):
    if module not in sys.modules:
      print("updateLazyModule failed: Couldn't find %s module" % module)
      return
    module = sys.modules[module]
  if not isinstance(module, LazyModule):
    print("updateLazyModule failed: module '%s' is not a LazyModule" % module)
    return
  if isinstance(input_module_names, basestring):
    input_module_names = [input_module_names]
  if config_dir is None:
    config_dir = os.path.dirname(module.__path__[0])
  for input_module_name in input_module_names:
    filename = os.path.join(config_dir, "%s.json" % input_module_name)
    with open(filename) as input:
      module_attributes = json.load(input)['attributes']
      #print("Updating %s with %d attributes" % (filename, len(module_attributes)))
      module._update_lazy_attributes({attribute: input_module_name for attribute in module_attributes})

    #print("Updated %s module with %d attributes from %s" % (module, len(module._LazyModule__lazy_attributes), input_module_name))

def createLazyModule(module_name, module_path, input_module_names=[], config_dir=None):

  thisModule = sys.modules[module_name] if module_name in sys.modules else None

  if isinstance(thisModule, LazyModule):
    # Handle reload case where we've already done this once.
    # If we made a new module every time, multiple reload()s would fail
    # because the identity of sys.modules['itk'] would always be changing.
    #print("slicer: Calling ctor of LazyModule")
    thisModule.__init__(module_name)
  else:
    print("slicer: Creating new LazyModule")
    thisModule = LazyModule(module_name)

  # Set the __path__ attribute, which is required for this module to be used as a
  # package
  setattr(thisModule, '__path__', module_path)

  sys.modules[module_name] = thisModule

  updateLazyModule(thisModule, input_module_names, config_dir)

  return thisModule
