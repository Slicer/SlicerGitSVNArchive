import os
import sys
import itertools
import re
import shutil
import textwrap
import subprocess
from subprocess import PIPE, CalledProcessError
from collections import namedtuple, deque

# This script helps with creation of fully-relocated application bundles.
# This means all non-system RPATHs (ELF, Mach-O) are guaranteed to only
# refer inside the bundle. On Windows (TODO) all load-time dependencies
# are dumped alongside the application binary
#
# The approach here is to make two passes:
# - collect dependencies and put them in place in the package
#   - resolution uses `Fixer.bundled_path`
# - fixing references assuming everything is placed correctly
#   - here we process all executable objects in the bundle,
#     and resolve non-system, non-framework dependencies based
#     on what is actually in the bundle. Any unresolved deps
#     cause an error.
#     note: we cannot check LoadLibrary calls, directly,
#           but this should be ok assuming the application
#           is well-behaved and only calls LoadLibrary on
#           libs that live in the bundle.

# Requires CLI tools:
#   darwin: otool, install_name_tool
#   nt: dumpbin
#   linux: ldd
#   cygwin: TODO

# Set condition to False for debug output
def _debug(info):
    if True: return
    print(info)


# Context
Context = namedtuple('Context',
                     ['slicer_build', 'slicer_superbuild', 'app_name',
                      'app_dir', 'libs', 'libs_path', 'itkfactories_dir',
                      'qtmodules_dir', 'climodules_dir', 'slicer_lib_dir',
                      'fixup_path', 'python_stdlib_dir',
                      'python_sitepackages_dir', 'qtplugins_dir', 'resolved'])

###############################################################################
# Utilities

def walk_files(basepath):
    """Returns iterator: all files under `basepath`"""

    [[(yield os.path.join(p,f)) for f in fns] for (p,_,fns) in os.walk(basepath)]

def walk_predicate(predicate, basepath):
    """Returns iterator: all files under `basepath` matching predicate(f)"""

    [(yield f) for f in walk_files(basepath) if predicate(f)]

def rel_path_join(*kw):
    # avoid absolute path if any component starts with /
    return os.path.normpath(os.path.join(kw[0], *[x.lstrip("/") for x in kw[1:]]))

###############################################################################
# NT

###############################################################################
# Linux

###############################################################################

class Fixer(object):
    def __init__(self, context, env=None):
        self.context = context
        self.env = env

    def copy_dep(self, source, target):
        if os.path.exists(target):
            print("warning: skipping copy_dep target -- {}".format(target))
            return

        _debug("  | want to copy:\n      {}\n      -> {}".format(source, target))

        assert(target.startswith(self.context.app_dir))
        assert(os.path.isabs(source))
        assert(os.path.isabs(target))

        # handle missing intermediates in target
        if not os.path.isdir(os.path.split(target)[0]):
            os.makedirs(os.path.split(target)[0])

        # do the copy!
        if os.path.isfile(source):
            shutil.copy(source, target)
        elif os.path.isdir(source):
            # note we MUST set symlinks=True here, or else
            # the internal relative paths in the Qt package
            # will not be resolvable.
            shutil.copytree(source, target, symlinks=True)
        else:
            raise Exception("Unhandled copy_dep type: {} -> {}".format(source,target))

    def bundle_deps(self):
        # start with the executable, and target libs from installer
        q = deque([self.exe_name()] + self.context.libs)
        self.context.resolved.update({ x: x for x in self.context.libs})

        while len(q) > 0:
            current_item = q.popleft()

            _debug("Current item: {}".format(current_item))

            if not os.path.exists(current_item):
                raise Exception("!-! process_deps: item must exist: {}".format(current_item))

            # copy dependencies, as needed
            for dep_ref in self.file_shared_deps(current_item):
                bundled_relpath = self.bundled_path(dep_ref)
                if bundled_relpath is None:
                    continue

                target_path = self.real_bundled_path(bundled_relpath)

                if not self.context.resolved.has_key(target_path):
                    _debug("  Processing dependency:\n    {}".format(target_path))

                    source_path = self.resolve_dep_path(dep_ref, current_item)
                    self.copy_dep(source_path, target_path)
                    self.context.resolved[target_path] = source_path
                    q.append(target_path)

                    _debug("    ->    {}".format(source_path))

###############################################################################
# Darwin

