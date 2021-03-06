![C/C++ CI of Cli](https://github.com/daniele77/cli/workflows/C/C++%20CI%20of%20Cli/badge.svg)

# cli

A cross-platform header only C++14 library for interactive command line interfaces (Cisco style)

![demo_local_session](https://user-images.githubusercontent.com/5451767/51046611-d1dadc00-15c6-11e9-8a0d-2c66efc83290.gif)

![demo_telnet_session](https://user-images.githubusercontent.com/5451767/51046612-d1dadc00-15c6-11e9-83c2-beadb3593348.gif)

![C/C++ CI of Cli](https://github.com/daniele77/cli/workflows/C/C++%20CI%20of%20Cli/badge.svg)

## Features

* Header only
* Cross-platform (linux and windows)
* Menus and submenus
* Remote sessions (telnet)
* Persistent history (navigation with arrow keys)
* Autocompletion (with TAB key)
* Async interface
* Colors

## How to get CLI library

* From [GitHub](https://github.com/daniele77/cli/releases)
* Using [Vcpkg](https://github.com/Microsoft/vcpkg)

## Dependencies

The library depends on asio (either the standalone version or the boost version)
only to provide telnet server.
Therefore, the library has no dependencies if you don't need remote sessions.

## Installation

The library is header-only: it consists entirely of header files
containing templates and inline functions, and require no separately-compiled
library binaries or special treatment when linking.

Extract the archive wherever you want.

Now you must only remember to specify the cli (and optionally asio or boost) paths when
compiling your source code.

If you fancy it, a Cmake script is provided. To install you can use:

    mkdir build && cd build
    cmake ..
    sudo make install

and, if you want to specify the installation path:

    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX:PATH=<cli_install_location>
    make install

## Compilation of the examples

You can find some examples in the directory "examples".
Each .cpp file corresponds to an executable. You can compile each example by including
cli (and optionally asio/boost header files) 
and linking pthread on linux (and optionally boost system).

To compile the examples using cmake, use:

    mkdir build
    cd build
    cmake .. -DCLI_BuildExamples=ON
    # or: cmake .. -DCLI_BuildExamples=ON -DBOOST_INCLUDEDIR=<boost_include_directory>
    make all
    # or: cmake --build .

In the same directory you can also find:

* a GNU make file (Makefile)
* a Windows nmake file (makefile.win)
* a Visual Studio solution

You can specify boost library path in the following ways:

### GNU Make

    make CXXFLAGS="-isystem <boost_include>" LDFLAGS="-L<boost_lib>"

example:

    make CXXFLAGS="-isystem /opt/boost_1_66_0/install/x86/include" LDFLAGS="-L/opt/boost_1_66_0/install/x86/lib"

(if you want to use clang instead of gcc, you can set the variable CXX=clang++)

### Windows nmake

Optionally set the environment variable ASIO or BOOST to provide the library path.
Then, from a visual studio console, start `nmake` passing one of the `makefile.*.win` files.

E.g., from a visual studio console, use one of the following commands:

    # only compile examples that do not require asio
    nmake /f makefile.noasio.win
    # compile examples using boost asio
    set BOOST=<path of boost libraries>
    nmake /f makefile.boostasio.win
    # compile examples using standalone asio
    set ASIO=<path of asio library>
    nmake /f makefile.standaloneasio.win

### Visual Studio solution

Set the environment variable BOOST. Then, open the file
`cli/examples/examples.sln`

## CLI usage

The cli interpreter can manage correctly sentences using quote (') and double quote (").
Any character (spaces too) comprises between quotes or double quotes are considered as a single parameter of a command.
The characters ' and " can be used inside a command parameter by escaping them with a backslash.

Some example:

    cli> echo "this is a single parameter"
    this is a single parameter
    cli> echo 'this too is a single parameter'
    this too is a single parameter
    cli> echo "you can use 'single quotes' inside double quoted parameters"
    you can use 'single quotes' inside double quoted parameters
    cli> echo 'you can use "double quotes" inside single quoted parameters'
    you can use "double quotes" inside single quoted parameters
    cli> echo "you can escape \"quotes\" inside a parameter"               
    you can escape "quotes" inside a parameter
    cli> echo 'you can escape \'single quotes\' inside a parameter'
    you can escape 'single quotes' inside a parameter
    cli> echo "you can also show backslash \\ ... "                
    you can also show backslash \ ... 

## License

Distributed under the Boost Software License, Version 1.0.
(See accompanying file [LICENSE.txt](LICENSE.txt) or copy at
<http://www.boost.org/LICENSE_1_0.txt>)

## Contact

Please report issues here:
<http://github.com/daniele77/cli/issues>

and questions, feature requests, ideas, anything else here:
<http://github.com/daniele77/cli/discussions>

You can always contact me via twitter at @DPallastrelli

---

## Contributing (We Need Your Help!)

Any feedback from users and stakeholders, even simple questions about
how things work or why they were done a certain way, carries value
and can be used to improve the library.

Even if you just have questions, asking them in [GitHub Discussions](http://github.com/daniele77/cli/discussions)
provides valuable information that can be used to improve the library - do not hesitate,
no question is insignificant or unimportant!

If you or your company uses cli library, please consider becoming a sponsor to keep the project strong and dependable.
