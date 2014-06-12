require "mkmf"

EXTENSION_NAME = "gumbo_ext"
USE_SYSTEM_LIBS = arg_config('--use-system-libraries', ENV['RUBY_GUMBO_USE_SYSTEM_LIBRARIES'])
LIBGUMBO_PACKAGE = "https://github.com/google/gumbo-parser/archive/3a61e9ad963cacfb3246468feab28c5058f621c1.zip"
LIBGUMBO_PACKAGE_VERSION = "1.0-2014-05-03-3a61e9"

RbConfig::MAKEFILE_CONFIG["CC"] = ENV["CC"] if ENV["CC"]
$CFLAGS << " -std=c99"

if USE_SYSTEM_LIBS
  unless pkg_config("libgumbo")
    $libs << " -lgumbo"
  end
else
    message <<-end_message
************************************************************************
IMPORTANT!  ruby-gumbo builds and uses a packaged verison of libgumbo.

If this is a concern for you and you want to use the system library
instead, abort this installation process and reinstall ruby-gumbo as
follows:

    gem install ruby-gumbo -- --use-system-libraries

Or, if you are using Bundler, tell it to use the option:

    bundle config build.ruby-gumbo --use-system-libraries
    bundle install
************************************************************************
end_message

  require "mini_portile"
  libgumbo = MiniPortile.new("libgumbo", LIBGUMBO_PACKAGE_VERSION)
  libgumbo.target = File.expand_path("../../ports", __FILE__)
  libgumbo.files = [LIBGUMBO_PACKAGE]

  libgumbo.download unless libgumbo.downloaded?
  libgumbo.extract
  libgumbo.patch
  libgumbo.send(:execute, "autogen", %Q(sh autogen.sh))
  libgumbo.configure unless libgumbo.configured?
  libgumbo.compile
  libgumbo.install unless libgumbo.installed?
  libgumbo.activate

  pkgconfig_path = File.join(libgumbo.path, "lib", "pkgconfig")
  ENV["PKG_CONFIG_PATH"] = "#{pkgconfig_path}:#{ENV["PKG_CONFIG_PATH"]}"

  find_header("gumbo.h")
  unless pkg_config("gumbo")
    abort 'No package'
  end
end

create_header
create_makefile(EXTENSION_NAME)
