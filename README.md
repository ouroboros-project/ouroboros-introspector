HOW TO BUILD
============
 
Assuming that $LLVM_DIR points to a directory with LLVM+Clang as described in http://clang.llvm.org/get_started.html
(no need to compile Clang at this point), and that $OPI_DIR points to the directory you cloned this repository, do:
 
 ```
 $ mkdir opi-build && cd opi-build
 $ cmake "$LLVM_DIR" -DLLVM_EXTERNAL_CLANG_TOOLS_EXTRA_SOURCE_DIR="$OPI_DIR"
 ```

Compile target *ouroboros-introspector*, which in turn will compile the needed Clang tools.


REQUIREMENTS
============
 
* Building Clang with CMake, check it's documentation.
* This has only been tested against Clang 3.5
 
 
USAGE
=====
 
```
 $ opi <PATH_TO_HEADER> <PATH_TO_HEADER> .. -- -x c++
```
 
