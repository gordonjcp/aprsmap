#! /usr/bin/env python

# the following two variables are used by the target "waf dist"
VERSION='0'
APPNAME='aprsmap'

# these variables are mandatory ('/' are converted automatically)
top = '.'
out = 'build'

def options(opt):
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')
    conf.check(header_name='stdlib.h')
    conf.check(header_name='math.h')

    conf.check_cc(lib='m', uselib_store='M')

    conf.env.CCFLAGS = ['-O0', '-g3']
    conf.check_cfg(package='gtk+-2.0', uselib_store='GTK', atleast_version='2.6.0', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package='gmodule-2.0', uselib_store='GMODULE', atleast_version='2.18.0', args='--cflags --libs')
    conf.check_cfg(package='gthread-2.0', uselib_store='GTHREAD', atleast_version = '2.32.1', args = '--cflags --libs')
    # because of a packaging bug is libosmgpsmap, check for libsoup explicitly
    conf.check_cfg(package='libsoup-2.4', atleast_version = '2.4', args = '--cflags --libs')
    
    conf.check_cfg(package='osmgpsmap', uselib_store='OSMGPSMAP', atleast_version = '0.7.2', args = '--cflags --libs')
    conf.check_cfg(package='libfap', uselib_store='FAP', atleast_version = '1.1', args = '--cflags --libs')
    conf.check_cfg(package='sqlite3', uselib_store='SQL', atleast_version = '3', args = '--cflags --libs')
    conf.check_cfg(package='gthread-2.0', uselib_store='GTHREAD2', atleast_version = '2.32.1', args = '--cflags --libs')


def build(bld):
    # aprsmap
    bld(
        features = 'c cprogram',
        source = ['aprsis.c', 'callbacks.c', 'mapviewer.c', 'station.c'],
        target = 'aprsmap',
        uselib = 'GTK OSMGPSMAP FAP GMODULE SQL GTHREAD2 M',
        includes = '. /usr/include')

