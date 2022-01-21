#!/usr/bin/env python3

#
# Script that builds the client source code.
#

import sys
import argparse

from tools import cmake_functions
from tools import working_dir
from tools import util

#
# Main run() function (entry point for this script)
#
def run():
  parser = argparse.ArgumentParser()

  parser.add_argument('--release_cxx', help='Build a release C++ binary (default: false)', type=bool, default=False)
  parser.add_argument('--skip_tools_build', help='Skip building/configuring tools project (faster startup if tools have not changed)', type=bool, default=False, required=False)
  parser.add_argument('--build_single_threaded', help='Build the single threaded version of the application', type=bool, default=True)
  parser.add_argument('--build_multi_threaded', help='Build the multi threaded version of the application', type=bool, default=True)

  app_args = parser.parse_args()

  if not cmake_functions.program_exists("emcmake") or not cmake_functions.program_exists("emmake"):
    print("EMSDK not found - aborting!")
    return -1

  #
  # Step 1: Make sure building tools tools are up to date
  #
  tooldir = 'tools-release' if app_args.release_cxx else 'tools-debug'
  if not app_args.skip_tools_build:
    tools_cmake_args = ["cmake", "../..", "."]
    working_dir.cwd_cpp_binary_dir(tooldir)
    tools_cmake_args += cmake_functions.cxx_build_cmake_args(debug_build=not app_args.release_cxx)
    print('Configuring CMake tools directory... (this can take a long time!)')
    util.call_or_die(tools_cmake_args)
    print('Building protoc...')
    util.call_or_die(["cmake", "--build", ".", "--config", ("MinSizeRel" if app_args.release_cxx else "Debug"), "--target", "protoc"])
    print('Building igpack-gen...')
    util.call_or_die(["cmake", "--build", ".", "--config", ("MinSizeRel" if app_args.release_cxx else "Debug"), "--target", "igpack-gen"])
    print('Tools build complete!')
  
  #
  # Step 2: Build the WASM binaries and JavaScript glue for the sanctify client app...
  #
  if app_args.build_single_threaded:
    bindir = 'wasm-st-release' if app_args.release_cxx else 'wasm-st-debug'
    working_dir.cwd_cpp_binary_dir(bindir)
    bin_cmake_args = ["emcmake", "cmake", "../..", "."]
    bin_cmake_args += cmake_functions.cxx_build_cmake_args(debug_build=not app_args.release_cxx, threads=False, tool_build_root=tooldir)
    print('Configuring single threaded build CMake WASM directory... (this can take a long time!)')
    util.call_or_die(bin_cmake_args)
    print('Building sanctify-game-client (single threaded)...')
    util.call_or_die(["emmake", "ninja", "sanctify-game-client"])
    working_dir.copy_js_and_wasm(bindir, threaded=False)


  if app_args.build_multi_threaded:
    bindir = 'wasm-mt-release' if app_args.release_cxx else 'wasm-mt-debug'
    working_dir.cwd_cpp_binary_dir(bindir)
    bin_cmake_args = ["emcmake", "cmake", "../..", "."]
    bin_cmake_args += cmake_functions.cxx_build_cmake_args(debug_build=not app_args.release_cxx, threads=True, tool_build_root=tooldir)
    print('Configuring multi threaded build CMake WASM directory... (this can take a long time!)')
    util.call_or_die(bin_cmake_args)
    print('Building sanctify-game-client (multi threaded)...')
    util.call_or_die(["emmake", "ninja", "sanctify-game-client"])
    working_dir.copy_js_and_wasm(bindir, threaded=True)

  #
  # Finally, exit this process happily:
  #
  print("Finished!")
  return 0

if __name__ == '__main__':
  sys.exit(run())