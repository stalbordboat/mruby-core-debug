## mruby-core-debug
This is a fork of mruby, that maintains full compadibililty with mruby 3.4.0.
This removes mruby's complex build system, and with that goes mrbgems, and presyms.
The aim is to makes mruby more like Lua in the sense that it's a small repo that just
builds its static library and bytecode compiler.

This is called **mruby-core-debug** because it's purpose is to be the Ruby virtual machine, and core API and debugging API
for **The Game SDK**.

#### Build and Install
To build you need:

+ [*Ruby*](https://www.ruby-lang.org/en/)
+ [*Rake*](https://rubygems.org/gems/rake)

This will install to your `$HOME/.local` by default, edit `Rakefile` to change that.

#### *Build*
```console
rake
```

#### *Install*
```console
rake install
```

For more tasks

```console
rake -T
```

