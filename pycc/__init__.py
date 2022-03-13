from os import path
from sys import platform

from pycc._pycc import Ctx as _Ctx
from pycc._pycc import error


def Ctx(pymaj, pymin, load_from=None):
    '''
    Ctx(pymaj, pymin, load_from=None)
    '''

    if load_from is not None:
        return _Ctx(pymaj, pymin, load_from)

    topdir = path.abspath(path.dirname(__file__))
    libpython_dir = path.abspath(path.join(topdir, 'lib'))
    libpython_fmt = 'python{}{}'

    if platform == 'win32':
        libpython_fmt += '.dll'
    else:
        libpython_fmt += '.so'

    libpython_file = path.join(
        libpython_dir, libpython_fmt.format(pymaj, pymin)
    )

    libpython_zip = path.join(
        libpython_dir, 'python{}{}.zip'.format(pymaj, pymin)
    )

    if path.isfile(libpython_file) and path.isfile(libpython_zip):
        return _Ctx(pymaj, pymin, libpython_file, topdir)
    else:
        return _Ctx(pymaj, pymin)
