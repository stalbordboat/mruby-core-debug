require_relative 'Tasks/spec.rb'
require          'rake/clean'

CC          = 'cc'
CFLAGS      = [
                '-std=gnu99',
                #'-g',
                '-fpic',
                '-O3',
                #'-Wall',
                '-Wundef',
                '-Werror-implicit-function-declaration',
                '-Wwrite-strings',
                '-I include',
                '-D MRB_NO_GEMS',   # Mandatory
                '-D MRB_NO_PRESYM', # Mandatory
                '-D MRB_UTF8_STRING',
                #'-D MRB_USE_DEBUG_HOOK',
              ]
SRC_DIRNAME = 'src'
BIN         = 'mrbc'
LIB         = 'libmruby-core-debug.a'
LIBNAME     = File.basename(LIB, '.a').split('lib').last
AR          = 'ar'
AR_FLAGS    = 'rcs'
PC          = 'mruby-core-debug.pc'

PREFIX      = "#{ENV['HOME']}/.local"
BIN_PREFIX  = "#{PREFIX}/bin"
INC_PREFIX  = "#{PREFIX}/include/GameSDK"
LIB_PREFIX  = "#{PREFIX}/lib"
PC_PREFIX   = "#{PREFIX}/lib/pkgconfig"

Task::LibBuild.new do |spec|
  spec.cc            = CC
  spec.cflags        = CFLAGS
  spec.sources       = FileList["#{SRC_DIRNAME}/lib_*.c"]
  spec.ar            = AR
  spec.ar_flags      = AR_FLAGS
  spec.lib           = LIB
  spec.pc            = PC
  spec.pc_content    = %Q[prefix=#{PREFIX}
exec_prefix=${prefix}
includedir=${prefix}/include/GameSDK
libdir=${exec_prefix}/lib

Name: #{LIBNAME}
Description: A fork of mruby for GameSDK.
Version: 3.4.0
Cflags: -I${includedir}
Libs: -L${libdir} -l#{LIBNAME} -lm]
end

Task::BinBuild.new do |spec|
  spec.cc       = CC
  spec.cflags   = CFLAGS
  spec.sources  = Dir["#{SRC_DIRNAME}/bin_*.c"]
  spec.bin      = BIN
  spec.lib      = LIB
end

Task::Install.new do |spec|
  spec.bin_src  = BIN
  spec.bin_dest = BIN_PREFIX
  spec.inc_src  = ['include/mruby', 'include/mruby.h', 'include/mrbconf.h']
  spec.inc_dest = INC_PREFIX
  spec.lib_src  = LIB
  spec.lib_dest = LIB_PREFIX
  spec.pc_src   = PC
  spec.pc_dest  = PC_PREFIX
end

Task::Uninstall.new do |spec|
  spec.bin      = BIN
  spec.bin_path = BIN_PREFIX
  spec.inc      = ['mruby', 'mruby.h', 'mrbconf.h']
  spec.inc_path = INC_PREFIX
  spec.lib      = LIB
  spec.lib_path = LIB_PREFIX
  spec.pc       = PC
  spec.pc_path  = PC_PREFIX
end

CLEAN.include("#{SRC_DIRNAME}/*.o", BIN, LIB, PC)
