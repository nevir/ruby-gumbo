
require 'rake/clean'

require 'rdoc/task'

require 'rubygems/package_task'


PKG_NAME = "ruby-gumbo"
PKG_VERSION = "1.0.1"

EXT_CONF = "ext/extconf.rb"
MAKEFILE = "ext/Makefile"
MODULE = "ext/gumbo.so"
SRC = Dir.glob("ext/*.c") << MAKEFILE

CLEAN.include [MODULE, "ext/*.o"]
CLOBBER.include ["ext/mkmf.log", "ext/extconf.h", MAKEFILE]

# Build
file MAKEFILE => EXT_CONF do |t|
  Dir::chdir(File::dirname(EXT_CONF)) do
    unless sh "ruby #{File::basename(EXT_CONF)}"
      $stderr.puts "extconf.rb failed"
      break
    end
  end
end

file MODULE => SRC do |t|
  Dir::chdir(File::dirname(EXT_CONF)) do
    unless sh "make"
      $stderr.puts "make failed"
      break
    end
  end
end

desc "Build the native library"
task :build => MODULE

# Documentation
RDOC_FILES = FileList["ext/gumbo.c", "lib/gumbo/extra.rb"]

Rake::RDocTask.new do |task|
  #task.main = "README.rdoc"
  task.rdoc_dir = "doc/api"
  task.rdoc_files.include(RDOC_FILES)
end

Rake::RDocTask.new(:ri) do |task|
  #task.main = "README.rdoc"
  task.rdoc_dir = "doc/ri"
  task.options << "--ri-system"
  task.rdoc_files.include(RDOC_FILES)
end

# Packaging
PKG_FILES = FileList["Rakefile", "LICENSE", "README.mkd",
                     "lib/gumbo/*.rb",
                     "ext/extconf.rb", "ext/*.[hc]"]

SPEC = Gem::Specification.new do |spec|
    spec.name = PKG_NAME
    spec.version = PKG_VERSION
    spec.summary = "Ruby bindings for the gumbo html5 parser"
    spec.author = "Nicolas Martyanoff"
    spec.email = "khaelin@gmail.com"
    spec.license = "ISC"

    spec.files = PKG_FILES
    spec.extensions = "ext/extconf.rb"

    spec.required_ruby_version = ">= 1.9.3"
end

Gem::PackageTask.new(SPEC) do |pkg|
    pkg.need_tar = true
end
