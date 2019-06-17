#!/usr/bin/env python3
# encoding: utf-8

from distutils.core import setup, Extension
from setuptools.command.install import install
import os

rpygrating_module = Extension('_rpigratings', 
		sources = ['rpg/_rpigratings.c'],
                extra_compile_args = ['-O3'],
		extra_link_args=['-lwiringPi'])


#Edit .bashrc to stop cursor showing up on main monitor
class InstallWrapper(install):
  """For running custom code on install"""

  def run(self):
    self._edit_bashrc()
    self._make_logdir()
    install.run(self)

  def _make_logdir(self):
    log_path = os.path.expanduser("~/rpg/logs")
    if not os.path.exists(log_path):
      try:
        os.makedirs(log_path)
      except PermissionError:
        print("Install failed")
        print("Try running as sudo")
        return

  def _edit_bashrc(self):
    try:
      with open(os.path.expanduser("~/.bashrc"), 'r') as f:
        found = f.read().find('RPG_CURSOR_HIDE')
    except PermissionError:
      print("Install failed")
      print("Try running install as sudo")
      return

    if found == -1:
      try:
        with open(os.path.expanduser("~/.bashrc"), "a") as f:
          f.write('\n')
          f.write('# RPG_CURSOR_HIDE\n')
          f.write('# Added by Python RPG module to remove cursor all windows but SSH\n')
          f.write('if [ -n "$SSH_CONNECTION" ]; then\n')
          f.write('  setterm -cursor on\n')
          f.write('else\n')
          f.write('  setterm -cursor off\n')
          f.write('fi\n')
      except PermissionError:
        print("Install failed")
        print("Try running install as sudo")

setup(name='rpg',
      version='0.1.0',
      packages = ['rpg'],
      description='A drifting grating implimentation',
      ext_modules=[rpygrating_module],
      cmdclass={'install': InstallWrapper}
)
