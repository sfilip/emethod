
set(cmake_generated ${CMAKE_BINARY_DIR}/CMakeCache.txt
                    ${CMAKE_BINARY_DIR}/cmake_install.cmake  
                    ${CMAKE_BINARY_DIR}/Makefile
                    ${CMAKE_BINARY_DIR}/CMakeFiles
                    ${CMAKE_BINARY_DIR}/CPackConfig.cmake
                    ${CMAKE_BINARY_DIR}/CPackSourceConfig.cmake

                    ${CMAKE_BINARY_DIR}/../efrac/build
                    ${CMAKE_BINARY_DIR}/../efrac/Makefile

                    ${CMAKE_BINARY_DIR}/../flopoco/cmake_install.cmake  
                    ${CMAKE_BINARY_DIR}/../flopoco/Makefile
                    ${CMAKE_BINARY_DIR}/../flopoco/src/CMakeFiles
                    ${CMAKE_BINARY_DIR}/../flopoco/src/random/cmake_install.cmake  
                    ${CMAKE_BINARY_DIR}/../flopoco/src/random/Makefile
                    ${CMAKE_BINARY_DIR}/../flopoco/src/random/CMakeFiles
)

foreach(file ${cmake_generated})
  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()
endforeach(file)

