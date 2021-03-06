FIND_PATH(MPREAL_INCLUDE
	  NAMES
	  mpreal.h
	  PATHS
      $ENV{MPREALDIR}
      ${INCLUDE_INSTALL_DIR}
)

IF (MPREAL_INCLUDE)
    SET(MPREAL_FOUND TRUE)
ENDIF (MPREAL_INCLUDE)

IF ( MPREAL_FOUND )
	MESSAGE ( STATUS "Found MPFRC++ : ${MPREAL_INCLUDE}" )
ELSE ( MPREAL_FOUND )
	MESSAGE ( STATUS "Could not find MPFRC++" )
ENDIF ( MPREAL_FOUND )