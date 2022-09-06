Installing XFFT
===============

Currently, there are no precompiled packages, which means that you will
have to compile from source. The software is developed and tested under
Linux, though all dependencies should be available under Windows and MacOS.

Dependencies
------------

* `Qt` core, widgets and svg ([https://www.qt.io](https://www.qt.io))
* `FFTW` ([https://www.fftw.org](https://www.fftw.org))
* `boost` operators and heap ([https://www.boost.org/](https://www.boost.org/))

On Linux, all these are available as package on every reasonable distribution.
For instance, on Debian based distributions, such as Ubuntu, the following
commands will install the necessary dependencies:
	apt install qtbase5-dev
	apt install libfftw3-dev
	apt install libboost-dev

Under Windows, installing QtCreator should install the Qt infrastructure.
The boost libraries (operators and heap) are header only libraries and can
simply be put in the build directory. The `FFTW` project provides Windows
binaries.

C++ compiler
------------

The code is compiled using recent `g++` ([https://gcc.gnu.org/](https://gcc.gnu.org/))
and `clang` ([https://clang.llvm.org/](https://clang.llvm.org/)). The code is
compiled using C++20, though makes very little use of C++20 features,
besides the odd standard library function. It does use C++17 constructs
such as structured bindings rather heavily, though.

Compiling
---------

The build system is `qmake` provided by `Qt`. On Linux, the programm can be compiled
with the sequence of commands (executed from the main directory):
	qmake
	make
The `xfft.pro` file might have to be adapted.
