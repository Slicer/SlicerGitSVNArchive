import hashlib
import io
import json
import os
import unittest
import random
import re
import string
import subprocess
import sys
import threading

import BaseHTTPServer
import SocketServer as socketserver

from urlparse import urlparse, parse_qs

import SlicerExtensionBuildSystemTestConfig as config

_server = None
_midas_token = 'TestTokenWithExpectedLengthxxxxxxxxxxxxx'
_requests = []

def get_cmakecache_values(file_path, variables):
  result = dict.fromkeys(variables)
  with open(file_path) as cmakecache:
    for line in cmakecache:
      line = line.strip()
      if not line or line.startswith('//'):
        continue
      for variable in list(variables):
        if line.startswith(variable):
          result[variable] = string.split(line, sep='=', maxsplit=1)[1]
          variables.remove(variable)
          break
      if not len(variables):
        break
  return result

def save_request(request, response_code):
  global _requests
  _requests.append((request, response_code))
  print("do_" + request.split(' ')[0] + '[%s]' % response_code)
  sys.stdout.flush()

def get_open_port():
  """Sources:
  * http://stackoverflow.com/questions/2838244/get-open-tcp-port-in-python/2838309#2838309
  * https://www.dnorth.net/2012/03/17/the-port-0-trick/
  * https://pypi.python.org/pypi/port-for/
  """
  import socket
  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  s.bind(("",0))
  port = s.getsockname()[1]
  return port

class Handler(BaseHTTPServer.BaseHTTPRequestHandler):

  def send_response_with_message(self,
      code=200, message="OK", response_type="text/html"):
    self.send_response(code)
    self.send_header('Content-Type', response_type)
    self.send_header('Content-Length', str(len(message)))
    self.send_header('Connection', 'close')
    self.end_headers()
    self.wfile.write(message)

  def do_GET(self):
    if self.headers.getheader("Expect") == "100-continue":
      self.send_response(100)
      self.end_headers()

    response_type = "text/html"
    message="OK"

    is_midas = self.path.startswith('/midas3/api/json')
    if is_midas:
      response_type = "application/json"
      query_data = parse_qs(urlparse(self.path).query)
      if query_data['method'][0] == 'midas.login':
        # Send back token
        response_data = {
          'stat': 'ok',
          'code': '0',
          'data': {
            'token': _midas_token
          }
        }
        message = json.dumps(response_data)

    self.send_response_with_message(response_type=response_type, message=message)


  def do_PUT(self):

    if self.headers.getheader("Expect") == "100-continue":
      self.send_response(100)
      self.end_headers()

    length = int(self.headers['Content-Length'])
    content = None
    with io.BytesIO() as file:
      blockSize = 1024 * 1024
      bytesLeft = length
      while bytesLeft > 0:
        bytesToRead = min(blockSize, bytesLeft)
        file.write(self.rfile.read(bytesToRead))
        bytesLeft -= bytesToRead
      content = file.getvalue()

    response_type = "text/html"
    message= "OK"

    is_cdash = self.path.startswith('/submit.php')
    if is_cdash:
      hash = hashlib.md5()
      hash.update(content)
      md5sum = hash.hexdigest()

      response_type = "text/xml"
      message = """<cdash version="2.3.0">
        <status>OK</status>
        <message></message>
        <md5>%s</md5>
      </cdash>
      """ % md5sum

    is_midas = self.path.startswith('/midas3/api/json')
    if is_midas:
      response_type = "application/json"
      query_data = parse_qs(urlparse(self.path).query)
      if query_data['method'][0] == 'midas.slicerpackages.extension.upload':
        # Send back extension data
        response_data = {
          'stat': 'ok',
          'code': '0',
          'message': '',
          'data': {
            'extension': {
              'productname': query_data['productname'][0]
              # XXX Real server sends back all properties of extension DAO
            }
          }
        }
        message = json.dumps(response_data)

    self.send_response_with_message(response_type=response_type, message=message)

  def log_message(self, *args, **kwargs):
    request = args[1]
    response_code = args[2]
    if response_code == '100': # Do not keep track of continuation request
      return
    save_request(request, response_code)


class ServerThread(object):
  def __init__(self, port):
    self.port = port
    self.httpd = socketserver.TCPServer(('127.0.0.1', self.port), Handler)
    self.thread = threading.Thread(target=self.start, args=(self.httpd,))
    self.thread.start()

  def start(self, httpd):
    httpd.serve_forever()

  def stop(self):
    #
    self.httpd.shutdown()
    self.httpd.server_close()
    self.thread.join(timeout=10)