# 0xfeedfacf is 64-bit Mach-O header magic
MACHO_MAGIC = bytearray.fromhex('feedfacf')
MACHO_CIGAM = bytearray.fromhex('cffaedfe')

Dylib = namedtuple('Dylib', ['id', 'dylibs', 'rpaths'])

def is_macho(f):
    return open(f).read(4) in (MACHO_MAGIC, MACHO_CIGAM)

def otool_analyze_dylib(fname, env=None):
    assert(os.path.isfile(fname))
    id_re = re.compile(r"""Load command.*\n\s+cmd LC_ID_DYLIB\n.*\n\s+name\ (\S+)""")
    dylib_re = re.compile(r"""Load command.*\n\s+cmd LC_LOAD_DYLIB\n.*\n\s+name\ (\S+)""")
    rpath_re = re.compile(r"""Load command.*\n\s+cmd LC_RPATH\n.*\n\s+path\ (\S+)""")

    proc = subprocess.Popen(['otool', '-l', fname], stderr=PIPE, stdout=PIPE)
    output, _ = proc.communicate()

    id     = id_re.findall(output) or None
    dylibs = dylib_re.findall(output)
    rpaths = rpath_re.findall(output)
    return Dylib(id, dylibs, rpaths)

class FixerDarwin(Fixer):
    # TODO configure?
    _allow_framework = ["Qt"]
    _allow_unresolved = ['libqsqlpsql', 'libqsqlmysql']

    def __init__(self, context, env=None):
        super(FixerDarwin, self).__init__(context, env)

        self._dep_cache = {}

        # note: these must be ordered by decreasing specificity!
        self._matchers = self._init_matchers()

    def exe_name(self):
        # TODO generalize to Info.plist
        return os.path.join(self.context.app_dir, "Contents/MacOS/Slicer")

    def file_shared_deps(self, lib):
        return self.file_info(lib).dylibs

    def file_info(self, lib, cache=True):
        if not cache: return otool_analyze_dylib(lib)

        ret = self._dep_cache.get(lib, None)
        return (ret
                if ret
                else self._dep_cache.setdefault(lib, otool_analyze_dylib(lib)))

    def file_rpaths(self, lib, cache=True):
        return self.file_info(lib, cache=cache).rpaths

    def copy_dep(self, source, target):
        # frameworks must be copied whole
        m = re.search(r"(.+)/([^/]+\.framework)", source)
        if m:
            source = os.path.join(*m.groups())
            target = rel_path_join(self.context.app_dir, "Contents/Frameworks/", m.group(2))
            assert(os.path.isdir(source))

        super(FixerDarwin, self).copy_dep(source, target)

    def resolve_dep_path(self, lib, ref_lib):
        if os.path.isabs(lib) and os.path.isfile(lib):
            return lib
        elif lib.startswith("@rpath"):
            real_rpaths = self.file_info(ref_lib).rpaths

            # also try app rpaths because CMake
            # deletes unknown RPATHs
            # FIXME: don't hardcode app name
            real_rpaths.extend(self.file_rpaths(os.path.join(self.context.app_dir, "Contents/MacOS/Slicer")))

            sub_rpath = lib.replace("@rpath", "")
            for path in real_rpaths:
                try_rpath = rel_path_join(path, sub_rpath)
                if os.path.isfile(try_rpath):
                    return try_rpath
        else:
            for path in self.context.libs_path:
                test_path = rel_path_join(path, lib)
                if os.path.isfile(test_path):
                    return test_path

        raise Exception("Could not `resolve_dep_path` for: {}".format(lib))


    def real_bundled_path(self, bundled_path):
        return rel_path_join(self.context.app_dir, "Contents", *bundled_path)

    def is_system(self, lib):
        return re.search("^(/System|/Library|/usr/lib/|/opt/X11/)", lib) is not None

    def is_rpath(self, lib):
        return lib.startswith("@rpath")

    def in_superbuild(self, lib):
        return lib.startswith(self.context.slicer_superbuild)

    def use_framework(self, lib):
        if ".framework" in lib:
            for fw in self._allow_framework:
                if re.match(fw + "[^/]+\.framework"):
                    return True
        else:
            return False

    def _init_matchers(self):
        return [
            (re.compile(r"Contents/bin/([^/]+)"),
                ["bin"]
            ),
            (re.compile(r"([^/]+\.framework)/(.+)"),
                ["Frameworks"]
            ),
            (re.compile(r"(libpython[^/]+\.dylib)$"),
                ["lib/Python/lib"]
            ),
            (re.compile(r"lib-dynload/([^/]+\.so)$"),
                ["lib/Python/", self.context.python_stdlib_dir, "lib-dynload"]
            ),
            (re.compile(r"tcl-build/lib/(lib[^/]+\.dylib)$"),
                ["lib/TclTk/lib"]
            ),
            (re.compile(r"(libitcl[^/]+\.dylib)$"),
                ["lib/TclTk/lib/itcl4.0.1"]
            ),
            (re.compile(self.context.climodules_dir + r"/([^/]+)"),
                [self.context.climodules_dir]
            ),
            (re.compile(self.context.qtmodules_dir + r"/([^/]+\.(?:so|dylib))$"),
                [self.context.qtmodules_dir]
            ),
            (re.compile(self.context.itkfactories_dir + r"/([^/]+Plugin\.(?:so|dylib))$"),
                [self.context.itkfactories_dir]
            ),
            (re.compile(self.context.qtplugins_dir + r"/(designer|iconengines|styles|imageformats|sqldrivers)" +
                                                r"/([^/]+\.so|dylib)"),
                [self.context.qtplugins_dir],
            ),
            # note: this is a very broad match, so it must be last
            (re.compile(r"([^/]+\.dylib|so)$"),
                [self.context.slicer_lib_dir]
            )]

    def bundled_path(self, lib):
        lib = os.path.normpath(lib)

        if self.is_system(lib):
            return None

        if (os.path.isabs(lib) and not
           (self.in_superbuild(lib) or self.use_framework(lib))):
            return None

        for matchpair in self._matchers:
            m = matchpair[0].search(lib)
            if m:
                return matchpair[1] + list(m.groups())

        raise Exception("Unhandled lib: {}".format(lib))

    # use `apply_changes=False` for verification-only
    def remap_dylib_references(self, apply_changes=True):
        app_contents = os.path.join(self.context.app_dir, "Contents")

        # note: we must include everything here, in order to resolve symlinks
        #       however, we do a test for samefile in the resolution loop.
        macho_files = [f for f in walk_predicate(is_macho, self.context.app_dir)]

        # sort by decreasing length so that searching macho_files will give
        # the most-specific library reference first.
        macho_files.sort(lambda x,y: cmp(len(x), len(y)))

        # reset the lookup cache
        self._dep_cache = {}

        # main loop: walk all the files and make fixes
        for obj_abspath in macho_files:
            _debug("Resolving: {}".format(obj_abspath))

            # reset the rewrite batch for this object
            rewrites = []

            # object real path in the bundle
            obj_relpath = obj_abspath.replace(app_contents, "")

            # fix the install name (LC_ID_DYLIB -- only in dylibs of course)
            if self.file_info(obj_abspath).id is not None:
                rewrites.extend(['-id', rel_path_join("@rpath", *self.bundled_path(obj_abspath))])

            # figure out the package-relative path of this object, to use for rpath
            obj_reldotted = os.path.relpath(self.context.app_dir+"/Contents", os.path.split(obj_abspath)[0])
            # TODO: assert that this is still in the bundle?
            relref = "@loader_path/" + obj_reldotted + "/"


            # loop and fix loader hints (LC_RPATH)
            for rpath in self.file_rpaths(obj_abspath):
                if os.path.isabs(rpath):
                    rewrites.extend(['-delete_rpath', rpath])
                elif rpath == relref:
                    # skip if already exists
                    relref = ""

            if relref:
                rewrites.extend(['-add_rpath', relref])


            # loop and fix dylib references (LC_LOAD_DYLIB)
            for ref in self.file_info(obj_abspath).dylibs:

                # assuming unique names, so take the last component
                ref_lib = os.path.split(ref)[-1]
                ref_rel = ref.replace(app_contents, "")

                if os.path.isabs(ref):
                    if self.is_system(ref):
                        continue
                    elif any([x in obj_abspath for x in self._allow_unresolved]):
                        _debug("! skipping unresolved from allow-list: {}".format(ref))
                        continue
                    elif self.in_superbuild(ref) or self.use_framework(ref):
                        pass
                    else:
                        raise Exception("Prohibited dylib reference to absolute path: {}\n  by: {}".
                                                                               format(ref, obj_abspath))


                # we search every known Mach-O in the bundle
                # note: we know the file exists already, and
                #       can use the match immediately because
                #       macho_files is *relative* to .app
                # note: we exclude the samefile to avoid
                #       symlinked version in frameworks.
                ref_filt = []
                for x in macho_files:
                    if (x.endswith(ref_lib) and not [os.path.samefile(x, z) for z in ref_filt]):
                        ref_filt.append(x)

                # we expect to find only one match
                if len(ref_filt) != 1:
                    raise Exception("Dependency error finding: {}\n  got: {}".
                                                        format(ref, ref_filt))

                newref_relpath = ref_filt[0].replace(app_contents, "")
                assert(self.in_superbuild(os.path.join(app_contents, ref_filt[0])))
                newref = os.path.normpath('@rpath/' + rel_path_join(newref_relpath))

                rewrites.extend(['-change', ref, newref])

            # commit the rewrites, if any
            if apply_changes and rewrites:
                _debug("= rewrites: {}".format(rewrites))

                assert(obj_abspath.startswith(self.context.app_dir))
                subprocess.check_output(['install_name_tool', obj_abspath] + rewrites)


    def platform_step(self):
        self.remap_dylib_references()

