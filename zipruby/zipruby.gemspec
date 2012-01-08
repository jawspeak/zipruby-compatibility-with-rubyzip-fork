Gem::Specification.new do |spec|
  spec.name              = 'zipruby'
  spec.version           = '0.3.6'
  spec.summary           = 'Ruby bindings for libzip.'
  spec.files             = Dir.glob('ext/*.*') + %w(ext/extconf.rb README.txt zipruby.c LICENSE.libzip ChangeLog)
  spec.author            = 'winebarrel'
  spec.email             = 'sgwr_dts@yahoo.co.jp'
  spec.homepage          = 'http://zipruby.rubyforge.org'
  spec.extensions        = 'ext/extconf.rb'
  spec.has_rdoc          = true
  spec.rdoc_options      << '--title' << 'Zip/Ruby - Ruby bindings for libzip.'
  spec.extra_rdoc_files  = %w(README.txt zipruby.c LICENSE.libzip ChangeLog)
  spec.rubyforge_project = 'zipruby'
end
