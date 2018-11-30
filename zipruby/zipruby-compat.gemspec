Gem::Specification.new do |spec|
  spec.name              = 'zipruby-compat'
  spec.version           = '0.3.7'
  spec.summary           = 'Ruby bindings for libzip.'
  spec.files             = Dir.glob('ext/*.*') + %w(ext/extconf.rb README.txt zipruby.c LICENSE.libzip ChangeLog)
  spec.author            = 'jawspeak'
  spec.email             = 'jaw@jawspeak.com'
  spec.homepage          = 'https://github.com/jawspeak/zipruby-compatibility-with-rubyzip-fork'
  spec.extensions        = 'ext/extconf.rb'
end
