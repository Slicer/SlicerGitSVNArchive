import glob
import os
import re

from .ExtensionProject import ExtensionProject

#=============================================================================
class ExtensionDescription(object):
  """Representation of an extension description.

  This class provides a Python object representation of an extension
  description. The extension information is made available as attributes on the
  object. The "well known" attributes are described
  :wikidoc:`Developers/Extensions/DescriptionFile here`. Custom attributes may
  be added with :func:`setattr`. Attributes may be removed with :func:`delattr`
  or the :meth:`.clear` method.
  """

  _reParam = re.compile(r"([a-zA-Z][a-zA-Z0-9_]*)\s+(.+)")

  #---------------------------------------------------------------------------
  def __init__(self, repo=None, filepath=None, sourcedir=None):
    """
    :param repo:
      Extension repository from which to create the description.
    :type repo:
      :class:`git.Repo <git:git.repo.base.Repo>`,
      :class:`.Subversion.Repository` or ``None``.
    :param filepath:
      Path to an existing ``.s4ext`` to read.
    :type filepath:
      :class:`basestring` or ``None``.
    :param sourcedir:
      Path to an extension source directory.
    :type sourcedir:
      :class:`basestring` or ``None``.

    :raises:
      * :exc:`~exceptions.KeyError` if the extension description is missing a
        required attribute.
      * :exc:`~exceptions.Exception` if there is some other problem
        constructing the description.

    The description may be created from a repository instance (in which case
    the description repository information will be populated), a path to the
    extension source directory, or a path to an existing ``.s4ext`` file.
    No more than one of ``repo``, ``filepath`` or ``sourcedir`` may be given.
    If none are provided, the description will be incomplete.
    """

    args = (repo, filepath, sourcedir)
    if args.count(None) < len(args) - 1:
      raise Exception("cannot construct %s: only one of"
                      " (repo, filepath, sourcedir) may be given" %
                      type(self).__name__)

    if filepath is not None:
      with open(filepath) as fp:
        self._read(fp)

    elif repo is not None:
      # Handle git repositories
      if hasattr(repo, "remotes"):
        remote = None
        svnRemote = None

        # Get SHA of HEAD (may not exist if no commit has been made yet!)
        try:
          sha = repo.head.commit.hexsha

        except ValueError:
          sha = "NA"

        # Try to get git remote
        try:
          remote = repo.remotes.origin
        except:
          if len(repo.remotes) == 1:
            remote = repo.remotes[0]

        if remote is None:
          # Try to get svn remote
          config = repo.config_reader()
          for s in config.sections():
            if s.startswith("svn-remote"):
              svnRemote = s[12:-1]
              break

          if svnRemote is None:
            # Do we have any remotes?
            if len(repo.remotes) == 0:
              setattr(self, "scm", "git")
              setattr(self, "scmurl", "NA")
              setattr(self, "scmrevision", sha)

            else:
              raise Exception("unable to determine repository's primary remote")

          else:
            si = self._gitSvnInfo(repo, svnRemote)
            setattr(self, "scm", "svn")
            setattr(self, "scmurl", si["URL"])
            setattr(self, "scmrevision", si["Revision"])

        else:
          setattr(self, "scm", "git")
          setattr(self, "scmurl", self._remotePublicUrl(remote))
          setattr(self, "scmrevision", sha)

        sourcedir = repo.working_tree_dir

      # Handle svn repositories
      elif hasattr(repo, "wc_root"):
        setattr(self, "scm", "svn")
        setattr(self, "scmurl", repo.url)
        setattr(self, "scmrevision", repo.last_change_revision)
        sourcedir = repo.wc_root

    else:
      setattr(self, "scm", "local")
      setattr(self, "scmurl", "NA")
      setattr(self, "scmrevision", "NA")

    if sourcedir is not None:
      p = ExtensionProject(sourcedir)
      self._setProjectAttribute("homepage", p, required=True)
      self._setProjectAttribute("category", p, required=True)
      self._setProjectAttribute("description", p)
      self._setProjectAttribute("contributors", p)

      self._setProjectAttribute("status", p)
      self._setProjectAttribute("enabled", p, default="1")
      self._setProjectAttribute("depends", p, default="NA")
      self._setProjectAttribute("build_subdirectory", p, default=".")

      self._setProjectAttribute("iconurl", p)
      self._setProjectAttribute("screenshoturls", p)

      if self.scm == "svn":
        self._setProjectAttribute("svnusername", p, elideempty=True)
        self._setProjectAttribute("svnpassword", p, elideempty=True)

  #---------------------------------------------------------------------------
  def __repr__(self):
    return repr(self.__dict__)

  #---------------------------------------------------------------------------
  @staticmethod
  def _remotePublicUrl(remote):
    url = remote.url
    if url.startswith("git@"):
      return url.replace(":", "/").replace("git@", "git://")

    return url

  #---------------------------------------------------------------------------
  @staticmethod
  def _gitSvnInfo(repo, remote):
    result = {}
    for l in repo.git.svn('info', R=remote).split("\n"):
      if len(l):
        key, value = l.split(":", 1)
        result[key] = value.strip()
    return result

  #---------------------------------------------------------------------------
  def _setProjectAttribute(self, name, project, default=None, required=False,
                           elideempty=False, substitute=True):

    if default is None and not required:
      default=""

    v = project.getValue("EXTENSION_" + name.upper(), default, substitute)

    if len(v) or not elideempty:
      setattr(self, name, v)

  #---------------------------------------------------------------------------
  def clear(self, attr=None):
    """Remove attributes from the extension description.

    :param attr: Name of attribute to remove.
    :type attr: :class:`str` or ``None``

    If ``attr`` is not ``None``, this removes the specified attribute from the
    description object, equivalent to calling ``delattr(instance, attr)``. If
    ``attr`` is ``None``, all attributes are removed.
    """

    for key in self.__dict__.keys() if attr is None else (attr,):
      delattr(self, key)

  #---------------------------------------------------------------------------
  def _read(self, fp):
    for l in fp:
      m = self._reParam.match(l)
      if m is not None:
        setattr(self, m.group(1), m.group(2).strip())

  #---------------------------------------------------------------------------
  def read(self, path):
    """Read extension description from directory.

    :param path: Directory containing extension description.
    :type path: :class:`basestring`

    :raises:
      :exc:`~exceptions.IOError` if ``path`` does not contain exactly one
      extension description file.

    This attempts to read an extension description from the specified ``path``
    which contains a single extension description (``.s4ext``) file (usually an
    extension build directory).
    """

    self.clear()

    descriptionFiles = glob.glob(os.path.join(path, "*.[Ss]4[Ee][Xx][Tt]"))
    if len(descriptionFiles) < 1:
      raise IOError("extension description file not found")

    if len(descriptionFiles) > 1:
      raise IOError("multiple extension description files found")

    with open(descriptionFiles[0]) as fp:
      self._read(fp)

  #---------------------------------------------------------------------------
  def _write(self, fp):
    fp.write("""#
# First token of each non-comment line is the keyword and the rest of the line
# (including spaces) is the value.
# - the value can be blank
#

# This is source code manager (i.e. svn)\n""")
    fp.write("scm " + ("%s" % getattr(self, "scm")).strip() + "\n")
    fp.write("scmurl " + ("%s" % getattr(self, "scmurl")).strip() + "\n")
    fp.write("scmrevision " + ("%s" % getattr(self, "scmrevision")).strip() + "\n")
    fp.write("""
# list dependencies
# - These should be names of other modules that have .s4ext files
# - The dependencies will be built first\n""")
    fp.write("depends " + ("%s" % getattr(self, "depends")).strip() + "\n")
    fp.write("""
# Inner build directory (default is ".")\n""")
    fp.write("build_subdirectory " + ("%s" % getattr(self, "build_subdirectory")).strip() + "\n")
    fp.write("""
# homepage\n""")
    fp.write("homepage " + ("%s" % getattr(self, "homepage")).strip() + "\n")
    fp.write("""
# Firstname1 Lastname1 ([SubOrg1, ]Org1), Firstname2 Lastname2 ([SubOrg2, ]Org2)
# For example: Jane Roe (Superware), John Doe (Lab1, Nowhere), Joe Bloggs (Noware)\n""")
    fp.write("contributors " + ("%s" % getattr(self, "contributors")).strip() + "\n")
    fp.write("""
# Match category in the xml description of the module (where it shows up in Modules menu)\n""")
    fp.write("category " + ("%s" % getattr(self, "category")).strip() + "\n")
    fp.write("""
# url to icon (png, size 128x128 pixels)\n""")
    fp.write("iconurl " + ("%s" % getattr(self, "iconurl")).strip() + "\n")
    fp.write("""
# Give people an idea what to expect from this code
#  - Is it just a test or something you stand behind?\n""")
    fp.write("status " + ("%s" % getattr(self, "status")).strip() + "\n")
    fp.write("""
# One line stating what the module does\n""")
    fp.write("description " + ("%s" % getattr(self, "description")).strip() + "\n")
    fp.write("""
# Space separated list of urls\n""")
    fp.write("screenshoturls " + ("%s" % getattr(self, "screenshoturls")).strip() + "\n")
    fp.write("""
# 0 or 1: Define if the extension should be enabled after its installation.\n""")
    fp.write("enabled " + ("%s" % getattr(self, "enabled")).strip() + "\n")

  #---------------------------------------------------------------------------
  def write(self, out):
    """Write extension description to a file or stream.

    :param out: Stream or path to which to write the description.
    :type out: :class:`~io.IOBase` or :class:`basestring`

    This writes the extension description to the specified file path or stream
    object. This is suitable for producing a ``.s4ext`` file from a description
    object.
    """

    if hasattr(out, "write") and callable(out.write):
      self._write(out)

    else:
      with open(out, "w") as fp:
        self._write(fp)
