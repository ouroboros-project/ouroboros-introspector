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
  
add_custom_command(TARGET clang-introspector
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} 
                   -DINTROSPECTOR_PATH=$<TARGET_FILE:clang-introspector>
                   -DINPUT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/test-inputs
                   -DOUTPUT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/test-outputs
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/PrepareRunTest.cmake
                   )