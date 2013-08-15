
require "mkmf"

RbConfig::MAKEFILE_CONFIG["CC"] = ENV["CC"] if ENV["CC"]

extension_name = "gumbo"

unless pkg_config("libgumbo")
  $libs << " -lgumbo"
end

create_header
create_makefile(extension_name)
