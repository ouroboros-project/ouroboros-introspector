set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable(clang-introspector
  Introspector.cpp
  )
target_link_libraries(clang-introspector
  clangTooling
  clangBasic
  clangASTMatchers
  )
