# Installation #
## Requirements ##
### Linux ###
  * OpenCL 1.1 (AMD APP and Nvidia OpenCL 1.1b tested and seem to work)
  * g++4.4 or later (or any compiler that supports rvalues and variadic templates).
### Windows ###
  * Visual Studio 2012
  * Visual C++ Compiler Nov 2012 CTP
  * Wix (to build MSI)

## Configuration ##
### Linux ###
edit Makefile.inc
  * CXX is the compiler used
  * USERFLAGS are additional things you want to pass to the compiler
  * OpenCLInclude is the directory where OpenCL headers reside

## To build library ##
type make

## To build examples ##
type make examples

## To install ##
Type sudo make install

This copies all the headers and kernel headers into /usr/include/clutil and libclutil.a into /usr/lib

### Windows ###
  * Open clutil.sln. Build solution. Run the resulting clutil.msi
  * Or, just download the msi from the Downloads section.