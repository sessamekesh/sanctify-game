import sys
import subprocess

def call_or_die(cmd):
  try:
    sys.stdout.flush()
    sys.stderr.flush()
    print(cmd)
    subprocess.check_call(cmd, shell=True)
  except subprocess.CalledProcessError as e:
    print("'%s' failed (%s)", ' '.join(cmd), str(e.returncode))
    sys.exit(-1)