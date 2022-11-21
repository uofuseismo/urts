cmake_minimum_required(VERSION 3.16)
project(URTS VERSION 0.1.0 LANGUAGES CXX)
enable_testing()

include(CheckCXXCompilerFlag)

set(BUILD_SHARED_LIBS YES)
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

set(THREADS_PREFER_PTHREAD_FLAG ON)
include(CheckCXXCompilerFlag)
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(SOCI REQUIRED)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
# Optional
set(FindMiniSEED_DIR ${CMAKE_SOURCE_DIR}/cmake)
set(FindEarthworm_DIR ${CMAKE_SOURCE_DIR}/cmake)
find_package(FindEarthworm)
if (${FindEarthworm_FOUND})
   find_package(FindMiniSEED)
endif()

set(BUILD_EW FALSE)
set(BUILD_MSEED FALSE)
if (${FindEarthworm_FOUND})
   message("Will compile Earthworm")
   set(BUILD_EW TRUE)
   if (${EARTHWORM_DEFAULT_TRACEBUF2_SIZE})
      set(EARTHWORM_TRACEBUF2_SIZE 4096)
   endif()
   message("Using TraceBuf2 message size of: " ${EARTHWORM_TRACEBUF2_SIZE})
   if (${FindMiniSEED_FOUND})
      message("Will compile earthworm modules with MiniSEED")
      set(BUILD_MSEED TRUE)
      add_compile_definitions(WITH_MSEED)
   endif()
endif()

set(VERSION_SRC
    src/version.cpp)
set(GENERIC_SRC
    src/proxyBroadcasts/dataPacket/dataPacket.cpp
    src/proxyBroadcasts/dataPacket/publisher.cpp
    src/proxyBroadcasts/dataPacket/publisherOptions.cpp
    src/proxyBroadcasts/dataPacket/subscriber.cpp
    src/proxyBroadcasts/dataPacket/subscriberOptions.cpp
    src/services/standalone/incrementer/counter.cpp
    src/services/standalone/incrementer/incrementRequest.cpp
    src/services/standalone/incrementer/incrementResponse.cpp
    src/services/standalone/incrementer/itemsRequest.cpp
    src/services/standalone/incrementer/itemsResponse.cpp
    src/services/standalone/incrementer/service.cpp
    src/services/standalone/incrementer/serviceOptions.cpp
    src/services/standalone/incrementer/requestor.cpp
    src/services/standalone/incrementer/requestorOptions.cpp
    src/services/scalable/packetCache/bulkDataRequest.cpp
    src/services/scalable/packetCache/dataRequest.cpp
    src/services/scalable/packetCache/dataResponse.cpp
    src/services/scalable/packetCache/sensorRequest.cpp
    src/services/scalable/packetCache/sensorResponse.cpp
    )
if (${BUILD_EW})
   set(EARTHWORM_SRC
       src/earthworm/traceBuf2.cpp
       src/earthworm/waveRing.cpp)
endif()

add_library(urts ${VERSION_SRC} ${GENERIC_SRC} ${EARTHWORM_SRC})
set_target_properties(urts PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES 
                      CXX_EXTENSIONS NO)
target_include_directories(urts
                           PUBLIC ${UMPS_INCLUDE_DIR}
                           PUBLIC $<INSTALL_INTERFACE:${CMAKE_SOURCE_DIR}/include/urts>
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
target_link_libraries(urts
                      PRIVATE ${UMPS_LIBRARY} Threads::Threads)
if (${BUILD_EW})
    target_include_directories(urts PRIVATE ${EARTHWORM_INCLUDE_DIR})
    target_link_libraries(urts PRIVATE ${EARTHWORM_UTILITY_LIBRARY} ${EARTHWORM_MT_LIBRARY})
    target_compile_definitions(urts PRIVATE WITH_EARTHWORM)
    if (${BUILD_MSEED})
       target_include_directories(urts PRIVATE ${MINISEED_INCLUDE_DIR})
       target_link_libraries(urts PRIVATE ${MINISEED_LIBRARY})
    endif()
endif()

##########################################################################################
#                                         Unit Tests                                     #
##########################################################################################
#file(COPY ${CMAKE_SOURCE_DIR}/testing/data DESTINATION .)
set(TEST_SRC
    testing/main.cpp
    testing/proxyBroadcasts/dataPacket.cpp
    testing/services/standalone/incrementer.cpp
    testing/services/scalable/packetCache.cpp
    )
if (${BUILD_EW})
   set(TEST_SRC ${TEST_SRC} testing/earthworm/earthworm.cpp)
endif()
add_executable(unitTests ${TEST_SRC})
set_target_properties(unitTests PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
target_link_libraries(unitTests PRIVATE urts ${UMPS_LIBRARY} ${GTEST_BOTH_LIBRARIES})
target_include_directories(unitTests
                           PRIVATE ${UMPS_INCLUDE_DIR} ${GTEST_INCLUDE_DIRS}
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

# Add the tests
add_test(NAME unitTests
         COMMAND unitTests)

##########################################################################################
#                                            Modules                                     #
##########################################################################################
if (${BUILD_EW})
   add_executable(broadcastWaveRing src/modules/proxyBroadcasts/broadcastWaveRing.cpp)
   set_target_properties(broadcastWaveRing PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   target_include_directories(broadcastWaveRing
                              PRIVATE ${CMAKE_SOURCE_DIR}/include
                                      ${UMPS_INCLUDE_DIR}
                                      Boost::program_options)
   target_link_libraries(broadcastWaveRing
                         PRIVATE urts ${UMPS_LIBRARY}
                                 Boost::program_options Threads::Threads)
endif()

##########################################################################################
#                                      Python Wrapping                                   #
##########################################################################################
if (WRAP_PYTHON)
   message("Will build Python bindings")
   find_package(pybind11 REQUIRED)
   add_library(urtspy MODULE
               python/urtspy.cpp
               python/proxyBroadcasts.cpp
               python/proxyBroadcasts/dataPacket.cpp)
   set_target_properties(urtspy PROPERTIES
                         PREFIX ""
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   target_include_directories(urtspy
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/python/include>
                                      $<BUILD_INTERFACE:${UMPS_INCLUDE_DIR}>
                                      $<BUILD_INTERFACE:${UMPS_INCLUDE_DIR}/../>
                                      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
   target_link_libraries(urtspy
                         PRIVATE pybind11::module pybind11::pybind11 pybind11::lto
                                 urts ${UMPS_PYTHON_LIBRARY} ${UMPS_LIBRARY})

endif()


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
   install(TARGETS urts urtspy
           RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/urts
           LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
           ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
           PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
else()
   install(TARGETS urts
           RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/urts
           LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
           ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
           PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
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
