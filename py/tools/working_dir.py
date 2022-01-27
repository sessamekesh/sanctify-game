import os
import shutil

def cwd_project_root():
  os.chdir(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

def cwd_cpp_root():
  os.chdir(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'cpp')))

def cwd_cpp_binary_dir(buildname):
  path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'cpp', 'out', buildname))
  if not os.path.exists(path):
    os.makedirs(path)
  os.chdir(path)

def copy_js_and_wasm(buildpath, threaded):
  src_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'cpp', 'out', buildpath, 'sanctify-game', 'client'))
  dest_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'ts', 'packages', 'sanctify-web-client', 'public', ('wasm_mt' if threaded else 'wasm_st')))
  shutil.copyfile(os.path.join(src_path, 'sanctify-game-client.js'), os.path.join(dest_path, 'sanctify-game-client.js'))
  shutil.copyfile(os.path.join(src_path, 'sanctify-game-client.wasm'), os.path.join(dest_path, 'sanctify-game-client.wasm'))
  if threaded:
    shutil.copyfile(os.path.join(src_path, 'sanctify-game-client.worker.js'), os.path.join(dest_path, 'sanctify-game-client.worker.js'))
  if os.path.exists(os.path.join(src_path, 'sanctify-game-client.wasm.map')):
    shutil.copyfile(os.path.join(src_path, 'sanctify-game-client.wasm.map'), os.path.join(dest_path, 'sanctify-game-client.wasm.map'))
  shutil.rmtree(os.path.join(dest_path, '..', 'resources'))
  shutil.copytree(os.path.join(src_path, 'resources'), os.path.join(dest_path, '..', 'resources'))