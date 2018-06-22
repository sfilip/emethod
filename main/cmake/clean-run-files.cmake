
set(run_generated   ${CMAKE_BINARY_DIR}/discCoeffsExample.txt
                    ${CMAKE_BINARY_DIR}/EMethod.vhdl
                    ${CMAKE_BINARY_DIR}/test.input
)

foreach(file ${run_generated})
  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()
endforeach(file)

file(GLOB svg_files *.svg)
foreach(file ${svg_files})
  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()
endforeach(file)