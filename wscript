#! /usr/bin/env python
# encoding: utf-8
# a1batross, mittorn, 2018

from __future__ import print_function
from waflib import Logs, Options
import os
import sys

VERSION = '0.6'
APPNAME = 'quakewrapper'
SUBDIRS = [ 'cl_dll', 'dlls', 'mainui' ]
top = '.'

def get_git_version():
	# try grab the current version number from git
	version = 'notset'
	if os.path.exists('.git'):
		try:
			version = os.popen('git describe --dirty --always').read().strip()
		except Exception as e:
			pass

	if(len(version) == 0):
		version = 'notset'

	return version

def options(opt):
	opt.load('compiler_cxx compiler_c')
	if sys.platform == 'win32':
		opt.load('msvc msvs')

	opt.add_option(
		'--64bits', action = 'store_true', dest = 'ALLOW64', default = False,
		help = 'allow targetting 64-bits')

	opt.add_option(
		'--release', action = 'store_true', dest = 'RELEASE', default = False,
		help = 'strip debug info from binary and enable optimizations')

	opt.add_option(
		'--no-gcc-colors', action = 'store_false', dest = 'GCC_COLORS', default = True,
		help = 'do not enable gcc colors')

	opt.recurse(SUBDIRS)

def configure(conf):
	conf.env.MSVC_TARGETS = ['x86'] # explicitly request x86 target for MSVC
	conf.load('compiler_cxx compiler_c')
	if sys.platform == 'win32':
		conf.load('msvc msvs')

	# Check if we have 64-bit toolchain
	conf.env.DEST_64BIT = False # predict state
	try:
		conf.check_cc(
			fragment='''int main( void )
			{
				int check[sizeof(void*) == 4 ? 1: -1];
				return 0;
			}''',
			msg          = 'Checking if compiler create 32 bit code')
	except conf.errors.ConfigurationError:
		# Program not compiled, we have 64 bit 
		conf.env.DEST_64BIT = True

	if(conf.env.DEST_64BIT):
		if(not conf.options.ALLOW64):
			conf.env.append_value('LINKFLAGS', ['-m32'])
			conf.env.append_value('CFLAGS', ['-m32'])
			conf.env.append_value('CXXFLAGS', ['-m32'])
			Logs.info('NOTE: will build with 64-bit toolchain using -m32')
		else:
			Logs.warn('WARNING: 64-bit build may be unstable')

	if(conf.env.COMPILER_CC != 'msvc'):
		if(conf.env.COMPILER_CC == 'gcc'):
			conf.env.append_unique('LINKFLAGS', ['-Wl,--no-undefined'])
		if(conf.options.RELEASE):
			conf.env.append_unique('CFLAGS',   ['-O2'])
			conf.env.append_unique('CXXFLAGS', ['-O2'])
		else:
			conf.env.append_unique('CFLAGS',   ['-Og', '-g'])
			conf.env.append_unique('CXXFLAGS', ['-Og', '-g'])
		if conf.options.GCC_COLORS:
			conf.env.append_unique('CFLAGS', ['-fdiagnostics-color=always'])
			conf.env.append_unique('CXXFLAGS', ['-fdiagnostics-color=always'])
	else:
		if(conf.options.RELEASE):
			conf.env.append_unique('CFLAGS',   ['/O2'])
			conf.env.append_unique('CXXFLAGS', ['/O2'])
		else:
			conf.env.append_unique('CFLAGS',   ['/Z7'])
			conf.env.append_unique('CXXFLAGS', ['/Z7'])
			conf.env.append_unique('LINKFLAGS', ['/DEBUG'])
		conf.env.append_unique('DEFINES', '_USING_V110_SDK71_') # Force XP compability

	# Force XP compability, all build targets should add
	# subsystem=bld.env.MSVC_SUBSYSTEM
	# TODO: wrapper around bld.stlib, bld.shlib and so on?
	conf.env.MSVC_SUBSYSTEM = 'WINDOWS,5.01'

	if(conf.env.DEST_OS != 'win32'):
		conf.check( lib='dl' )
		conf.check( lib='m' )
		conf.check( lib='pthread' )

	conf.start_msg('Checking git hash')
	git_version = get_git_version()
	conf.end_msg(git_version)
	conf.env.append_unique('DEFINES', 'BUILD_COMMIT="' + git_version + '"')

	for i in SUBDIRS:
		conf.setenv(i, conf.env) # derive new env from global one
		conf.env.ENVNAME = i
		conf.msg(msg='Configuring ' + i, result='in progress', color='BLUE')
		# configure in standalone env
		conf.recurse(i)
		conf.msg(msg='Configuring ' + i, result='done', color='BLUE')
		conf.setenv('')

def build(bld):
	for i in SUBDIRS:
		bld.recurse(SUBDIRS)
