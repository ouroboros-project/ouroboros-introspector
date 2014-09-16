set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable(opwig
  Opwig.cpp
  )
target_link_libraries(opwig
  clangTooling
  clangBasic
  clangASTMatchers
  )
