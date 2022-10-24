cmake_minimum_required(VERSION 3.16)
project(URTS VERSION 0.1.0 LANGUAGES CXX)
enable_testing()

include(CheckCXXCompilerFlag)

set(ORGANIZATION "UUSS" CACHE STRING "Name of your organization.")
set(HOME_PATH $ENV{HOME})
set(DATA_PATH   ${HOME_PATH}/.local/share/${ORGANIZATION}/urts CACHE STRING "Default data path.")
set(CACHE_PATH  ${HOME_PATH}/.cache/${ORGANIZATION}/urts CACHE STRING "Default cache path.")
set(CONFIG_PATH ${HOME_PATH}/.config/${ORGANIZATION} CACHE STRING "Default config path.")

configure_file(${CMAKE_SOURCE_DIR}/include/private/version.hpp.in
               ${CMAKE_SOURCE_DIR}/include/urts/version.hpp)
configure_file(${CMAKE_SOURCE_DIR}/include/private/paths.hpp.in
               ${CMAKE_SOURCE_DIR}/include/private/paths.hpp)
configure_file(${CMAKE_SOURCE_DIR}/include/private/organization.hpp.in
               ${CMAKE_SOURCE_DIR}/include/private/organization.hpp)

##########################################################################################
#                                         Installation                                   #
##########################################################################################
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${version}"
    COMPATIBILITY AnyNewerVersion
)
if (WRAP_PYTHON)
   install(TARGETS urts pyurts
           RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
           LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
           ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
           PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
else()
   #install(TARGETS urts
   #        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
   #        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   #        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   #        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/urts
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES
        #"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
        COMPONENT ${PROJECT_NAME})
##########################################################################################
#                                     CPACK Packaging                                    #
##########################################################################################
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "UUSS")
set(CPACK_PACKAGE_CONTACT "ben.baker@utah.edu")
set(CPACK_PACKAGE_LICENSE "MIT")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "University of Utah Seismograph Stations Real-Time Seismology Utilities")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_COMPONENTS_ALL libraries headers)
if (WIN32)
    set(CPACK_GENERATOR ZIP WIX)
elseif (APPLE)
    set(CPACK_GENERATOR TGZ productbuild)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_GENERATOR TGZ RPM)
else()
    set(CPACK_GENERATOR TGZ)
endif()
set(CPACK_SOURCE_IGNORE_FILES
  /\\.git/
  \\.swp
  \\.orig
  /CMakeLists\\.txt\\.user
  /private/
)
include(CPack) # Put this last!
