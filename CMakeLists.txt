set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable(ouroboros-introspector
  Introspector.cpp
  )
target_link_libraries(ouroboros-introspector
  clangTooling
  clangBasic
  clangASTMatchers
  )
  
set_target_properties(ouroboros-introspector PROPERTIES 
    OUTPUT_NAME opi
)
  
add_custom_command(TARGET ouroboros-introspector
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} 
                   -DINTROSPECTOR_PATH=$<TARGET_FILE:ouroboros-introspector>
                   -DINPUT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/test-inputs
                   -DOUTPUT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/test-outputs
                   -P ${CMAKE_CURRENT_SOURCE_DIR}/PrepareRunTest.cmake
                   )