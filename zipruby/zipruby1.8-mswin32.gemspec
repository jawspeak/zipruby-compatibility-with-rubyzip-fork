# @jaw 2012-01-08 not tested in this fork

Gem::Specification.new do |spec|
  spec.name              = 'zipruby1.8'
  spec.version           = '0.3.6'
  spec.platform          = 'mswin32'
  spec.summary           = 'Ruby bindings for libzip.'
  spec.require_paths     = %w(lib/i386-mswin32)
  spec.files             = %w(lib/i386-mswin32/zipruby.so README.txt zipruby.c LICENSE.libzip ChangeLog)
  spec.author            = 'winebarrel'
  spec.email             = 'sgwr_dts@yahoo.co.jp'
  spec.homepage          = 'http://zipruby.rubyforge.org'
  spec.rubyforge_project = 'zipruby'
end
