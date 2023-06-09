# Set some CMake properties
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)


MESSAGE("================")
MESSAGE("Configuring TPLS")
MESSAGE("================")


SET(ENABLE_SHARED  ON
  CACHE BOOL
  "If set to ON to build shared libs and OFF to build static libs")

IF (ENABLE_SHARED)
  SET(ENABLE_STATIC  OFF)
ELSE()
  SET(ENABLE_STATIC  ON)
ENDIF()


# Check if we are asking for help
IF ( HELP )
  UNSET( HELP CACHE )
  FILE(READ README.txt TMP )
  MESSAGE("")
  MESSAGE("${TMP}")
  RETURN()
ENDIF()


# Set the compilers (must be done before PROJECT)
IF ( NOT CMAKE_C_COMPILER AND C_COMPILER )
  SET( CMAKE_C_COMPILER "${C_COMPILER}" )
ENDIF()
IF ( NOT CMAKE_CXX_COMPILER AND CXX_COMPILER )
  SET( CMAKE_CXX_COMPILER "${CXX_COMPILER}" )
ENDIF()
IF ( NOT CMAKE_Fortran_COMPILER AND Fortran_COMPILER )
  SET( CMAKE_Fortran_COMPILER "${Fortran_COMPILER}" )
ELSEIF ( NOT CMAKE_Fortran_COMPILER AND FORTRAN_COMPILER )
  SET( CMAKE_Fortran_COMPILER "${FORTRAN_COMPILER}" )
ENDIF()


# Set the project name
PROJECT( TPLs NONE )

# Set the install path
IF( PREFIX )
  SET( CMAKE_INSTALL_PREFIX ${PREFIX} )
ELSEIF( INSTALL_DIR )
  SET( CMAKE_INSTALL_PREFIX ${INSTALL_DIR} )
ELSEIF( TPL_INSTALL_DIR )
  SET( CMAKE_INSTALL_PREFIX ${TPL_INSTALL_DIR} )
ELSE( CMAKE_INSTALL_PREFIX )
  SET( CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
ENDIF()

#LEGACY DIR STRUCTURE
MESSAGE("Installing TPLs in: ${CMAKE_INSTALL_PREFIX}")

SET( BOOST_VERSION    1.55.0  )
SET( HDF5_VERSION     1.10.1  )
SET( LAPACK_VERSION   3.5.0   )
SET( PETSC_VERSION    3.12.4  )
SET( NETCDF_VERSION   4.7.3   )
SET( SILO_VERSION     4.10.2  )
SET( ZLIB_VERSION     1.2.7   )
SET( SWIG_VERSION     1.2.7   )

# Create a cmake file that can be included from other projects
FILE( WRITE "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "# This file sets cmake variables that specify the locations of TPLs\n")
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "# It may also include comments for record information on how the TPLs were compiled\n")


# Enable the desired languages
ENABLE_LANGUAGE(C)
ENABLE_LANGUAGE(CXX)
ENABLE_LANGUAGE(Fortran)


# Include CMake files
INCLUDE(ExternalProject)
INCLUDE(macros.cmake)


# SET the global compile and link flags
IDENTIFY_COMPILER()
SET_COMPILE_FLAGS()


# Set TPL list
SET(TPL_LIST_FULL_SORTED "ZLIB;HDF5;LAPACK;NETCDF;PETSC;SILO;BOOST")
SET(TPL_LIST "${TPL_LIST_FULL_SORTED}"
  CACHE STRING
  "The list of TPLs to build (order is not important)" )
STRING(REPLACE "," ";" TPL_LIST "${TPL_LIST}")
TRIBITS_SORT_LIST_ACCORDING_TO_MASTER_LIST("${TPL_LIST_FULL_SORTED}" TPL_LIST)
MESSAGE("Building/installing TPL_LIST='${TPL_LIST}'")
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "\n# TPL_LIST\nSET(TPL_LIST \"${TPL_LIST}\")\n")


# Set TPL defaults
SET( TPL_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/.. )
SET_DEFAULT_TPL( BOOST  SRC_DIR  ${TPL_SRC_DIR}/boost/${BOOST_VERSION}   )
SET_DEFAULT_TPL( HDF5   SRC_DIR  ${TPL_SRC_DIR}/hdf5/${HDF5_VERSION}     )
SET_DEFAULT_TPL( ZLIB   SRC_DIR  ${TPL_SRC_DIR}/zlib/${ZLIB_VERSION}     )
SET_DEFAULT_TPL( LAPACK SRC_DIR  ${TPL_SRC_DIR}/lapack/${LAPACK_VERSION} )
SET_DEFAULT_TPL( PETSC  SRC_DIR  ${TPL_SRC_DIR}/petsc/${PETSC_VERSION}   )
SET_DEFAULT_TPL( SILO   SRC_DIR  ${TPL_SRC_DIR}/silo/${SILO_VERSION}     )
SET_DEFAULT_TPL( NETCDF SRC_DIR  ${TPL_SRC_DIR}/netcdf/${NETCDF_VERSION}   )

MESSAGE("")
MESSAGE("Marking TPLs specified through 'TPL_LIST' to build from source!")
# Mark the TPLs to be built from source
FOREACH( TPL ${TPL_LIST} )
  MESSAGE("-- " "Building the TPL '${TPL}' from source!")
  SET(CMAKE_BUILD_${TPL}  TRUE)
ENDFOREACH()

MESSAGE("")
MESSAGE("Process all TPL *.cmake files")
FOREACH( TPL ${TPL_LIST_FULL_SORTED} )
  FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "\n")
  MESSAGE("")
  MESSAGE_TPL("Configuring ${TPL}")
  INCLUDE(TPLs/${TPL}.cmake)
ENDFOREACH()
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "\n")
MESSAGE("")


# Add distclean
ADD_DISTCLEAN()


