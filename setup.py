from sys import platform, version_info
from os import path

from distutils.core import setup, Extension
from distutils.command.build_ext import build_ext

libpython_tags = tuple(
    tag for tag in (
        '2.7.18', '3.5.10', '3.6.15', '3.7.12', '3.8.12', '3.9.7'
    ) if tag[:3] != '{}.{}'.format(
        version_info.major, version_info.minor
    )
)

libpython_dists = path.join(path.dirname(__file__), 'pydists')
libpython_libs = path.join(path.dirname(__file__), 'pycc', 'lib')

required = (
    'codecs',
    'encodings',
    'abc',
    'io',
    'stat',
    '_collections_abc',
    'genericpath',
    'posixpath',
    'os',
    '_sitebuiltins',
    '_bootlocale',
    'site',
    'rlcompleter',
    'ast',
    'types',
    'enum',
    'sre_constants',
    'sre_parse',
    'sre_compile',
    'operator',
    'keyword',
    'heapq',
    'reprlib',
    'collections',
    'functools',
    'copyreg', 'copy_reg',
    're',
    'linecache',
    'sysconfig',
    '_sysconfigdata',
    '_weakrefset',
    'UserDict',
    'traceback'
)

builtin = (
    '_heapq',
    '_struct',
    '_datetime',
    '_random',
    'unicodedata',
    'zlib'
)


class build_extpp(build_ext):
    def run(self):
        super(build_extpp, self).run()

        if platform == 'linux':
            self._fetch_libpython()
            self._unpack_libpython()
            self._build_libpython_linux()

    def _fetch_libpython(self):
        from urllib.request import urlopen
        from shutil import copyfileobj
        from os import unlink, makedirs

        if not path.isdir(libpython_dists):
            makedirs(libpython_dists)

        for tag in libpython_tags:
            dest = path.join(libpython_dists, 'python-' + tag + '.tar.gz')

            if path.isfile(dest):
                continue

            print('Fetch', tag)

            dist = 'https://github.com/python/cpython/' \
                'archive/refs/tags/v' + tag + '.tar.gz'

            try:
                with urlopen(dist) as response:
                    with open(dest, 'w+b') as out:
                        copyfileobj(response, out)
            except Exception:
                if path.isfile(dest):
                    unlink(dest)
                raise

    def _unpack_libpython(self):
        from tarfile import open as taropen

        for tag in libpython_tags:
            dest = path.join(libpython_dists, 'cpython-' + tag)
            if path.isdir(dest):
                continue

            print('Unpack', tag)
            src = path.join(libpython_dists, 'python-' + tag) + '.tar.gz'

            with taropen(src, mode='r:gz') as tar:
                tar.extractall(libpython_dists)

    def _build_libpython_linux(self):
        from subprocess import check_call
        from shutil import move, copytree, copy
        from os import makedirs, listdir
        from tempfile import mkdtemp
        from zipfile import ZipFile, ZIP_DEFLATED

        if not path.isdir(libpython_libs):
            makedirs(libpython_libs)

        for tag in libpython_tags:
            libver = '.'.join(tag.split('.')[:2])
            libver_dest = ''.join(tag.split('.')[:2])
            libname = 'libpython' + libver + '.so'
            dest_libname = 'python' + libver_dest + '.so'
            libzip = 'python' + libver_dest + '.zip'
            libname_dest = path.join(libpython_libs, dest_libname)
            libzip_dest = path.join(libpython_libs, libzip)

            srcs = path.join(libpython_dists, 'cpython-' + tag)

            if path.isfile(libname_dest) and path.isfile(libzip_dest):
                continue

            print('Build', tag)

            build_dir = mkdtemp()
            pylib = path.join(build_dir, 'Lib')

            try:
                check_call([
                    'sh',
                    path.abspath(path.join(srcs, 'configure')),
                    '--enable-shared', '--disable-static',
                    '--enable-optimizations'
                ], cwd=build_dir)

                setup_dest = path.join(build_dir, 'Modules', 'Setup.local')
                setup_dist = path.join(srcs, 'Modules', 'Setup.dist')
                if not path.isfile(setup_dist):
                    setup_dist = path.join(srcs, 'Modules', 'Setup')

                setup_dist_data = open(setup_dist).readlines()
                expected_builtins = tuple(
                    '#' + item for item in builtin
                )

                with open(setup_dest, 'w+') as out:
                    for line in setup_dist_data:
                        if line.startswith(expected_builtins):
                            out.write(line[1:])

                check_call([
                    'make', 'python',
                    'ABIFLAGS=', 'LIBS=-lz -ldl -lm -lutil -pthread'
                ], cwd=build_dir)

                print('Copy tree')
                copytree(
                    path.join(srcs, 'Lib'),
                    path.join(build_dir, 'Lib'),
                    True
                )

                print('Generate posix vars')
                check_call([
                    path.join(build_dir, 'python'),
                    '-E', '-S', '-m', 'sysconfig', '--generate-posix-vars'
                ], cwd=build_dir, env={
                    'LD_LIBRARY_PATH': build_dir,
                    'PYTHONHOME': pylib
                })

                if tag[0] != '2':
                    print('Get build dir')
                    pybuilddir = open(
                        path.join(build_dir, 'pybuilddir.txt')).read()

                    datas = tuple(
                        x for x in listdir(path.join(build_dir, pybuilddir))
                        if x.endswith('.py')
                    )

                    copy(
                        path.join(build_dir, pybuilddir, datas[0]),
                        path.join(build_dir, 'Lib', '_sysconfigdata.py')
                    )

                print('Compile all the things')

                compile_args = [
                    path.join(build_dir, 'python'),
                    '-Wi', '-tt', '-E', '-S',
                    path.join('Lib', 'compileall.py'),
                    '-f', '-x',
                    'bad_coding|badsyntax|site-packages|lib2to3/tests/data'
                ]

                if tag[0] != '2':
                    compile_args.append('-b')

                compile_args.append('.')

                check_call(
                    compile_args, cwd=build_dir, env={
                        'LD_LIBRARY_PATH': build_dir,
                        'PYTHONHOME': pylib
                    }
                )

                move(
                    path.join(build_dir, libname),
                    libname_dest
                )

            finally:
                pass
                # rmtree(build_dir)

            print('Build zip', tag)

            library = []

            required_packages = required
            required_modules = tuple(
                item + '.pyc' for item in required
            )

            for diritem in listdir(pylib):
                if diritem in required_packages:
                    pkgdir = path.join(pylib, diritem)
                    for pkgitem in listdir(pkgdir):
                        if pkgitem.endswith('.pyc'):
                            library.append(path.join(diritem, pkgitem))

                elif diritem in required_modules:
                    library.append(diritem)

            if not library:
                raise ValueError('Failed to find any dep')

            with ZipFile(libzip_dest, 'w', ZIP_DEFLATED) as zip:
                for item in library:
                    zip.write(path.join(pylib, item), item)


pycc = Extension(
    'pycc._pycc',
    sources=[
        'pycc.c', 'ccctx.c',
        'ccctx_internal.c',
        'ccctx_internal_pyX.c',
    ]
)


setup(
    name='pycc',
    version='0.3',
    description='Python bytecode cross-compiler (for python3)',
    packages=['pycc'],
    package_dir={
        'pycc': 'pycc'
    },
    ext_modules=[
        pycc
    ],
    cmdclass={
        'build_ext': build_extpp
    },
    classifiers=[
        'Development Status :: 3 - Alpha',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3.8',
    ],
    package_data={
        'pycc': ['lib/*.so', 'lib/*.zip']
    }
)
