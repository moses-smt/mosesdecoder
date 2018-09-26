from __future__ import print_function
from distutils.core import setup
from distutils.extension import Extension

import sys
import os

mosesdir = os.path.abspath('../../')
includes = [mosesdir, os.path.join(mosesdir, 'moses'), os.path.join(mosesdir, 'OnDiskPt'), os.path.join(mosesdir, 'moses/TranslationModel')] 
libdir = os.path.join(mosesdir, 'lib')

# options
# TODO: use argparse
available_switches = ['--with-cmph', '--moses-lib', '--cython', '--max-factors', '--max-kenlm-order']
with_cmph = False
defines = {'MAX_NUM_FACTORS':'1'}
suffix = '.cpp'
cmdcls = {}
while sys.argv[-1].split('=')[0] in available_switches:
    param = sys.argv.pop().split('=')
    if param[0] == '--with-cmph':
        with_cmph = True
    if param[0] == '--moses-lib':
        libdir = param[1]
    if param[0] == '--cython':
        print('I will be cythoning your pyx files...', file=sys.stderr)
        try:
            from Cython.Distutils import build_ext
            suffix = '.pyx'
            cmdcls['build_ext'] = build_ext
        except ImportError:
            print('You do not seem to have Cython installed')
    if param[0] == '--max-factors':
        defines['MAX_NUM_FACTORS'] = param[1]
    if param[0] == '--max-kenlm-order':
        defines['KENLM_MAX_ORDER'] = param[1]

print('mosesdir=%s\nincludes=%s\nlibdir=%s\ncmph=%s' % (mosesdir, includes, libdir, with_cmph), file=sys.stderr)

#basic=['z', 'stdc++', 'pthread', 'm', 'gcc_s', 'c', 'boost_system', 'boost_filesystem']
basic=[]
moses = ['moses']
additional = []

if with_cmph:
    additional.append('cmph')

extensions = [
            Extension(name='moses.dictree',
                sources=['moses/dictree' + suffix],
                include_dirs = ['.'] + includes,
                library_dirs = [libdir],
                runtime_library_dirs = [libdir],
                libraries= basic + moses + additional,
                language='c++',
                extra_compile_args=['-O3'] + ['-D%s=%s' % (k, v) for k, v in defines.iteritems()]
                ),
            ]

setup(
    cmdclass = cmdcls,
    name = 'moses',
    ext_modules= extensions,
    packages = ['moses'],
    package_dir = {'':'.'},

)
