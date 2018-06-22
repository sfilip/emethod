FIND_PATH(QSOPT_INCLUDE
	  NAMES
	  QSopt_ex.h
	  PATH_SUFFIXES
	  qsopt_ex
)

IF (QSOPT_INCLUDE)
    SET(QSOPT_FOUND TRUE)
ENDIF (QSOPT_INCLUDE)

IF ( QSOPT_FOUND )
	MESSAGE ( STATUS "Found QSopt : ${QSOPT_INCLUDE}" )
  ADD_DEFINITIONS(-DHAVE_QSOPT)
ELSE ( QSOPT_FOUND )
	MESSAGE ( STATUS "Could not find QSopt" )
ENDIF ( QSOPT_FOUND )


# find_library(QSOPT_LIB
#    names 
#    qsopt_ex libqsopt_ex libqsopt_ex.dylib libqsopt_ex.so
#    hints "${QSOPT_PREFIX_PATH}/lib"
#    doc "Directory for qsopt_ex installation"
# )

# if (QSOPT_LIB)
#    message(STATUS "Found qsopt_ex library in the path")
# else (QSOPT_LIB)
#    message(FATAL_ERROR "Could not find the path for the qsopt_ex library")
# endif (QSOPT_LIB)

