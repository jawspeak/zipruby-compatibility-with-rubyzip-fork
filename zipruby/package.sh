#!/bin/sh
VERSION=0.3.6

rm *.gem *.tar.bz2 2> /dev/null
rm -rf doc
echo "/* rdoc source */" > zipruby.c
for i in ext/*.[ch]
do
  tr -d '\r' < $i > $i.x && mv $i.x $i
done
cat ext/*.c >> zipruby.c
cp ../libzip/*.{c,h} ext
rdoc -w 4 -SHN -m README.txt README.txt zipruby.c LICENSE.libzip ChangeLog --title 'Zip/Ruby - Ruby bindings for libzip.'
mkdir work
cp -r * work 2> /dev/null
cd work
tar jcf zipruby-${VERSION}.tar.bz2 --exclude=.svn README.txt *.gemspec ext doc
gem build zipruby.gemspec
gem build zipruby-mswin32.gemspec
gem build zipruby1.8-mswin32.gemspec
cp zipruby-${VERSION}-x86-mswin32.gem zipruby-${VERSION}-mswin32.gem
rm -rf lib
mv lib1.9 lib
gem build zipruby1.9-mswin32.gemspec
rm zipruby.c
cp *.gem *.tar.bz2 ..
cd ..
for i in `ls ../libzip/*.{c,h}`
do
  rm ext/`basename $i`
done
rm -rf work
