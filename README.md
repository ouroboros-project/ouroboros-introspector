 -- HOW TO BUILD --
 
 Assuming that $LLVM_DIR points to a directory with LLVM+Clang as described in http://clang.llvm.org/get_started.html,
 and that $OPWIG_DIR points to the directory you cloned this repository, do:
 
 $ mkdir opwig-build && cd opwig-build
 $ cmake "$LLVM_DIR" -DLLVM_EXTERNAL_CLANG_TOOLS_EXTRA_SOURCE_DIR="$OPWIG_DIR"
 

 - REQUIREMENTS -
 
 * Building Clang with CMake, check it's documentation.
 * This has only been tested against Clang 3.5
 
 
 -- USAGE --
 
 $ opwig <PATH_TO_HEADER> <PATH_TO_HEADER> .. -- -x c++
 