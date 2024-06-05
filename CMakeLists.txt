cmake_minimum_required(VERSION 3.16)
project(URTS VERSION 0.1.0 LANGUAGES CXX)
enable_testing()

# Some options
option(BUILD_UI "Build the Qt-based user interface" OFF)
option(WRAP_PYTHON "Compile the Python bindings" OFF)

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
find_package(Boost COMPONENTS program_options date_time REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(SOCI REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(Threads REQUIRED)
find_package(Time REQUIRED)
find_package(GTest REQUIRED)
find_package(Catch2 3)
# Optional
#set(FindMiniSEED_DIR  ${CMAKE_MODULE_PATH}) #${CMAKE_SOURCE_DIR}/cmake)
#set(FindEarthworm_DIR ${CMAKE_MODULE_PATH}) #${CMAKE_SOURCE_DIR}/cmake)
#set(FindSEEDLink_DIR  ${CMAKE_MODULE_PATH}) #${CMAKE_SOURCE_DIR}/cmake)
find_package(Earthworm)
find_package(SEEDLink)
find_package(UUSSMLModels)
find_package(uLocator)
find_package(MAssociate)
if (${Earthworm_FOUND} OR ${SEEDLink_FOUND})
   find_package(MiniSEED)
endif()
if (${UUSSMLModels_FOUND})
   message("Enabling UUSS machine-learning models")
   set(BUILD_UUSS_MLMODELS TRUE)
endif() 
if (${BUILD_UI})
   find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
   find_package(QPhase REQUIRED)
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
   if (${MiniSEED_FOUND})
      message("Will compile earthworm modules with MiniSEED")
      set(BUILD_MSEED TRUE)
      add_compile_definitions(WITH_MSEED)
   endif()
endif()
if (${SEEDLink_FOUND})
   if (${MiniSEED_FOUND})
      message("Will compile SEEDLink")
      set(BUILD_MSEED TRUE)
      set(BUILD_SEEDLINK TRUE)
   endif()
endif()
add_compile_definitions(URTS_SRC)

set(VERSION_SRC
    src/version.cpp)
set(GENERIC_MESSAGE_SRC
    src/broadcasts/internal/dataPacket/dataPacket.cpp
    src/broadcasts/internal/probabilityPacket/probabilityPacket.cpp
    src/broadcasts/internal/pick/pick.cpp
    src/services/standalone/incrementer/incrementRequest.cpp
    src/services/standalone/incrementer/incrementResponse.cpp
    src/services/standalone/incrementer/itemsRequest.cpp
    src/services/standalone/incrementer/itemsResponse.cpp
    src/services/scalable/packetCache/bulkDataRequest.cpp
    src/services/scalable/packetCache/bulkDataResponse.cpp
    src/services/scalable/packetCache/dataRequest.cpp
    src/services/scalable/packetCache/dataResponse.cpp
    src/services/scalable/packetCache/sensorRequest.cpp
    src/services/scalable/packetCache/sensorResponse.cpp)
set(GENERIC_SRC
    #src/broadcasts/internal/dataPacket/dataPacket.cpp
    src/broadcasts/internal/dataPacket/publisher.cpp
    src/broadcasts/internal/dataPacket/publisherOptions.cpp
    src/broadcasts/internal/dataPacket/subscriber.cpp
    src/broadcasts/internal/dataPacket/subscriberOptions.cpp
    #src/broadcasts/internal/probabilityPacket/probabilityPacket.cpp
    src/broadcasts/internal/probabilityPacket/publisher.cpp
    src/broadcasts/internal/probabilityPacket/publisherOptions.cpp
    src/broadcasts/internal/probabilityPacket/subscriber.cpp
    src/broadcasts/internal/probabilityPacket/subscriberOptions.cpp
    #src/broadcasts/internal/pick/pick.cpp
    src/broadcasts/internal/pick/publisher.cpp
    src/broadcasts/internal/pick/publisherOptions.cpp 
    src/broadcasts/internal/pick/subscriber.cpp
    src/broadcasts/internal/pick/subscriberOptions.cpp
    src/broadcasts/internal/pick/uncertaintyBound.cpp
    src/broadcasts/utilities/dataPacketSanitizer.cpp
    src/broadcasts/utilities/dataPacketSanitizerOptions.cpp
    src/database/aqms/channelData.cpp
    src/database/aqms/channelDataTable.cpp
    src/database/aqms/channelDataTablePoller.cpp
    src/database/aqms/stationData.cpp
    src/database/aqms/stationDataTable.cpp
    src/database/connection/postgresql.cpp
    src/observerPattern/subject.cpp
    src/services/standalone/incrementer/counter.cpp
    #src/services/standalone/incrementer/incrementRequest.cpp
    #src/services/standalone/incrementer/incrementResponse.cpp
    #src/services/standalone/incrementer/itemsRequest.cpp
    #src/services/standalone/incrementer/itemsResponse.cpp
    src/services/standalone/incrementer/service.cpp
    src/services/standalone/incrementer/serviceOptions.cpp
    src/services/standalone/incrementer/requestor.cpp
    src/services/standalone/incrementer/requestorOptions.cpp
    #src/services/scalable/packetCache/bulkDataRequest.cpp
    #src/services/scalable/packetCache/bulkDataResponse.cpp
    src/services/scalable/packetCache/cappedCollection.cpp
    src/services/scalable/packetCache/circularBuffer.cpp
    #src/services/scalable/packetCache/dataRequest.cpp
    #src/services/scalable/packetCache/dataResponse.cpp
    #src/services/scalable/packetCache/sensorRequest.cpp
    #src/services/scalable/packetCache/sensorResponse.cpp
    src/services/scalable/packetCache/service.cpp
    src/services/scalable/packetCache/serviceOptions.cpp
    src/services/scalable/packetCache/singleComponentWaveform.cpp
    src/services/scalable/packetCache/threeComponentWaveform.cpp
    src/services/scalable/packetCache/requestor.cpp
    src/services/scalable/packetCache/requestorOptions.cpp
    src/services/scalable/packetCache/wigginsInterpolator.cpp
    )
if (${uLocator_FOUND})
   set(GENERIC_MESSAGE_SRC ${GENERIC_MESSAGE_SRC}
       src/services/scalable/travelTimes/stationRequest.cpp
       src/services/scalable/travelTimes/stationResponse.cpp
       src/services/scalable/associators/massociate/associationRequest.cpp
       src/services/scalable/associators/massociate/pick.cpp
      )
   set(GENERIC_SRC ${GENERIC_SRC}
       src/services/scalable/travelTimes/service.cpp
       src/services/scalable/travelTimes/serviceOptions.cpp 
      )
   if (${MAssociate_FOUND})
      set(GENERIC_SRC ${GENERIC_SRC}
          src/services/scalable/associators/massociate/service.cpp
          src/services/scalable/associators/massociate/serviceOptions.cpp
         )
   endif()
endif()
if (${BUILD_UUSS_MLMODELS})
    set(UUSS_MESSAGE_SRC
        src/services/scalable/detectors/uNetThreeComponentP/inferenceRequest.cpp
        src/services/scalable/detectors/uNetThreeComponentP/inferenceResponse.cpp
        src/services/scalable/detectors/uNetThreeComponentP/preprocessingRequest.cpp
        src/services/scalable/detectors/uNetThreeComponentP/preprocessingResponse.cpp
        src/services/scalable/detectors/uNetThreeComponentP/processingRequest.cpp
        src/services/scalable/detectors/uNetThreeComponentP/processingResponse.cpp
        src/services/scalable/detectors/uNetThreeComponentS/inferenceRequest.cpp
        src/services/scalable/detectors/uNetThreeComponentS/inferenceResponse.cpp
        src/services/scalable/detectors/uNetThreeComponentS/preprocessingRequest.cpp
        src/services/scalable/detectors/uNetThreeComponentS/preprocessingResponse.cpp
        src/services/scalable/detectors/uNetThreeComponentS/processingRequest.cpp
        src/services/scalable/detectors/uNetThreeComponentS/processingResponse.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/inferenceRequest.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/inferenceResponse.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/preprocessingRequest.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/preprocessingResponse.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingRequest.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingResponse.cpp
        src/services/scalable/pickers/cnnOneComponentP/inferenceRequest.cpp
        src/services/scalable/pickers/cnnOneComponentP/inferenceResponse.cpp
        src/services/scalable/pickers/cnnOneComponentP/preprocessingRequest.cpp
        src/services/scalable/pickers/cnnOneComponentP/preprocessingResponse.cpp
        src/services/scalable/pickers/cnnOneComponentP/processingRequest.cpp
        src/services/scalable/pickers/cnnOneComponentP/processingResponse.cpp
        src/services/scalable/pickers/cnnThreeComponentS/inferenceRequest.cpp
        src/services/scalable/pickers/cnnThreeComponentS/inferenceResponse.cpp
        src/services/scalable/pickers/cnnThreeComponentS/preprocessingRequest.cpp
        src/services/scalable/pickers/cnnThreeComponentS/preprocessingResponse.cpp
        src/services/scalable/pickers/cnnThreeComponentS/processingRequest.cpp
        src/services/scalable/pickers/cnnThreeComponentS/processingResponse.cpp)
    set(UUSS_SRC
        #src/services/scalable/detectors/uNetThreeComponentP/inferenceRequest.cpp
        #src/services/scalable/detectors/uNetThreeComponentP/inferenceResponse.cpp
        #src/services/scalable/detectors/uNetThreeComponentP/preprocessingRequest.cpp
        #src/services/scalable/detectors/uNetThreeComponentP/preprocessingResponse.cpp
        #src/services/scalable/detectors/uNetThreeComponentP/processingRequest.cpp
        #src/services/scalable/detectors/uNetThreeComponentP/processingResponse.cpp
        src/services/scalable/detectors/uNetThreeComponentP/requestor.cpp
        src/services/scalable/detectors/uNetThreeComponentP/requestorOptions.cpp
        src/services/scalable/detectors/uNetThreeComponentP/service.cpp
        src/services/scalable/detectors/uNetThreeComponentP/serviceOptions.cpp
        #src/services/scalable/detectors/uNetThreeComponentS/inferenceRequest.cpp
        #src/services/scalable/detectors/uNetThreeComponentS/inferenceResponse.cpp
        #src/services/scalable/detectors/uNetThreeComponentS/preprocessingRequest.cpp
        #src/services/scalable/detectors/uNetThreeComponentS/preprocessingResponse.cpp
        #src/services/scalable/detectors/uNetThreeComponentS/processingRequest.cpp
        #src/services/scalable/detectors/uNetThreeComponentS/processingResponse.cpp
        src/services/scalable/detectors/uNetThreeComponentS/requestor.cpp
        src/services/scalable/detectors/uNetThreeComponentS/requestorOptions.cpp
        src/services/scalable/detectors/uNetThreeComponentS/service.cpp
        src/services/scalable/detectors/uNetThreeComponentS/serviceOptions.cpp
        #src/services/scalable/firstMotionClassifiers/cnnOneComponentP/inferenceRequest.cpp
        #src/services/scalable/firstMotionClassifiers/cnnOneComponentP/inferenceResponse.cpp
        #src/services/scalable/firstMotionClassifiers/cnnOneComponentP/preprocessingRequest.cpp
        #src/services/scalable/firstMotionClassifiers/cnnOneComponentP/preprocessingResponse.cpp
        #src/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingRequest.cpp
        #src/services/scalable/firstMotionClassifiers/cnnOneComponentP/processingResponse.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/requestor.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/requestorOptions.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/service.cpp
        src/services/scalable/firstMotionClassifiers/cnnOneComponentP/serviceOptions.cpp
        #src/services/scalable/pickers/cnnOneComponentP/inferenceRequest.cpp
        #src/services/scalable/pickers/cnnOneComponentP/inferenceResponse.cpp
        #src/services/scalable/pickers/cnnOneComponentP/preprocessingRequest.cpp
        #src/services/scalable/pickers/cnnOneComponentP/preprocessingResponse.cpp
        #src/services/scalable/pickers/cnnOneComponentP/processingRequest.cpp
        #src/services/scalable/pickers/cnnOneComponentP/processingResponse.cpp
        src/services/scalable/pickers/cnnOneComponentP/requestor.cpp
        src/services/scalable/pickers/cnnOneComponentP/requestorOptions.cpp
        src/services/scalable/pickers/cnnOneComponentP/service.cpp
        src/services/scalable/pickers/cnnOneComponentP/serviceOptions.cpp
        #src/services/scalable/pickers/cnnThreeComponentS/inferenceRequest.cpp
        #src/services/scalable/pickers/cnnThreeComponentS/inferenceResponse.cpp
        #src/services/scalable/pickers/cnnThreeComponentS/preprocessingRequest.cpp
        #src/services/scalable/pickers/cnnThreeComponentS/preprocessingResponse.cpp
        #src/services/scalable/pickers/cnnThreeComponentS/processingRequest.cpp
        #src/services/scalable/pickers/cnnThreeComponentS/processingResponse.cpp
        src/services/scalable/pickers/cnnThreeComponentS/requestor.cpp
        src/services/scalable/pickers/cnnThreeComponentS/requestorOptions.cpp
        src/services/scalable/pickers/cnnThreeComponentS/service.cpp
        src/services/scalable/pickers/cnnThreeComponentS/serviceOptions.cpp
        )
endif()
if (${BUILD_EW})
   set(EARTHWORM_SRC
       src/broadcasts/external/earthworm/traceBuf2.cpp
       src/broadcasts/external/earthworm/waveRing.cpp)
endif()
if (${BUILD_SEEDLINK})
   set(SEEDLINK_SRC
       src/broadcasts/external/seedlink/client.cpp
       src/broadcasts/external/seedlink/clientOptions.cpp
       src/broadcasts/external/seedlink/streamSelector.cpp)
endif()
if (${BUILD_UI})
   set(UI_SRC
       ui/qurts.cpp
       ui/mainWindow.cpp ui/mainWindow.hpp
       ui/waveformViewer/mainWindow.cpp #ui/waveformViewer/mainWindow.hpp 
       #ui/uOperatorOptions.cpp 
       ui/aqmsDatabaseOptionsDialog.cpp
       ui/uOperatorOptionsDialog.cpp
       ui/waveformViewer/channelSelectorDialog.cpp
       ui/waveformViewer/channelListModel.cpp
       ui/topics.cpp)
endif()

add_library(urts ${VERSION_SRC} ${GENERIC_SRC} ${GENERIC_MESSAGE_SRC} ${UUSS_SRC} ${UUSS_MESSAGE_SRC} ${EARTHWORM_SRC} ${SEEDLINK_SRC})
set_target_properties(urts PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES 
                      CXX_EXTENSIONS NO)
target_include_directories(urts
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                                   $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
                           PUBLIC ${UMPS_INCLUDE_DIR}
                           PRIVATE SOCI::soci_postgresql ${PostgreSQL_INCLUDE_DIRS} ${Time_INCLUDE_DIR} ${uLocator_INCLUDE_DIR} ${MAssociate_INCLUDE_DIR}
                           PUBLIC $<INSTALL_INTERFACE:${CMAKE_SOURCE_DIR}/include/urts>)
target_link_libraries(urts
                      PRIVATE ${UMPS_LIBRARY} ${Time_LIBRARY}
                              SOCI::soci_postgresql Boost::date_time Threads::Threads ${uLocator_LIBRARY} ${MAssociate_LIBRARY})
if (${BUILD_MSEED})
   target_include_directories(urts PRIVATE BEFORE ${MINISEED_INCLUDE_DIR})
   target_link_libraries(urts PRIVATE ${MINISEED_LIBRARY})
endif()
if (${BUILD_SEEDLINK})
   target_include_directories(urts PRIVATE ${SEEDLINK_INCLUDE_DIR})
   target_link_libraries(urts PRIVATE ${SEEDLINK_LIBRARY})
endif()
# Earthworm screws everything up.  They ripped a specific version of libmseed and
# jammed it into the earthworm include directory.  This stomps over the appropriate
# limbseed version.
if (${BUILD_EW})
    target_include_directories(urts PRIVATE ${EARTHWORM_INCLUDE_DIR})
    target_link_libraries(urts PRIVATE ${EARTHWORM_UTILITY_LIBRARY} ${EARTHWORM_MT_LIBRARY})
    target_compile_definitions(urts PRIVATE WITH_EARTHWORM)
endif()
if (${BUILD_UUSS_MLMODELS})
   target_include_directories(urts PRIVATE ${UUSSMLModels_INCLUDE_DIR})
   target_link_libraries(urts PRIVATE ${UUSSMLModels_LIBRARY})
endif()

##########################################################################################
#                                         Unit Tests                                     #
##########################################################################################
file(COPY ${CMAKE_SOURCE_DIR}/testing/data DESTINATION .)
set(TEST_SRC
    testing/main.cpp
    testing/database/aqms.cpp
    testing/broadcasts/internal/dataPacket.cpp
    testing/broadcasts/internal/pick.cpp
    testing/broadcasts/internal/probabilityPacket.cpp
    testing/services/standalone/incrementer.cpp
    testing/services/scalable/packetCache.cpp
    )
if (${BUILD_UUSS_MLMODELS})
   set(TEST_SRC ${TEST_SRC}
       testing/services/scalable/detectors/uNetThreeComponentP.cpp
       testing/services/scalable/detectors/uNetThreeComponentS.cpp
       testing/services/scalable/firstMotionClassifiers/cnnOneComponentP.cpp 
       testing/services/scalable/pickers/cnnOneComponentP.cpp
       testing/services/scalable/pickers/cnnThreeComponentS.cpp
)
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
target_link_libraries(unitTests PRIVATE urts ${UMPS_LIBRARY} ${GTEST_BOTH_LIBRARIES}
                                             ${UUSSMLModels_LIBRARY})
target_include_directories(unitTests
                           PRIVATE ${UMPS_INCLUDE_DIR} ${GTEST_INCLUDE_DIRS}
                                   ${UUSSMLModels_INCLUDE_DIR}
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

# Add the tests
add_test(NAME unitTests
         COMMAND unitTests)

##########################################################################################
#                                            Modules                                     #
##########################################################################################
add_executable(incrementerService src/services/standalone/incrementer/serviceModule.cpp)
set_target_properties(incrementerService PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
target_include_directories(incrementerService
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                           Boost::program_options)
target_link_libraries(incrementerService
                      PRIVATE urts ${UMPS_LIBRARY} Boost::program_options Threads::Threads)

add_executable(packetCacheService src/services/scalable/packetCache/serviceModule.cpp)
set_target_properties(packetCacheService PROPERTIES
                      CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS NO)
target_include_directories(packetCacheService
                           PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                           Boost::program_options)
target_link_libraries(packetCacheService
                      PRIVATE urts ${UMPS_LIBRARY} Boost::program_options Threads::Threads)

set(URTS_MODULES incrementerService packetCacheService)

if (${BUILD_EW})
   add_executable(broadcastWaveRing src/broadcasts/external/earthworm/broadcastModule.cpp)
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
   add_executable(broadcastSEEDLink src/broadcasts/external/seedlink/broadcastModule.cpp)
   set_target_properties(broadcastSEEDLink PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES 
                         CXX_EXTENSIONS NO) 
   target_include_directories(broadcastSEEDLink
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              Boost::program_options)
   target_link_libraries(broadcastSEEDLink
                         PRIVATE urts ${UMPS_LIBRARY}
                                 Boost::program_options Threads::Threads)
   set(URTS_MODULES ${MODULES} broadcastSEEDLink)
endif()

if (${BUILD_UUSS_MLMODELS})
   #add_executable(picksToDatabase src/modules/pickers/picksToDatabase.cpp)
   add_executable(mlDetector src/modules/detectors/mlDetector.cpp)
   add_executable(mlPickRegressor src/modules/pickers/mlPicker.cpp)
  #                               src/modules/pickers/triggerWindow.cpp
  #                               src/modules/pickers/thresholdDetectorOptions.cpp
  #                               src/modules/pickers/thresholdDetector.cpp)
   add_executable(thresholdPicker src/modules/pickers/threshold.cpp
                                  src/modules/pickers/triggerWindow.cpp
                                  src/modules/pickers/thresholdDetectorOptions.cpp
                                  src/modules/pickers/thresholdDetector.cpp)
   add_executable(uNetThreeComponentPService src/services/scalable/detectors/uNetThreeComponentP/serviceModule.cpp)
   add_executable(uNetThreeComponentSService src/services/scalable/detectors/uNetThreeComponentS/serviceModule.cpp)
   add_executable(pPickRegressorService src/services/scalable/pickers/cnnOneComponentP/serviceModule.cpp)
   add_executable(sPickRegressorService src/services/scalable/pickers/cnnThreeComponentS/serviceModule.cpp)
   add_executable(firstMotionClassifierService src/services/scalable/firstMotionClassifiers/cnnOneComponentP/serviceModule.cpp)
   #set_target_properties(picksToDatabase PROPERTIES
   #                      CXX_STANDARD 20
   #                      CXX_STANDARD_REQUIRED YES
   #                      CXX_EXTENSIONS NO)
   set_target_properties(mlDetector PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES 
                         CXX_EXTENSIONS NO)
   set_target_properties(mlPickRegressor PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   set_target_properties(thresholdPicker PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   set_target_properties(uNetThreeComponentPService PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   set_target_properties(uNetThreeComponentSService PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   set_target_properties(pPickRegressorService PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   set_target_properties(sPickRegressorService PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   set_target_properties(firstMotionClassifierService PROPERTIES
                         CXX_STANDARD 20
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO)
   #target_include_directories(picksToDatabase
    #                          PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    #                          PRIVATE ${UMPS_INCLUDE_DIR} ${PostgreSQL_INCLUDE_DIRS} Boost::program_options SOCI::soci_postgresql)
   target_include_directories(mlDetector
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE ${UMPS_INCLUDE_DIR} Boost::program_options)
   target_include_directories(mlPickRegressor
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE ${UMPS_INCLUDE_DIR} Boost::program_options)
   target_include_directories(thresholdPicker
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE ${UMPS_INCLUDE_DIR} Boost::program_options)
   target_include_directories(uNetThreeComponentPService
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE ${UMPS_INCLUDE_DIR} Boost::program_options)
   target_include_directories(uNetThreeComponentSService
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE ${UMPS_INCLUDE_DIR} Boost::program_options)
   target_include_directories(pPickRegressorService
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE ${UMPS_INCLUDE_DIR} Boost::program_options)
   target_include_directories(sPickRegressorService
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE ${UMPS_INCLUDE_DIR} Boost::program_options)
   target_include_directories(firstMotionClassifierService
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE ${UMPS_INCLUDE_DIR} Boost::program_options)
   #target_link_libraries(picksToDatabase
   #                      PRIVATE urts ${UMPS_LIBRARY}
   #                              Boost::program_options Threads::Threads SOCI::soci_postgresql)
   target_link_libraries(mlDetector
                         PRIVATE urts ${UMPS_LIBRARY} ${UUSSMLModels_LIBRARY}
                                 Boost::program_options Threads::Threads)
   target_link_libraries(mlPickRegressor
                         PRIVATE urts ${UMPS_LIBRARY} ${UUSSMLModels_LIBRARY}
                                 Boost::program_options Threads::Threads)
   target_link_libraries(thresholdPicker
                         PRIVATE urts ${UMPS_LIBRARY}
                                 Boost::program_options Threads::Threads)
   target_link_libraries(uNetThreeComponentPService
                         PRIVATE urts ${UMPS_LIBRARY} ${UUSSMLModels_LIBRARY}
                                 Boost::program_options Threads::Threads)
   target_link_libraries(uNetThreeComponentSService
                         PRIVATE urts ${UMPS_LIBRARY} ${UUSSMLModels_LIBRARY}
                                 Boost::program_options Threads::Threads)
   target_link_libraries(pPickRegressorService
                         PRIVATE urts ${UMPS_LIBRARY} ${UUSSMLModels_LIBRARY}
                                 Boost::program_options Threads::Threads)
   target_link_libraries(sPickRegressorService
                         PRIVATE urts ${UMPS_LIBRARY} ${UUSSMLModels_LIBRARY}
                                 Boost::program_options Threads::Threads)
   target_link_libraries(firstMotionClassifierService
                         PRIVATE urts ${UMPS_LIBRARY} ${UUSSMLModels_LIBRARY}
                                 Boost::program_options Threads::Threads)
   set(URTS_MODULES ${MODULES} mlDetector
                               mlPickRegressor
                               thresholdPicker
                               uNetThreeComponentPService
                               uNetThreeComponentSService
                               pPickRegressorService
                               sPickRegressorService
                               firstMotionClassifierService)
endif()

##########################################################################################
#                                       User Interface                                   #
##########################################################################################
if (${BUILD_UI})
   qt6_add_resources(QURTS_RESOURCES
                     ${CMAKE_SOURCE_DIR}/ui/resources/qurts.qrc)
   add_executable(qurts ${UI_SRC} ${QURTS_RESOURCES})
   set_target_properties(qurts PROPERTIES
                         CXX_STANDARD 17
                         CXX_STANDARD_REQUIRED YES
                         CXX_EXTENSIONS NO
                         AUTOMOC ON
                         AUTORCC ON
                         AUTOUIC ON)
   target_include_directories(qurts
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/ui>
                              PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
                              PRIVATE Qt6::Core Qt6::Widgets Qt6::Network
                                      ${QPHASE_INCLUDE_DIR}
                              PRIVATE ${UMPS_INCLUDE_DIR})
   target_link_libraries(qurts PRIVATE urts Qt6::Core Qt6::Widgets Qt6::Network
                                       ${UMPS_LIBRARY} ${QPHASE_LIBRARY})
endif()

##########################################################################################
#                                      Python Wrapping                                   #
##########################################################################################
if (${WRAP_PYTHON})
   message("Will build Python bindings")
   find_package(pybind11 REQUIRED)
   add_library(urtspy MODULE
               python/urtspy.cpp
               python/broadcasts.cpp
               python/broadcasts/dataPacket.cpp
               python/broadcasts/pick.cpp
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
