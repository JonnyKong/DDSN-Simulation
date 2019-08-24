# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION = 0.1

from waflib import Utils
import os

def options(opt):
    opt.load(['compiler_cxx', 'cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags', 'sanitizers', 'boost', 'protoc'], tooldir=['.waf-tools'])

    opt.add_option('--with-tests', action='store_true', default=False,
                   dest='with_tests', help='''Build unit tests''')

def configure(conf):
    conf.load(['compiler_cxx', 'cxx', 'gnu_dirs', 'default-compiler-flags', 'sanitizers', 'boost', 'protoc'])

    if 'PKG_CONFIG_PATH' not in os.environ:
        os.environ['PKG_CONFIG_PATH'] = Utils.subst_vars('${LIBDIR}/pkgconfig', conf.env)

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    boost_libs = 'system log unit_test_framework'
    conf.check_boost(lib=boost_libs)

def build(bld):
    bld.env['VERSION'] = VERSION

    bld.stlib(target = 'vsync',
              name = 'vsync',
              source = bld.path.ant_glob(['lib/*.cpp', 'lib/*.proto']),
              use = 'NDN_CXX BOOST',
              includes = 'lib',
              export_includes = 'lib',
              cxxflags = '-DBOOST_LOG_DYN_LINK -Wno-deprecated-declarations')

    bld.program(target = 'vsync-test',
                name = 'vsync-test',
                source = bld.path.ant_glob(['tests/*.cpp']),
                includes = 'tests',
                use = 'NDN_CXX BOOST vsync',
                cxxflags = '-DBOOST_TEST_DYN_LINK -Wno-deprecated-declarations')

    bld.program(target = 'simple',
                name = 'simple',
                source = 'examples/simple.cpp',
                includes = 'examples',
                use = 'NDNCXX BOOST vsync',
                cxxflags = '-Wno-deprecated-declarations')
