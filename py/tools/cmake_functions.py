import os
import hashlib
import time

# Find the given executable in PATH, return true if it's found and false otherwise
def program_exists(program_name):
  fpath, fname = os.path.split(program_name)
  if fpath:
    if os.path.isfile(fpath) and os.access(fpath, os.X_OK):
      return True
  else:
    for path in os.environ["PATH"].split(os.pathsep):
      path = path.strip("'")
      exe_file = os.path.join(path, program_name)
      if os.path.isfile(exe_file) and os.access(exe_file, os.X_OK):
        return True
  return False

def cxx_build_cmake_args(tool_build_root=None, threads=True, graphics_debugging=False, logging=True, debug_build=True):
  args = []
  args.append('-DIG_BUILD_TESTS=OFF')
  args.append('-DIG_BUILD_SERVER=OFF')
  args.append('-DIG_ENABLE_THREADS=' + ('ON' if threads else 'OFF'))
  args.append('-DIG_ENABLE_GRAPHICS_DEBUGGING=' + ('ON' if graphics_debugging else 'OFF'))
  args.append('-DIG_ENABLE_LOGGING=' + ('ON' if logging else 'OFF'))
  args.append('-DIG_TOOL_WRANGLE_PATH=' + ('ig-tools.cmake' if not tool_build_root else ('../' + tool_build_root + '/ig-tools.cmake')))
  args.append('-DCMAKE_BUILD_TYPE=' + ('Debug' if debug_build else 'MinSizeRel'))
  return args