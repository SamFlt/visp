import os
from pathlib import Path
import json
import subprocess
from subprocess import CalledProcessError
import os
from typing import List, Union
import shutil
happypose_url = 'https://github.com/agimus-project/happypose.git'




conda_exe = shutil.which('conda')
if conda_exe is None:
  raise RuntimeError('Conda was not found with shutil.which')

def get_happypose_env_path(megapose_env: str) -> Union[Path, None]:
  '''
  Retrieve the megapose environment path, by parsing the conda info --envs command
  may return none if no environment with the given name is found
  '''
  env_data = str(subprocess.check_output('conda info --envs', shell=True).decode())
  env_lines = env_data.split('\n')
  happypose_env_line = [line for line in env_lines if line.startswith(megapose_env)]
  assert len(happypose_env_line) <= 1, 'Found multiple environment names with same name, this should not happen'
  if len(happypose_env_line) == 0:
    return None
  happypose_env_line = happypose_env_line[0]
  happypose_env_path = Path(happypose_env_line.split()[-1]).absolute()
  assert(happypose_env_path.exists())
  return happypose_env_path

def get_megapose_bin_conda_env(happypose_env: str) -> Path:
  '''
  Retrieve the bin folder of a conda environment
  '''
  happypose_env_path = get_happypose_env_path(happypose_env)
  assert happypose_env_path is not None
  happypose_env_bin = happypose_env_path / 'bin'
  return happypose_env_bin

def get_pip_for_conda_env(happypose_env: str):
  '''
  Retrieve the pip script linked to a conda environment
  '''
  conda_bin = get_megapose_bin_conda_env(happypose_env)
  # On windows, pip is not in the bin directory but rather in scripts
  if os.name == 'nt':
    return conda_bin.parent / 'Scripts' / 'pip'
  else:
    return conda_bin / 'pip'

def get_python_for_conda_env(happypose_env: str):
  '''
  Retrieve the python interpreter linked to a conda environment
  '''
  conda_bin = get_megapose_bin_conda_env(happypose_env)
  return conda_bin / 'python'

def get_rclone_for_conda_env(happypose_env: str) -> Path:
  '''
  Retrieve rclone program installed in a conda environment
  '''
  return get_megapose_bin_conda_env(happypose_env) / 'rclone'

def happypose_already_cloned(happypose_path: Path) -> bool:
  '''
  Check whether the happypose GitHub repo has already been cloned at the given location
  '''
  must_exist: List[Path] = [
    happypose_path,
    happy_megapose(happypose_path),
    happypose_path / 'rclone.conf'
  ]
  return all(map(lambda p: p.exists(), must_exist))

def happy_megapose(happypose_path: Path) -> Path:
  return happypose_path / 'happypose' / 'pose_estimators' / 'megapose'

def happypose_env_file(happypose_path: Path) -> Path:
  return happypose_path / 'environment.yml'

def conda_env_already_exists(happypose_env: str) -> bool:
  '''
  Check whether the given conda environment already exists
  '''
  return get_happypose_env_path(happypose_env) is not None

def clone_happypose(happypose_path: Path):
  '''
  Clone the happypose repository, and initialize its submodules. If it already exists, nothing is performed.
  '''
  if not happypose_already_cloned(happypose_path):
    try:
      subprocess.run(['git', 'clone', '--branch', 'dev', '--recurse-submodules', happypose_url, str(happypose_path)], check=True, text=True)
    except CalledProcessError as e:
      print('Could not clone happypose repository')
      exit(1)
  else:
    print('happypose git repo already exists, skipping...')

def install_dependencies(happypose_path: Path, happypose_environment: str):
  '''
  Install or update the required conda dependencies for happypose
  '''
  try:
    if not conda_env_already_exists(happypose_environment):
      subprocess.run([conda_exe, 'env', 'create', '--name', happypose_environment, '--file', happypose_env_file(happypose_path)], check=True)
    else:
      print(f'Conda environment {happypose_environment} already exists, updating dependencies...')
      subprocess.run([conda_exe, 'env', 'update', '--name', happypose_environment, '--file', happypose_env_file(happypose_path)], check=True)
    happypose_env_pip = get_pip_for_conda_env(happypose_environment)
    subprocess.run([str(happypose_env_pip.absolute()), 'install', '-e',  str(happypose_path)], check=True)
  except CalledProcessError as e:
    print('Could not create conda environment')
    exit(1)


def download_models(happypose_env: str, happypose_path: Path, happypose_data_path: Path):
  '''
  Download the happypose deep learning models
  '''
  os.environ['HAPPYPOSE_DATA_DIR'] = str(happypose_data_dir)
  python_bin = get_python_for_conda_env(happypose_env)
  subprocess.run([
    str(python_bin),
    '-m',
    'happypose.toolbox.utils.download',
    '--megapose_models'
  ], check=True)

def install_server(happypose_env: str):
  '''
  Install the megapose_server package
  '''
  happypose_env_pip = get_pip_for_conda_env(happypose_env)
  subprocess.run([happypose_env_pip, 'install', '.'], check=True)



if __name__ == "__main__":
  happypose_variables = None
  with open('./happypose_variables.json', 'r') as variables:
    happypose_variables = json.load(variables)

  megapose_server_dir = Path(os.path.dirname(os.path.abspath(__file__)))

  happypose_dir = Path(happypose_variables['happypose_dir']).absolute()
  happypose_data_dir = Path(happypose_variables['happypose_data_dir']).absolute()
  happypose_environment = happypose_variables['environment']

  happypose_dir.mkdir(exist_ok=True)
  happypose_data_dir.mkdir(exist_ok=True)



  display_message = f'''
This script installs Megapose and the server to communicate with ViSP. The installed Megapose version from the Happypose project
the file "happypose_variables.json" specifies where megapose should be cloned and where the models should be downloaded.
It also contains the name of the conda environment to create.

current values:
  - Happypose directory: {happypose_dir}
  - Happypose model directory: {happypose_data_dir}
  - Conda environment name: {happypose_environment}

Installation requires:
  - git
  - conda

All these programs should be in your path.

It will create a new environment called megapose, with all dependencies installed.
All the megapose models will be downloaded via rclone.

If you encounter any problem, see https://github.com/megapose6d/megapose6d for the installation steps.

The steps followed in the script are the same, but end with the installation of the megapose_server package (the python scripts in this folder)

'''
  print(display_message)
  print('Cloning happypose directory...')
  clone_happypose(happypose_dir)
  print('Creating conda environment and installing happypose...')
  install_dependencies(happypose_dir, happypose_environment)
  print('Downloading megapose models...')
  download_models(happypose_environment, happypose_dir, happypose_data_dir)
  print('Installing server...')
  install_server(happypose_environment)

  print(f'''
Megapose server is now installed!
Try:
  $ conda activate {happypose_environment}
  $ python -m megapose_server.run -h
  ''')
