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
    
    #conf.env.CCFLAGS = ['-O0', '-g3']
    #conf.env.CCFLAGS +=  ['-DG_DISABLE_SINGLE_INCLUDES','-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES', '-DGTK_DISABLE_SINGLE_INCLUDES']
    #conf.env.CCFLAGS +=  ["-DG_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"]
    #conf.env.CCFLAGS += ["-DGSEAL_ENABLE"]

    conf.check_cfg(package='gtk+-2.0', uselib_store='GTK', atleast_version='2.6.0', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package = 'osmgpsmap', uselib_store='OSMGPSMAP', atleast_version = '0.7.2', args = '--cflags --libs')
    conf.check_cfg(package = 'libfap', uselib_store='FAP', atleast_version = '0.9', args = '--cflags --libs')
    
def build(bld):
    # 1. A simple program
    bld(
        features = 'cc cprogram',
        source = bld.path.ant_glob('**/*.c'),
        #source = ('audio_jack.c filter.c gui.c lysdr.c sdr.c waterfall/waterfall.c'),
        #include = ['.', './waterfall',],
        target = 'aprsmap',
        uselib = "GTK OSMGPSMAP FAP",
        includes = '. /usr/include')