class FixerWin32(Fixer):
    pass

class FixerPosix(Fixer):
    # Need to set LD_LIBRARY_PATH before calling ldd
    pass

def read_config(path):
    # Output order set in: `{Slicer_SRC}/CMake/SlicerCPackBundleFixup.cmake.in`
    with open(path) as f:
        f_in = f.read()
        regex = re.compile(r"""slicer_build='(?P<slicer_build>.+)'\n
                               slicer_superbuild='(?P<slicer_superbuild>.+)'\n
                               app_name='(?P<app_name>.+)'\n
                               app_dir='(?P<app_dir>.+)'\n
                               libs='(?P<libs>.+)'\n
                               libs_path='(?P<libs_path>.+)'\n
                               slicer_lib_dir='(?P<slicer_lib_dir>.+)'\n
                               itkfactories_dir='(?P<itkfactories_dir>.+)'\n
                               qtmodules_dir='(?P<qtmodules_dir>.+)'\n
                               qtplugins_dir='(?P<qtplugins_dir>.+)'\n
                               climodules_dir='(?P<climodules_dir>.+)'\n
                               python_stdlib_dir='(?P<python_stdlib_dir>.+)'\n
                               python_sitepackages_dir='(?P<python_sitepackages_dir>.+)'\n
                               fixup_path='(?P<fixup_path>.+)'
                            """, re.X)

        match = regex.match(f_in)
        assert(match is not None)

        d = match.groupdict()
        for key,value in d.iteritems():
            if key in ['libs', 'libs_path']: d[key] = value.split(';')
            else: d[key] = os.path.normpath(value)

        return d

