#! /usr/bin/env python

# the following two variables are used by the target "waf dist"
VERSION='0'
APPNAME='mapviewer'

# these variables are mandatory ('/' are converted automatically)
top = '.'
out = 'build'

def set_options(opt):
    opt.tool_options('compiler_cc')

def configure(conf):
    conf.check_tool('compiler_cc')
    conf.check(header_name='stdlib.h')
    conf.check(header_name='math.h')

    conf.env.CCFLAGS = ['-O0', '-g3']
    conf.check_cfg(package='gtk+-2.0', uselib_store='GTK', atleast_version='2.6.0', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package = 'osmgpsmap', uselib_store='OSMGPSMAP', atleast_version = '0.7.2', args = '--cflags --libs')
    #conf.check_cc(lib='osmgpsmap', uselib_store='OSMGPSMAP')    
    conf.check_cc(lib='fap', uselib_store='FAP')
def build(bld):
    # aprsmap
    bld(
        features = 'cc cprogram',
        source = bld.path.ant_glob('**/*.c'),
        #source = ('audio_jack.c filter.c gui.c lysdr.c sdr.c waterfall/waterfall.c'),
        #include = ['.', './waterfall',],
        target = 'aprsmap',
        uselib = "GTK OSMGPSMAP FAP",
        includes = '. /usr/include')