class SlicerExtensionBuildSystemTest(unittest.TestCase):

  def setUp(self):
    # Set CMAKE_CFG_INTDIR
    cmake_cfg_intdir = sys.argv[1]
    self.assertTrue(cmake_cfg_intdir == '.' and not config.CMAKE_BUILD_TYPE == "")
    self.assertTrue(not config.CMAKE_BUILD_TYPE == "" and config.CMAKE_CONFIGURATION_TYPES == "")
    setattr(config, 'CMAKE_CFG_INTDIR', cmake_cfg_intdir)

    # Get an open port
    self.port = get_open_port()

    # Clear request lists
    global _requests
    _requests = []

    # Start mock server
    global _server
    _server = ServerThread(self.port)

  def tearDown(self):
    if _server:
      _server.stop()

  def test_index_build_with_upload(self):
    self._test_index_build('build_with_upload', True)

  def test_index_build_without_upload(self):
    self._test_index_build('build_without_upload', False)

  def _test_index_build(self, test_name, test_upload):

    # Filter out paths starting with config.Slicer_SUPERBUILD_DIR
    # This is needed to ensure executables like svn or git installed
    # on the system resolved their expected dependencies (e.g system OpenSSL
    # libraries instead of Slicer OpenSSL libraries)
    env = dict(os.environ)
    for varname in ['LD_LIBRARY_PATH', 'PATH', 'PYTHONPATH']:
      paths = env[varname].split(os.pathsep)
      filtered_paths = [path for path in paths if not path.startswith(config.Slicer_SUPERBUILD_DIR)]
      env[varname] = os.pathsep.join(filtered_paths)

    test_binary_dir = config.CMAKE_CURRENT_BINARY_DIR + '/' + test_name + '-build'

    # Remove binary directory
    subprocess.check_call(
      [config.CMAKE_COMMAND, '-E', 'remove_directory', test_binary_dir],
      cwd=config.CMAKE_CURRENT_BINARY_DIR,
      env=env
    )

    # Create binary directory
    subprocess.check_call(
      [config.CMAKE_COMMAND, '-E', 'make_directory', test_binary_dir],
      cwd=config.CMAKE_CURRENT_BINARY_DIR,
      env=env
    )

    # Prepare configure command
    cmd = [
      config.CMAKE_COMMAND,
      '-G', config.CMAKE_GENERATOR,
      '-DSlicer_DIR:PATH=' + config.Slicer_BINARY_DIR,
      '-DSlicer_EXTENSION_DESCRIPTION_DIR:PATH=' + config.CMAKE_CURRENT_SOURCE_DIR + '/TestIndex',
      '-DSlicer_LOCAL_EXTENSIONS_DIR:PATH=' + config.CMAKE_CURRENT_SOURCE_DIR,
     ' -DBUILD_TESTING:BOOL=0',
      '-DCMAKE_C_COMPILER:PATH=' + config.CMAKE_C_COMPILER,
      '-DCMAKE_CXX_COMPILER:PATH=' + config.CMAKE_CXX_COMPILER
      ]

    if not config.CMAKE_CONFIGURATION_TYPES:
      cmd.append('-DCMAKE_BUILD_TYPE:STRING=' + config.CMAKE_BUILD_TYPE)

    if config.CMAKE_GENERATOR_PLATFORM:
      cmd.extend(['-A', config.CMAKE_GENERATOR_PLATFORM])

    if config.CMAKE_GENERATOR_TOOLSET:
      cmd.extend(['-T', config.CMAKE_GENERATOR_TOOLSET])

    cmd.append('-DSlicer_UPLOAD_EXTENSIONS:BOOL=%s' % ('1' if test_upload else '0'))

    if test_upload:
      cmd.append('-DCTEST_DROP_SITE:STRING=127.0.0.1:%d' % self.port)
      cmd.append('-DMIDAS_PACKAGE_URL:STRING=http://127.0.0.1:%d/midas3' % self.port)

    cmd.append(config.Slicer_SOURCE_DIR + '/Extensions/CMake')

    # Configure
    subprocess.check_call(
      cmd,
      cwd=test_binary_dir,
      env=env
    )

    # Build
    cmd = [config.CMAKE_COMMAND, '--build', test_binary_dir]
    if config.CMAKE_CONFIGURATION_TYPES:
      cmd.extend(['--config', config.CMAKE_CFG_INTDIR])
    subprocess.check_call(
      cmd, cwd=test_binary_dir, env=env
    )

    # Check extension caches
    for extensionName in ['TestExtA', 'TestExtB', 'TestExtC']:

      # Read cache
      varname = '%s_BUILD_SLICER_EXTENSION' % extensionName
      cmakecache = test_binary_dir + '/' + extensionName + '-build/CMakeCache.txt'
      cache_values = get_cmakecache_values(cmakecache, [varname])

      # Check <ExtensionName>_BUILD_SLICER_EXTENSION variable
      self.assertIn(cache_values[varname], ['1', 'ON', 'TRUE'])

    # Read TestExtC cache
    cmakecache = test_binary_dir + '/TestExtC-build/CMakeCache.txt'
    cache_values = get_cmakecache_values(cmakecache, [
      'EXTENSION_DEPENDS',
      'TestExtA_DIR',
      'TestExtB_DIR',
      ])

    # Check EXTENSION_DEPENDS variable
    self.assertEqual(cache_values['EXTENSION_DEPENDS'], 'TestExtA;TestExtB')

    # Check <extensionName>_DIR variables
    self.assertIsNotNone(cache_values['TestExtA_DIR'])
    self.assertTrue(os.path.exists(cache_values['TestExtA_DIR']))
    self.assertIsNotNone(cache_values['TestExtB_DIR'])
    self.assertTrue(os.path.exists(cache_values['TestExtB_DIR']))

    # Check TestExtC generated description file
    cmd = [
      config.CMAKE_COMMAND,
      '-DSlicer_EXTENSIONS_CMAKE_DIR:PATH=' + config.Slicer_SOURCE_DIR + '/Extensions/CMake',
      '-DTestExtC_BUILD_DIR:PATH=' + test_binary_dir + '/TestExtC-build/',
      '-P', config.CMAKE_CURRENT_SOURCE_DIR + '/CheckGeneratedDescriptionFiles.cmake'
      ]
    subprocess.check_call(
      cmd, cwd=test_binary_dir, env=env
    )

    def parse_request(result):
      (request, response_code) = result
      self.assertEqual(response_code, '200')
      (method, raw_query, _) = request.split(' ')
      query_data = parse_qs(urlparse(raw_query).query)
      return (method, query_data)

    def check_cdash_request(parsed_request, expected_http_method, expected_filename_regex):
      http_method = parsed_request[0]
      query_data = parsed_request[1]
      self.assertEqual(http_method, expected_http_method)
      self.assertEqual(query_data['project'][0], 'Slicer4')
      self.assertIsNotNone(re.match(expected_filename_regex, query_data['FileName'][0]))

    def check_midas_request(parsed_request, expected_http_method, expected_midas_method, expected_params={}):
      http_method = parsed_request[0]
      query_data = parsed_request[1]
      self.assertEqual(http_method, expected_http_method)
      self.assertEqual(query_data['method'][0], expected_midas_method)
      for expected_key, expected_value in expected_params.iteritems():
        self.assertEqual(query_data[expected_key][0], expected_value)

    # Check CDash and Midas queries
    if not test_upload:
      self.assertEqual(_requests, [])
    else:
      self.assertEqual(len(_requests), 7 * 3)

      requests = iter(_requests)

      for extensionName in ['TestExtA', 'TestExtB', 'TestExtC']:
        # Upload configure/build/test results to CDash
        check_cdash_request(parse_request(requests.next()), 'PUT', r'.+' + extensionName + r'.+Configure\.xml')
        check_cdash_request(parse_request(requests.next()), 'PUT', r'.+' + extensionName + r'.+Build\.xml')
        check_cdash_request(parse_request(requests.next()), 'PUT', r'.+' + extensionName + r'.+Test\.xml')
        # Upload package to midas
        check_midas_request(parse_request(requests.next()), 'GET', 'midas.login')
        check_midas_request(parse_request(requests.next()), 'PUT', 'midas.slicerpackages.extension.upload', {'productname': extensionName})
        # Upload packaging result to CDash
        check_cdash_request(parse_request(requests.next()), 'PUT', r'.+' + extensionName + r'.+Build\.xml')
        # Upload url to CDash
        check_cdash_request(parse_request(requests.next()), 'PUT', r'.+' + extensionName + r'.+Upload\.xml')
