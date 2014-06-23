require 'rake/clean'
require 'rubygems/package_task'
require 'yard'

VERSION = '1.1.0'

BUILT_EXTENSION = "ext/gumbo_ext.#{RbConfig::CONFIG['DLEXT']}"
BUILT_FILES = FileList[
  BUILT_EXTENSION,
]
EXTENSION_SOURCE_FILES = FileList[
  'ext/extconf.rb',
  'ext/ruby_gumbo*.{h,c}',
]
SOURCE_FILES = FileList[
  'Rakefile',
  'LICENSE',
  'README.mkd',
  'lib/**/*.rb',
  *EXTENSION_SOURCE_FILES,
]
VENDOR_FILES = FileList[
  'vendor/gumbo-parser/src/*',
]
PACKAGED_FILES = FileList[
  *BUILT_EXTENSION,
  *SOURCE_FILES,
  *VENDOR_FILES
]

# Building

task :build => BUILT_EXTENSION

# Note that this will fail to pick up new files; you'll want to rake clean
# after adding/remove files. (The trade off is that versus rebuilding the
# Makefile each time an extension source file is touched).
file 'ext/Makefile' => ['ext/extconf.rb'] + VENDOR_FILES do
  Dir.chdir 'ext' do
    ruby 'extconf.rb'
  end
end

file BUILT_EXTENSION => ['ext/Makefile'] + EXTENSION_SOURCE_FILES do
  Dir.chdir 'ext' do
    sh 'make'
  end
end

# Documentation

YARD::Rake::YardocTask.new(:doc)

# Packaging

SPEC = Gem::Specification.new do |spec|
    spec.name    = 'ruby-gumbo'
    spec.version = VERSION
    spec.summary = 'Ruby bindings for the gumbo html5 parser'
    spec.authors = ['Nicolas Martyanoff', 'Ian MacLeod']
    spec.email   = ['khaelin@gmail.com', 'ian@nevir.net']
    spec.license = 'ISC'

    spec.files      = SOURCE_FILES + VENDOR_FILES
    spec.extensions = 'ext/extconf.rb'

    spec.required_ruby_version = '>= 1.9.3'
end

Gem::PackageTask.new(SPEC) do |pkg|
  pkg.need_tar = true
  pkg.need_zip = true
end

# Cleaning

CLEAN.include('ext/**/*', '.yardoc')
CLEAN.exclude(*SOURCE_FILES, *BUILT_FILES)

CLOBBER.include('doc', *BUILT_FILES)
