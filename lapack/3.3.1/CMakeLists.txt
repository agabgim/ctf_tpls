cmake_minimum_required(VERSION 2.6)
project(LAPACK)
ENABLE_LANGUAGE(Fortran)

# Configure the warning and code coverage suppression file
configure_file( 
  "${LAPACK_SOURCE_DIR}/CTestCustom.cmake.in"
  "${LAPACK_BINARY_DIR}/CTestCustom.cmake"
  COPYONLY
)

# Add the CMake directory for custon CMake modules
set(CMAKE_MODULE_PATH "${LAPACK_SOURCE_DIR}/CMAKE" ${CMAKE_MODULE_PATH})

if (UNIX)
   if ( "${CMAKE_Fortran_COMPILER}" MATCHES "ifort" )
  set( CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -fltconsistency -fp_port" )
   endif ()
endif ()

macro(lapack_install_library lib)
  install(TARGETS ${lib} EXPORT lapack-targets
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    RUNTIME DESTINATION bin
  )
endmacro()

# --------------------------------------------------
# Testing

enable_testing()
include(CTest)
enable_testing()
# --------------------------------------------------

# Organize output files.  On Windows this also keeps .dll files next
# to the .exe files that need them, making tests easy to run.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LAPACK_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LAPACK_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LAPACK_BINARY_DIR}/lib)

# --------------------------------------------------
# Check for any necessary platform specific compiler flags
include( CheckLAPACKCompilerFlags )
CheckLAPACKCompilerFlags()

# --------------------------------------------------
# Check second function

include(CheckTimeFunction)
set(TIME_FUNC NONE ${TIME_FUNC})
CHECK_TIME_FUNCTION(NONE TIME_FUNC)
CHECK_TIME_FUNCTION(INT_CPU_TIME TIME_FUNC)
CHECK_TIME_FUNCTION(EXT_ETIME TIME_FUNC)
CHECK_TIME_FUNCTION(EXT_ETIME_ TIME_FUNC)
CHECK_TIME_FUNCTION(INT_ETIME TIME_FUNC)
message(STATUS "--> Will use second_${TIME_FUNC}.f and dsecnd_${TIME_FUNC}.f as timing function.")

set(SECOND_SRC  ${LAPACK_SOURCE_DIR}/INSTALL/second_${TIME_FUNC}.f)
set(DSECOND_SRC  ${LAPACK_SOURCE_DIR}/INSTALL/dsecnd_${TIME_FUNC}.f)
set(prefix ${CMAKE_INSTALL_PREFIX})
set(libdir ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
set(PKG_CONFIG_DIR ${libdir}/pkgconfig)

# --------------------------------------------------
# Subdirectories that need to be processed

option(USE_OPTIMIZED_BLAS "Whether or not to use an optimized BLAS library instead of included netlib BLAS" OFF)

# Check the usage of the user provided BLAS libraries
if(BLAS_LIBRAIRIES)
  include(CheckFortranFunctionExists)
  set(CMAKE_REQUIRED_LIBRARIES ${BLAS_LIBRAIRIES})
  CHECK_FORTRAN_FUNCTION_EXISTS("dgemm" BLAS_FOUND)
  unset( CMAKE_REQUIRED_LIBRARIES )
  if(BLAS_FOUND)
    message(STATUS "--> BLAS supplied by user is WORKING, will use ${BLAS_LIBRAIRIES}.")
  else(BLAS_FOUND)
    message(ERROR "--> BLAS supplied by user is not WORKING, CANNOT USE ${BLAS_LIBRAIRIES}.")
    message(ERROR "-->     Will use REFERENCE BLAS (by default)")
    message(ERROR "-->     Or Correct your BLAS_LIBRAIRIES entry ")
    message(ERROR "-->     Or Consider checking USE_OPTIMIZED_BLAS")
  endif(BLAS_FOUND)

# User did not provide a BLAS Library but specified to search for one
elseif( USE_OPTIMIZED_BLAS )
  find_package( BLAS )
endif (BLAS_LIBRAIRIES)

# Neither user specified or optimized BLAS libraries can be used
if(NOT BLAS_FOUND)
  message(STATUS "Using supplied NETLIB BLAS implementation")
  add_subdirectory(BLAS)
  set( BLAS_LIBRARIES blas )
else()
  set( CMAKE_EXE_LINKER_FLAGS 
    "${CMAKE_EXE_LINKER_FLAGS} ${BLAS_LINKER_FLAGS}" 
    CACHE STRING "Linker flags for executables" FORCE)
  set( CMAKE_MODULE_LINKER_FLAGS 
    "${CMAKE_MODULE_LINKER_FLAGS} ${BLAS_LINKER_FLAGS}" 
    CACHE STRING "Linker flags for modules" FORCE)
  set( CMAKE_SHARED_LINKER_FLAGS 
    "${CMAKE_SHARED_LINKER_FLAGS} ${BLAS_LINKER_FLAGS}" 
    CACHE STRING "Linker flags for shared libs" FORCE)
endif( NOT BLAS_FOUND )
   
add_subdirectory(SRC)
if(BUILD_TESTING)
  add_subdirectory(TESTING)
endif(BUILD_TESTING)

# --------------------------------------------------
# CPACK Packaging 

SET(CPACK_PACKAGE_NAME "LAPACK")
SET(CPACK_PACKAGE_VENDOR "University of Tennessee, Univ. of California Berkeley, Univ. of Colorado Denver and NAG Ltd")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LAPACK- Linear Algebra Package")
set(LAPACK_VERSION 3.3.1)
set(CPACK_PACKAGE_VERSION_MAJOR 3)
set(CPACK_PACKAGE_VERSION_MINOR 3)
set(CPACK_PACKAGE_VERSION_PATCH 1)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "LAPACK")
IF(WIN32 AND NOT UNIX)
  # There is a bug in NSI that does not handle full unix paths properly. Make
  # sure there is at least one set of four (4) backlasshes.
  SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\http://icl.cs.utk.edu/lapack-forum")
  SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.netlib.org/lapack")
  SET(CPACK_NSIS_CONTACT "lapack@eecs.utk.edu")
  SET(CPACK_NSIS_MODIFY_PATH ON)
  SET(CPACK_NSIS_DISPLAY_NAME "LAPACK-${LAPACK_VERSION}")
  set(CPACK_PACKAGE_RELOCATABLE "true")
ELSE(WIN32 AND NOT UNIX)
  SET(CPACK_GENERATOR "TGZ")
  SET(CPACK_SOURCE_GENERATOR TGZ)
  SET(CPACK_SOURCE_PACKAGE_FILE_NAME "lapack-${LAPACK_VERSION}" )
  SET(CPACK_SOURCE_IGNORE_FILES ~$ .svn ${CPACK_SOURCE_IGNORE_FILES} )
ENDIF(WIN32 AND NOT UNIX)
INCLUDE(CPack)


# --------------------------------------------------
# By default static library
OPTION(BUILD_SHARED_LIBS "Build shared libraries" OFF )
OPTION(BUILD_STATIC_LIBS "Build static libraries" ON )
#OPTION(BUILD_SHARED_LIBS "Build shared libraries" ON )

if( NOT BLAS_FOUND )
  export(TARGETS blas lapack FILE lapack-targets.cmake)
else( NOT BLAS_FOUND )
  export(TARGETS lapack FILE lapack-targets.cmake)
endif( NOT BLAS_FOUND )
configure_file(${LAPACK_SOURCE_DIR}/CMAKE/lapack-config-version.cmake.in
  ${LAPACK_BINARY_DIR}/lapack-config-version.cmake @ONLY)
configure_file(${LAPACK_SOURCE_DIR}/CMAKE/lapack-config-build.cmake.in
  ${LAPACK_BINARY_DIR}/lapack-config.cmake @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/lapack.pc.in ${CMAKE_CURRENT_BINARY_DIR}/lapack.pc)
 install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/lapack.pc
  DESTINATION ${PKG_CONFIG_DIR}
   )

configure_file(${LAPACK_SOURCE_DIR}/CMAKE/lapack-config-install.cmake.in
  ${LAPACK_BINARY_DIR}/CMakeFiles/lapack-config.cmake @ONLY)
install(FILES
  ${LAPACK_BINARY_DIR}/CMakeFiles/lapack-config.cmake
  ${LAPACK_BINARY_DIR}/lapack-config-version.cmake
  DESTINATION lib/cmake/lapack-${LAPACK_VERSION}
  )
install(EXPORT lapack-targets
  DESTINATION lib/cmake/lapack-${LAPACK_VERSION})
