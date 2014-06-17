require 'rake/file_list'

Gem::Specification.new do |spec|
  spec.name = "ruby-gumbo"
  spec.version = "1.1.0.rc1"
  spec.summary = "Ruby bindings for the gumbo html5 parser"
  spec.author = "Nicolas Martyanoff"
  spec.email = "khaelin@gmail.com"
  spec.license = "ISC"

  spec.test_files    = spec.files.grep(%r{^(test|spec|features)/})
  spec.files = Rake::FileList[
    "Rakefile",
    "LICENSE",
    "README.mkd",
    "lib/**/*.rb",
    "ext/extconf.rb",
    "ext/*.[hc]"
  ]
  spec.extensions = "ext/extconf.rb"

  spec.required_ruby_version = ">= 1.9.3"

  spec.add_runtime_dependency 'mini_portile', '~> 0.6'
end
