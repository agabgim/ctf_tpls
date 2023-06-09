# This will configure and build lapack
# User can configure the source path by speficfying LAPACK_SRC_DIR
#    the download path by specifying LAPACK_URL, or the installed 
#    location by specifying LAPACK_INSTALL_DIR


# Intialize download/src/install vars
SET( LAPACK_BUILD_DIR "${CMAKE_BINARY_DIR}/LAPACK-prefix/src/LAPACK-build" )
IF ( LAPACK_URL ) 
    MESSAGE_TPL("   LAPACK_URL = ${LAPACK_URL}")
    SET( LAPACK_CMAKE_URL            "${LAPACK_URL}"       )
    SET( LAPACK_CMAKE_DOWNLOAD_DIR   "${LAPACK_BUILD_DIR}" )
    SET( LAPACK_CMAKE_SOURCE_DIR     "${LAPACK_BUILD_DIR}" )
    SET( LAPACK_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lapack-${LAPACK_VERSION}" )
    SET( CMAKE_BUILD_LAPACK TRUE )
ELSEIF ( LAPACK_SRC_DIR )
    MESSAGE_TPL("   LAPACK_SRC_DIR = ${LAPACK_SRC_DIR}")
    SET( LAPACK_CMAKE_URL                                  )
    SET( LAPACK_CMAKE_DOWNLOAD_DIR                         )
    SET( LAPACK_CMAKE_SOURCE_DIR     "${LAPACK_SRC_DIR}"   )
    SET( LAPACK_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lapack-${LAPACK_VERSION}" )
    IF (NOT EXISTS "${LAPACK_CMAKE_INSTALL_DIR}")
      MESSAGE_TPL("   LAPACK_CMAKE_INSTALL_DIR does not exist, build LAPACK!")
      SET(CMAKE_BUILD_LAPACK  TRUE)
    ENDIF()
ELSEIF ( LAPACK_INSTALL_DIR ) 
    SET( LAPACK_CMAKE_INSTALL_DIR "${LAPACK_INSTALL_DIR}" )
    SET( CMAKE_BUILD_LAPACK FALSE )
ELSE()
    MESSAGE(FATAL_ERROR "Please specify LAPACK_SRC_DIR, LAPACK_URL, or LAPACK_INSTALL_DIR")
ENDIF()
IF (BUILD_SERIAL_TPLS)
FILE( MAKE_DIRECTORY "${LAPACK_CMAKE_INSTALL_DIR}_serial" )
SET( LAPACK_INSTALL_DIR "${LAPACK_CMAKE_INSTALL_DIR}_serial" )
MESSAGE_TPL( "   LAPACK_INSTALL_DIR = ${LAPACK_INSTALL_DIR}_serial" )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(LAPACK_INSTALL_DIR \"${LAPACK_INSTALL_DIR}\")\n" )


# Installed library names
SET( BLAS_DIR   "${LAPACK_INSTALL_DIR}" )
SET( LAPACK_DIR "${LAPACK_INSTALL_DIR}" )
SET( BLAS_LIB_DIR   "${BLAS_DIR}/lib" )
SET( LAPACK_LIB_DIR "${LAPACK_DIR}/lib" )
IF (ENABLE_SHARED)
  SET(LIB_POSTFIX "so")
ELSE()
  SET(LIB_POSTFIX "a")
ENDIF()
SET(BLAS_LIBS   "${LAPACK_INSTALL_DIR}/lib/libblas.${LIB_POSTFIX}" )
SET(LAPACK_LIBS "${LAPACK_INSTALL_DIR}/lib/liblapack.${LIB_POSTFIX}" )
SET(BLAS_LIB_NAMES   "blas" )
SET(LAPACK_LIB_NAMES "lapack" )


# Build lapack
MESSAGE_TPL("   CMAKE_BUILD_LAPACK = ${CMAKE_BUILD_LAPACK}")
IF ( CMAKE_BUILD_LAPACK ) 
    EXTERNALPROJECT_ADD(
        LAPACK
        URL                 "${LAPACK_CMAKE_URL}"
        DOWNLOAD_COMMAND    URL
        DOWNLOAD_DIR        "${LAPACK_CMAKE_DOWNLOAD_DIR}"
        SOURCE_DIR          "${LAPACK_SRC_DIR}"
        UPDATE_COMMAND      ""
        BUILD_IN_SOURCE     0
        INSTALL_DIR         "${LAPACK_INSTALL_DIR}"
        CMAKE_ARGS          "${CMAKE_ARGS};-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/lapack-${LAPACK_VERSION}_serial"
        BUILD_COMMAND       make -j ${PROCS_INSTALL} VERBOSE=1
        INSTALL_COMMAND     make install -j ${PROCS_INSTALL} VERBOSE=1
        LOG_DOWNLOAD 1   LOG_UPDATE 1   LOG_CONFIGURE 1   LOG_BUILD 1   LOG_TEST 1   LOG_INSTALL 1
    )
    ADD_TPL_SAVE_LOGS( LAPACK )
    ADD_TPL_CLEAN( LAPACK )
ENDIF()
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(BLAS_DIR    \"${BLAS_DIR}\")\n"    )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(LAPACK_DIR  \"${LAPACK_DIR}\")\n"  )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(BLAS_LIBS   \"${BLAS_LIBS}\")\n"   )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(LAPACK_LIBS \"${LAPACK_LIBS}\")\n" )
ELSE()

FILE( MAKE_DIRECTORY "${LAPACK_CMAKE_INSTALL_DIR}" )
SET( LAPACK_INSTALL_DIR "${LAPACK_CMAKE_INSTALL_DIR}" )
MESSAGE_TPL( "   LAPACK_INSTALL_DIR = ${LAPACK_INSTALL_DIR}" )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(LAPACK_INSTALL_DIR \"${LAPACK_INSTALL_DIR}\")\n" )


# Installed library names
SET( BLAS_DIR   "${LAPACK_INSTALL_DIR}" )
SET( LAPACK_DIR "${LAPACK_INSTALL_DIR}" )
SET( BLAS_LIB_DIR   "${BLAS_DIR}/lib" )
SET( LAPACK_LIB_DIR "${LAPACK_DIR}/lib" )
IF (ENABLE_SHARED)
  SET(LIB_POSTFIX "so")
ELSE()
  SET(LIB_POSTFIX "a")
ENDIF()
SET(BLAS_LIBS   "${LAPACK_INSTALL_DIR}/lib/libblas.${LIB_POSTFIX}" )
SET(LAPACK_LIBS "${LAPACK_INSTALL_DIR}/lib/liblapack.${LIB_POSTFIX}" )
SET(BLAS_LIB_NAMES   "blas" )
SET(LAPACK_LIB_NAMES "lapack" )


# Build lapack
MESSAGE_TPL("   CMAKE_BUILD_LAPACK = ${CMAKE_BUILD_LAPACK}")
IF ( CMAKE_BUILD_LAPACK ) 
    EXTERNALPROJECT_ADD(
        LAPACK
        URL                 "${LAPACK_CMAKE_URL}"
        DOWNLOAD_COMMAND    URL
        DOWNLOAD_DIR        "${LAPACK_CMAKE_DOWNLOAD_DIR}"
        SOURCE_DIR          "${LAPACK_SRC_DIR}"
        UPDATE_COMMAND      ""
        BUILD_IN_SOURCE     0
        INSTALL_DIR         "${LAPACK_INSTALL_DIR}"
        CMAKE_ARGS          "${CMAKE_ARGS};-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/lapack-${LAPACK_VERSION}"
        BUILD_COMMAND       make -j ${PROCS_INSTALL} VERBOSE=1
        INSTALL_COMMAND     make install -j ${PROCS_INSTALL} VERBOSE=1
        LOG_DOWNLOAD 1   LOG_UPDATE 1   LOG_CONFIGURE 1   LOG_BUILD 1   LOG_TEST 1   LOG_INSTALL 1
    )
    ADD_TPL_SAVE_LOGS( LAPACK )
    ADD_TPL_CLEAN( LAPACK )
ENDIF()
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(BLAS_DIR    \"${BLAS_DIR}\")\n"    )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(LAPACK_DIR  \"${LAPACK_DIR}\")\n"  )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(BLAS_LIBS   \"${BLAS_LIBS}\")\n"   )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(LAPACK_LIBS \"${LAPACK_LIBS}\")\n" )
ENDIF()