def fix_package(config):
    fixer = None
    env = os.environ.copy()

    d = read_config(config)
    context = Context(app_name=d['app_name'], slicer_build=d['slicer_build'],
                    slicer_superbuild=d['slicer_superbuild'], libs=d['libs'],
                    libs_path=d['libs_path'], itkfactories_dir=d['itkfactories_dir'],
                    app_dir=d['app_dir'], qtmodules_dir=d['qtmodules_dir'],
                    climodules_dir=d['climodules_dir'], fixup_path=d['fixup_path'],
                    python_stdlib_dir=d['python_stdlib_dir'],
                    python_sitepackages_dir=d['python_sitepackages_dir'],
                    qtplugins_dir=d['qtplugins_dir'], slicer_lib_dir=d['slicer_lib_dir'],
                    resolved={})

    if sys.platform == 'darwin':
        # note: darwin is POSIX, so this must come first.
        fixer = FixerDarwin(context)
    elif os.name == 'posix':
        if sys.platform == 'cygwin': env['PATH']            += addpath
        else:                        env['LD_LIBRARY_PATH'] += addpath
        raise Exception("TODO: general non-Darwin posix support")
        fixer = FixerPosix(env)
    elif sys.platform == 'win32':
        raise Exception("TODO: win32 support")
    else:
        raise Exception("Unknown platform")

    # find and bundle all depdencies
    fixer.bundle_deps()

    # run the platform-specific fixup routine to make sure package is relocatable
    fixer.platform_step()



fix_package('/opt/bld/s5nj/Slicer-build/CMake/SlicerCPackBundleFixup/bundle_config.txt')