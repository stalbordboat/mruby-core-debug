# Reference Documentation: https://ruby.github.io/rake/Rake/DSL.html
require 'rake/dsl_definition'
require 'rake/file_list'
include Rake::DSL

module Task
  # Defined in Build#initialize.
  task :default => :all
end

class Task::LibBuild
  attr_accessor :cc
  attr_accessor :cflags
  attr_accessor :ar
  attr_accessor :ar_flags
  attr_accessor :sources
  attr_accessor :lib
  attr_accessor :pc
  attr_accessor :pc_content

  def initialize
    yield(self)

    @objects  = @sources.ext('.o')
    @cflags   = @cflags.join(' ')

    desc 'Builds all source files'
    task :all => @objects do
      # Compile the static library.
      sh "#{@ar} #{@ar_flags} #{@lib} #{@objects.join(' ')}"

      # Build the compiler.
      Rake::Task[:bin_build].invoke

      # Write pkgconf file.
      File.write(@pc, @pc_content)
    end

    rule '.o' => '.c' do |file_task|
      src      = file_task.source
      obj      = "#{File.basename(src, '.c')}.o"
      obj_path = "src/#{obj}"

      # Compile individual object files.
      sh "#{@cc} #{@cflags} -c #{src} -o #{obj_path}"
    end
  end
end

class Task::BinBuild
  attr_accessor :cc
  attr_accessor :cflags
  attr_accessor :sources
  attr_accessor :bin
  attr_accessor :lib

  def initialize
    yield(self)

    @sources = @sources.join(' ')
    @cflags  = @cflags.join(' ')

    desc 'Make the build'
    task :bin_build do
      sh "#{@cc} #{@cflags} #{@sources} -o #{@bin} #{@lib} -lm"
    end
  end
end

class Task::Install
  attr_accessor :bin_src
  attr_accessor :bin_dest
  attr_accessor :inc_src
  attr_accessor :inc_dest
  attr_accessor :lib_src
  attr_accessor :lib_dest
  attr_accessor :pc_src
  attr_accessor :pc_dest

  def initialize
    yield(self)

    desc 'Moves install files to the install path'
    task :install do |task|
      verbose = !task.application.options.silent

      # Install Executable

      FileUtils.mkdir_p(@bin_dest, verbose: verbose)

      FileUtils.install(@bin_src, @bin_dest, verbose: verbose)

      # Install Includes

      FileUtils.mkdir_p(@inc_dest, verbose: verbose)

      @inc_src.each do |path|
        FileUtils.cp_r(path, @inc_dest, verbose: verbose) if File.directory?(path)
        FileUtils.cp(path, @inc_dest, verbose: verbose)   if File.file?(path)
      end

      # Install Library

      FileUtils.mkdir_p(@lib_dest, verbose: verbose)
      FileUtils.mkdir_p(@pc_dest, verbose: verbose)

      FileUtils.cp(@lib_src, @lib_dest, verbose: verbose)
      FileUtils.cp(@pc_src, @pc_dest, verbose: verbose)
    end
  end
end

class Task::Uninstall
  attr_accessor :bin
  attr_accessor :bin_path
  attr_accessor :inc
  attr_accessor :inc_path
  attr_accessor :lib
  attr_accessor :lib_path
  attr_accessor :pc
  attr_accessor :pc_path

  def initialize
    yield(self)

    desc 'Removes install files from the install path'
    task :uninstall do |task|
      verbose = !task.application.options.silent

      # Remove Executable

      FileUtils.cd(@bin_path, verbose: verbose) do
        FileUtils.rm_f(@bin, verbose: verbose)
      end

      # Remove Includes

      FileUtils.cd(@inc_path, verbose: verbose) do
        @inc.each do |path|
          FileUtils.rm_rf(path, verbose: verbose) if File.directory?(path)
          FileUtils.rm(path, verbose: verbose)    if File.file?(path)
        end
      end

      # Remove PC File

      FileUtils.rm("#{@lib_path}/#{@lib}", verbose: verbose);
      FileUtils.rm("#{@pc_path}/#{@pc}", verbose: verbose);
    end
  end
end
