# This will configure and build petsc
# User can configure the source path by speficfying SLEPC_SRC_DIR,
#    the download path by specifying SLEPC_URL, or the installed 
#    location by specifying SLEPC_INSTALL_DIR


# Intialize download/src/install vars
SET( SLEPC_BUILD_DIR "${CMAKE_BINARY_DIR}/SLEPC-prefix/src/SLEPC-build" )
IF ( SLEPC_URL ) 
    MESSAGE_TPL("   SLEPC_URL = ${SLEPC_URL}")
    SET( SLEPC_CMAKE_URL            "${SLEPC_URL}"       )
    SET( SLEPC_CMAKE_DOWNLOAD_DIR   "${SLEPC_BUILD_DIR}" )
    SET( SLEPC_CMAKE_SOURCE_DIR     "${SLEPC_BUILD_DIR}" )
    SET( SLEPC_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/slepc-${SLEPC_VERSION}" )
    SET( CMAKE_BUILD_SLEPC TRUE )
ELSEIF ( SLEPC_SRC_DIR )
    VERIFY_PATH("${SLEPC_SRC_DIR}")
    MESSAGE_TPL("   SLEPC_SRC_DIR = ${SLEPC_SRC_DIR}")
    SET( SLEPC_CMAKE_URL            "${SLEPC_SRC_DIR}"   )
    SET( SLEPC_CMAKE_DOWNLOAD_DIR   "${SLEPC_BUILD_DIR}" )
    SET( SLEPC_CMAKE_SOURCE_DIR     "${SLEPC_BUILD_DIR}" )
    SET( SLEPC_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/slepc-${SLEPC_VERSION}" )
    #SET( CMAKE_BUILD_SLEPC TRUE )
ELSEIF ( SLEPC_INSTALL_DIR ) 
    SET( SLEPC_CMAKE_INSTALL_DIR "${SLEPC_INSTALL_DIR}" )
    SET( CMAKE_BUILD_SLEPC FALSE )
ELSE()
    MESSAGE(FATAL_ERROR "Please specify SLEPC_SRC_DIR, SLEPC_URL, or SLEPC_INSTALL_DIR")
ENDIF()
SET( SLEPC_INSTALL_DIR "${SLEPC_CMAKE_INSTALL_DIR}" )
MESSAGE_TPL( "   SLEPC_INSTALL_DIR = ${SLEPC_INSTALL_DIR}" )
FILE( APPEND "${CMAKE_INSTALL_PREFIX}/TPLs.cmake" "SET(SLEPC_INSTALL_DIR \"${SLEPC_INSTALL_DIR}\")\n" )

IF (CMAKE_BUILD_SLEPC)

    SET(DEPENDS_ARGS)
    IF (CMAKE_BUILD_LAPACK)
      LIST(APPEND DEPENDS_ARGS  LAPACK)
    ENDIF()
    IF (CMAKE_BUILD_PETSC)
      LIST(APPEND DEPENDS_ARGS  PETSC)
    ENDIF()
    IF (DEPENDS_ARGS)
      SET(DEPENDS_ARGS DEPENDS ${DEPENDS_ARGS})
    ENDIF()

    EXTERNALPROJECT_ADD( 
        SLEPC
        URL                 "${SLEPC_CMAKE_URL}"
        DOWNLOAD_DIR        "${SLEPC_CMAKE_DOWNLOAD_DIR}"
        SOURCE_DIR          "${SLEPC_CMAKE_SOURCE_DIR}"
        UPDATE_COMMAND      ""
        CONFIGURE_COMMAND   env PETSC_DIR=${CMAKE_INSTALL_PREFIX}/petsc-${PETSC_VERSION} PETSC_ARCH= ${SLEPC_BUILD_DIR}/configure --prefix=${CMAKE_INSTALL_PREFIX}/slepc-${SLEPC_VERSION}
        BUILD_COMMAND       make SLEPC_DIR=${SLEPC_BUILD_DIR} PETSC_DIR=${CMAKE_INSTALL_PREFIX}/petsc-${PETSC_VERSION}
        BUILD_IN_SOURCE     1
        INSTALL_COMMAND     make SLEPC_DIR=${SLEPC_BUILD_DIR} PETSC_DIR=${CMAKE_INSTALL_PREFIX}/petsc-${PETSC_VERSION} install
        ${DEPENDS_ARGS}
        LOG_DOWNLOAD 1   LOG_UPDATE 1   LOG_CONFIGURE 1   LOG_BUILD 1   LOG_TEST 1   LOG_INSTALL 1
      )

    ADD_TPL_SAVE_LOGS( SLEPC )
    ADD_TPL_CLEAN( SLEPC )

ENDIF()


