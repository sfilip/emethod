
find_package(Boost 
	REQUIRED 
	COMPONENTS
    	program_options
    )

IF ( Boost_FOUND )
	MESSAGE ( STATUS "Found Boost version ${Boost_LIB_VERSION}: ${Boost_INCLUDE_DIRS} ${Boost_LIBRARY_DIRS}" )
ELSE ( Boost_FOUND )
	MESSAGE ( STATUS "Could not find Boost" )
ENDIF ( Boost_FOUND )
