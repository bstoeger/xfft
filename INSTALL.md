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
The `xfft.pro` file might have to be adapted. In particular, if the code should be
executed on a different computer than it is compiled on, the `-march=native` option
should be removed. This option makes the compiler use all the CPU features of the
local CPU for optimizations. However, these features may not be present on every
CPU.

On Windows, I was successful in compiling and running the program using the following
sequence of steps:

* Install Qt Creator.
* Clone repository.
* Download and unpack the boost library ([https://www.boost.org/users/download/] (https://www.boost.org/users/download/)).
* Copy the 'boost' directory in the main directory of the repository.
* Download and unpack the precompiled version of fftw3 for windows ([http://www.fftw.org/install/windows.html] (http://www.fftw.org/install/windows.html)).
* Copy the fftw3.h file to the main directory of the repository.
* Copy the libfftw3-3.def and libfftw3-3.dll files to the main directory of the repository.
* Open the xfft.pro file in Qt Creator.
* Change to Release mode.
* Build the project in Qt Creator.
* Run from the build menu in Qt Creator.

To make a distributable directory, DLLs have to be copied in the release directory. I was
successful using the following steps (YMMV):
* Run windeployqt to copy the Qt libraries. In my case the correct command was
`c:\Qt\6.3.1\mingw_64\bin\windeployqt.exe "c:\Users\stoeger\src\build-xfft-Desktop_Qt_6_3_1_MinGW_64_bit-Release\release"`
* Copy the gcc DLLs from `Qt\x.x.x\mingw_64\bin\` into the release directory. In my case,
I had to copy `libgcc_s_seh-1.dll`, `libstdc++-6.dll` and `libwinpthread-1.dll`.
* Copy the `libfftw3-3.dll` file to the release directory.
* The release directory can now be copied and the `xfft` executable run directly from the directory.

Finally, an installer can be built with NSIS ([https://nsis.sourceforge.io/Download] (https://nsis.sourceforge.io/Download)):
* Manually copy the `xfft.nsi` (in `packaging/windows`), `xfft.ico` and `LICENCE` files into the release directory.
* Enter the release directory and run NSIS for example as such: `"c:\Program Files (x86)\NSIS\makensis.exe" xfft.nsi`
* If all works out fine, the installer will be written into the parent directory.