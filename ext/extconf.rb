require 'mkmf'

$CFLAGS << ' -std=c99'

unless enable_config('packaged-library')
  pkg_config('libgumbo')
end

if enable_config('packaged-library') || !have_library('gumbo', 'gumbo_parse')
  gumbo_lib_src = File.expand_path('../../vendor/gumbo-parser/src', __FILE__)
  unless File.directory? gumbo_lib_src
    abort "Couldn't find the packaged gumbo-parser library. " +
          "Did you forget to git clone --recursive?"
  end
  require 'fileutils'

  # mkmf doesn't appear to deal well with sources/objects in multiple
  # directories, so we bring the gumbo source to it.
  gumbo_sources = Dir[File.join(gumbo_lib_src, '*')]
  FileUtils.cp(gumbo_sources, File.dirname(__FILE__))
end

create_makefile('gumbo_ext')
