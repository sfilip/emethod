find_path(FPLLL_INCLUDE
    NAMES
    fplll.h
    PATHS
    $ENV{FPLLLDIR}
    ${INCLUDE_INSTALL_DIR}
)

find_library(FPLLL_LIBRARY 
	NAMES
	fplll 
	PATHS 
	$ENV{FPLLLDIR} 
	${LIB_INSTALL_DIR}
)


IF (FPLLL_INCLUDE AND FPLLL_LIBRARY)
    SET(FPLLL_FOUND TRUE)
ENDIF (FPLLL_INCLUDE AND FPLLL_LIBRARY)

IF ( FPLLL_FOUND )
	MESSAGE ( STATUS "Found FPLLL : ${FPLLL_INCLUDE} ${FPLLL_LIBRARY}" )
ELSE ( FPLLL_FOUND )
	MESSAGE ( STATUS "Could not find FPLLL" )
ENDIF ( FPLLL_FOUND )

mark_as_advanced(FPLLL_INCLUDE FPLLL_LIBRARY)


# find_path(FPLLL_INCLUDE
#    NAMES
#    fplll.h
#    PATHS
#    ${INCLUDE_INSTALL_DIR}
# )

# find_library(FPLLL_LIBRARY fplll PATHS ${LIB_INSTALL_DIR})

# include(FindPackageHandleStandardArgs)
# find_package_handle_standard_args(FPLLL DEFAULT_MSG
#                                 FPLLL_INCLUDE FPLLL_LIBRARY)
# mark_as_advanced(FPLLL_INCLUDE FPLLL_LIBRARY)