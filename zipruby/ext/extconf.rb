require 'mkmf'

if have_header('zlib.h') and have_library('z')
  have_func('fseeko')
  have_func('ftello')
  have_func('mkstemp')
  create_makefile('zipruby')
end
