from distutils.core import setup, Extension

pycc = Extension(
    'pycc',
    sources=[
        'pycc.c', 'ccctx.c',
        'ccctx_internal.c',
        'ccctx_internal_pyX.c',
    ]
)

setup(
    name='pycc',
    version='0.1',
    description='Python bytecode cross-compiler (for python3)',
    ext_modules=[
        pycc
    ]
)
