cmake_minimum_required(VERSION 3.16)
project(URTS VERSION 0.1.0 LANGUAGES CXX)
enable_testing()

include(CheckCXXCompilerFlag)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

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
find_package(Boost COMPONENTS program_options REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(SOCI REQUIRED)
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
# Optional
#set(FindMiniSEED_DIR  ${CMAKE_MODULE_PATH}) #${CMAKE_SOURCE_DIR}/cmake)
#set(FindEarthworm_DIR ${CMAKE_MODULE_PATH}) #${CMAKE_SOURCE_DIR}/cmake)
#set(FindSEEDLink_DIR  ${CMAKE_MODULE_PATH}) #${CMAKE_SOURCE_DIR}/cmake)
find_package(Earthworm)
find_package(SEEDLink)
find_package(UUSSMLModels)
if (${Earthworm_FOUND})
   find_package(MiniSEED)
endif()
if (${UUSSMLModels_FOUND})
   message("Enabling UUSS_MLMODELS")
   set(UUSS_MLMODELS TRUE)
endif() 

set(BUILD_EW FALSE)
set(BUILD_MSEED FALSE)
if (${Earthworm_FOUND})
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
if (${FindSEEDLink_FOUND})
   message("Will compile SEEDLink")
   set(BUILD_SEEDLINK TRUE)
endif()
add_compile_definitions(URTS_SRC)

set(VERSION_SRC
    src/version.cpp)
set(GENERIC_SRC
    src/broadcasts/internal/dataPacket/dataPacket.cpp
    src/broadcasts/internal/dataPacket/publisher.cpp
    src/broadcasts/internal/dataPacket/publisherOptions.cpp
    src/broadcasts/internal/dataPacket/subscriber.cpp
    src/broadcasts/internal/dataPacket/subscriberOptions.cpp
    src/broadcasts/internal/pick/pick.cpp
    src/broadcasts/internal/pick/publisher.cpp
    src/broadcasts/internal/pick/publisherOptions.cpp 
    src/broadcasts/internal/pick/subscriber.cpp
    src/broadcasts/internal/pick/subscriberOptions.cpp
    src/broadcasts/internal/pick/uncertaintyBound.cpp
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
    src/services/scalable/packetCache/bulkDataResponse.cpp
    src/services/scalable/packetCache/cappedCollection.cpp
    src/services/scalable/packetCache/circularBuffer.cpp
    src/services/scalable/packetCache/dataRequest.cpp
    src/services/scalable/packetCache/dataResponse.cpp
    src/services/scalable/packetCache/sensorRequest.cpp
    src/services/scalable/packetCache/sensorResponse.cpp
    src/services/scalable/packetCache/service.cpp
    src/services/scalable/packetCache/serviceOptions.cpp
    src/services/scalable/packetCache/singleComponentWaveform.cpp
    src/services/scalable/packetCache/threeComponentWaveform.cpp
    src/services/scalable/packetCache/requestorOptions.cpp
    src/services/scalable/packetCache/wigginsInterpolator.cpp
    )
if (${UUSS_MLMODELS})
    set(UUSS_SRC
        src/services/scalable/uNetDetector/threeComponent/inferenceRequest.cpp
        src/services/scalable/uNetDetector/threeComponent/inferenceResponse.cpp
        src/services/scalable/uNetDetector/threeComponent/preprocessingRequest.cpp
        src/services/scalable/uNetDetector/threeComponent/preprocessingResponse.cpp
        src/services/scalable/uNetDetector/threeComponent/service.cpp)
endif()
if (${BUILD_EW})
   set(EARTHWORM_SRC
       src/broadcasts/external/earthworm/traceBuf2.cpp
       src/broadcasts/external/earthworm/waveRing.cpp)
endif()
if (${BUILD_SEEDLINK})
   set(SEEDLINK_SRC
       src/broadcasts/external/seedlink/client.cpp
       src/broadcasts/external/seedlink/clientOptions.cpp)
endif()

add_library(urts ${VERSION_SRC} ${GENERIC_SRC} ${UUSS_SRC} ${EARTHWORM_SRC} ${SEEDLINK_SRC})
set_target_properties(urts PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES 
                      CXX_EXTENSIONS NO)
target_include_directories(urts
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                           PUBLIC ${UMPS_INCLUDE_DIR}
                           PUBLIC $<INSTALL_INTERFACE:${CMAKE_SOURCE_DIR}/include/urts>)
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
if (${BUILD_SEEDLINK})
   target_include_directories(urts PRIVATE ${SEEDLINK_INCLUDE_DIR})
   target_link_libraries(urts PRIVATE ${SEEDLINK_LIBRARY})
endif()
if (${UUSS_MLMODELS})
   target_include_directories(urts PRIVATE ${UUSSMLModels_INCLUDE_DIR})
   target_link_libraries(urts PRIVATE ${UUSSMLModels_LIBRARY})
endif()

##########################################################################################
#                                         Unit Tests                                     #
##########################################################################################
file(COPY ${CMAKE_SOURCE_DIR}/testing/data DESTINATION .)
set(TEST_SRC
    testing/main.cpp
    testing/broadcasts/internal/dataPacket.cpp
    testing/broadcasts/internal/pick.cpp
    testing/services/standalone/incrementer.cpp
    testing/services/scalable/packetCache.cpp
    )
if (${UUSS_MLMODELS})
   set(TEST_SRC ${TEST_SRC}
       testing/services/scalable/uNetDetector/threeComponent.cpp)
endif()
if (${BUILD_EW})
   set(TEST_SRC ${TEST_SRC} testing/broadcasts/external/earthworm.cpp)
endif()
if (${BUILD_SEEDLINK})
   set(TEST_SRC ${TEST_SRC} testing/broadcasts/external/seedlink.cpp)
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
add_executable(incrementer src/services/standalone/incrementer/module.cpp)
set_target_properties(incrementer PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
target_include_directories(incrementer PRIVATE
                           $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                           Boost::program_options)
target_link_libraries(incrementer
                      PRIVATE urts ${UMPS_LIBRARY} Boost::program_options Threads::Threads)

add_executable(packetCache src/services/scalable/packetCache/module.cpp)
set_target_properties(packetCache PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
target_include_directories(packetCache PRIVATE
                           $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                           Boost::program_options)
target_link_libraries(packetCache
                      PRIVATE urts ${UMPS_LIBRARY} Boost::program_options Threads::Threads)  

set(URTS_MODULES incrementer packetCache)

if (${BUILD_EW})
   add_executable(broadcastWaveRing src/broadcasts/external/earthworm/module.cpp)
   set_target_properties(broadcastWaveRing PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   target_include_directories(broadcastWaveRing PRIVATE
                              $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              Boost::program_options)
   target_link_libraries(broadcastWaveRing
                         PRIVATE urts ${UMPS_LIBRARY}
                                 Boost::program_options Threads::Threads)
   set(URTS_MODULES ${MODULES} broadcastWaveRing)
endif()
if (${BUILD_SEEDLINK})
   add_executable(broadcastSEEDLink src/broadcasts/external/seedlink/module.cpp)
   set_target_properties(broadcastSEEDLink PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES 
                         CXX_EXTENSIONS NO) 
   target_include_directories(broadcastSEEDLink PRIVATE
                              $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              Boost::program_options)
   target_link_libraries(broadcastSEEDLink
                         PRIVATE urts ${UMPS_LIBRARY}
                                 Boost::program_options Threads::Threads)
   set(URTS_MODULES ${MODULES} broadcastSEEDLink)
endif()

##########################################################################################
#                                      Python Wrapping                                   #
##########################################################################################
if (WRAP_PYTHON)
   message("Will build Python bindings")
   find_package(pybind11 REQUIRED)
   add_library(urtspy MODULE
               python/urtspy.cpp
               python/broadcasts.cpp
               python/broadcasts/dataPacket.cpp
               python/services.cpp
               python/services/scalable/packetCache.cpp)
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
   install(TARGETS urts urtspy ${URTS_MODULES}
           RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/urts
           LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
           ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
           PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
else()
   install(TARGETS urts ${URTS_MODULES}
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
