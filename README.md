## Why fork zipruby?
* Both rubyzip http://rubygems.org/gems/rubyzip and zipruby http://rubygems.org/gems/zipruby define Zip::File
* I didn't want to touch anything using rubyzip, but allow both libraries to coexist peacefully. 
* A previous bug report: https://bitbucket.org/winebarrel/zip-ruby/issue/1/problems-with-zipruby-and-zip-fileexists 

#### Rename Zip::* to ZipRuby::* module.
This is so that zipruby can interoperate with rubyzip. Previously, both defined Zip::File.
    
That was not cool, because if you had a project that used rubyzip, when you introduced rubyzip, then it will no longer work. You'd get errors like this: "undefined method `exists?' for Zip::File:Class"
    
Another workaround is https://github.com/aussiegeek/rubyzip/pull/6 where they describe changing rubyzip.
