cmake_minimum_required(VERSION 3.10)
project(DPS-Coursework-2023)

############################## Custom build modes ###############################

set(CMAKE_CXX_FLAGS_SANITIZE "-fno-omit-frame-pointer -fsanitize=address,signed-integer-overflow,null,alignment,bounds,function,return,vla-bound -O0 -g"
  CACHE STRING
  "Flags used by the C++ compiler during Sanitize builds."
  FORCE)
set(CMAKE_C_FLAGS_SANITIZE "-fno-omit-frame-pointer -fsanitize=address,signed-integer-overflow,null,alignment,bounds,function,return,vla-bound -O0 -g"
  CACHE STRING
  "Flags used by the C compiler during Sanitize builds."
  FORCE)
set(CMAKE_EXE_LINKER_FLAGS_SANITIZE
  ${CMAKE_EXE_LINKER_FLAGS_DEBUG} CACHE STRING
  "Flags used for linking binaries during Sanitize builds."
  FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_SANITIZE
  ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} CACHE STRING
  "Flags used by the shared libraries linker during Sanitize builds."
  FORCE)
mark_as_advanced(
  CMAKE_CXX_FLAGS_SANITIZE		  CMAKE_EXE_LINKER_FLAGS_SANITIZE
  CMAKE_C_FLAGS_SANITIZE		  CMAKE_SHARED_LINKER_FLAGS_SANITIZE
  )

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
  "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Sanitize."
  FORCE)

############################### External Projects ###############################

include(ExternalProject)

ExternalProject_Add(googlebenchmark
	URL "https://github.com/google/benchmark/archive/refs/tags/v1.7.1.tar.gz"
  DOWNLOAD_DIR $ENV{HOME}/.cmake-downloads/${CMAKE_PROJECT_NAME}
		CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DPS-Coursework-2023_BINARY_DIR}/deps -DBENCHMARK_DOWNLOAD_DEPENDENCIES=ON -DBENCHMARK_ENABLE_GTEST_TESTS=OFF $<IF:$<CONFIG:>,,-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}> -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
  BUILD_BYPRODUCTS ${DPS-Coursework-2023_BINARY_DIR}/deps/lib/${CMAKE_SHARED_LIBRARY_PREFIX}benchmark${CMAKE_STATIC_LIBRARY_SUFFIX} ${DPS-Coursework-2023_BINARY_DIR}/deps/lib/${CMAKE_SHARED_LIBRARY_PREFIX}benchmark_main${CMAKE_STATIC_LIBRARY_SUFFIX}
  )

ExternalProject_Add(catch2
	URL https://github.com/catchorg/Catch2/archive/refs/tags/v2.13.9.tar.gz
  DOWNLOAD_DIR $ENV{HOME}/.cmake-downloads/${CMAKE_PROJECT_NAME}
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${DPS-Coursework-2023_BINARY_DIR}/deps -DCATCH_BUILD_TESTING=NO -DBUILD_TESTING=NO -DCATCH_ENABLE_WERROR=NO -DCATCH_INSTALL_DOCS=NO -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
)

if(NOT DEFINED VELOX_PATH)
    set(VELOX_PATH "/usr/local/velox" CACHE STRING "the path to the velox code")
endif()

if(NOT DEFINED BUILT_VELOX_PATH)
    set(BUILT_VELOX_PATH "${VELOX_PATH}/_build/release" CACHE STRING "the path to the (built) velox")
endif()


set(VELOX_LINK_DIRECTORIES
  /usr/lib/x86_64-linux-gnu
  ${VELOX_PATH}/_deps/protobuf-build
  ${BUILT_VELOX_PATH}/lib
  ${BUILT_VELOX_PATH}/velox/common/base
  ${BUILT_VELOX_PATH}/velox/common/base/lib  
  ${BUILT_VELOX_PATH}/velox/common/caching
  ${BUILT_VELOX_PATH}/velox/common/encode
  ${BUILT_VELOX_PATH}/velox/common/file
  ${BUILT_VELOX_PATH}/velox/common/hyperloglog
  ${BUILT_VELOX_PATH}/velox/common/memory
  ${BUILT_VELOX_PATH}/velox/common/process
  ${BUILT_VELOX_PATH}/velox/common/process/
  ${BUILT_VELOX_PATH}/velox/common/serialization
  ${BUILT_VELOX_PATH}/velox/common/testutil
  ${BUILT_VELOX_PATH}/velox/common/time
  ${BUILT_VELOX_PATH}/velox/connectors
  ${BUILT_VELOX_PATH}/velox/core
  ${BUILT_VELOX_PATH}/velox/dwio/common
  ${BUILT_VELOX_PATH}/velox/dwio/common/compression
  ${BUILT_VELOX_PATH}/velox/dwio/common/encryption
  ${BUILT_VELOX_PATH}/velox/dwio/common/exception
  ${BUILT_VELOX_PATH}/velox/dwio/dwrf/common
  ${BUILT_VELOX_PATH}/velox/dwio/dwrf/utils
  ${BUILT_VELOX_PATH}/velox/dwio/dwrf/writer
  ${BUILT_VELOX_PATH}/velox/exec
  ${BUILT_VELOX_PATH}/velox/exec/tests/utils
  ${BUILT_VELOX_PATH}/velox/expression
  ${BUILT_VELOX_PATH}/velox/expression/type_calculation
  ${BUILT_VELOX_PATH}/velox/external/date
  ${BUILT_VELOX_PATH}/velox/functions
  ${BUILT_VELOX_PATH}/velox/functions/lib
  ${BUILT_VELOX_PATH}/velox/functions/prestosql/aggregates
  ${BUILT_VELOX_PATH}/velox/parse
  ${BUILT_VELOX_PATH}/velox/serializers
  ${BUILT_VELOX_PATH}/velox/tpch/gen
  ${BUILT_VELOX_PATH}/velox/type
  ${BUILT_VELOX_PATH}/velox/type/tz
  ${BUILT_VELOX_PATH}/velox/vector
  ${BUILT_VELOX_PATH}/velox/vector/tests/utils
  ${VELOX_PATH}/fmt/_build
  ${VELOX_PATH}/folly/_build
)

set(
  VELOX_LIBRARIES
  velox_aggregates
  velox_presto_serializer
  velox_dwio_common_compression
  velox_dwio_common_exception
  velox_exception
  velox_memory
  velox_parse_expression
  velox_parse_parser
  velox_process
  velox_serialization
  velox_test_util
  velox_connector
  velox_core
  velox_encode
  velox_expression_functions
  velox_type_calculation
  velox_file
  velox_process
  velox_caching
  velox_function_registry
  velox_expression
  velox_functions_util
  velox_common_base
  velox_time
  velox_vector
  
  velox_type
  velox_external_date
  velox_dwio_common_encryption
  velox_dwio_dwrf_writer
  velox_dwio_dwrf_utils
  velox_dwio_dwrf_common
  velox_dwio_common
  velox_config
  velox_common_hyperloglog
  velox_functions_lib
  velox_exec
  velox_connector

  double-conversion
  event
  folly
  unwind
  boost_regex
  fmt
  gflags
  glog
  z
  gtest

  boost_context

  velox_type_tz
  zstd
  snappy-shared
  lz4
  velox_common_hyperloglog


  atomic
)



#################################### Targets ####################################


set(ImplementationFiles Source/DPS-Coursework-2023.cpp)
set(ExecutableFiles Source/main.cpp)
set(TestFiles Tests/DPS-Coursework-2023Tests.cpp)
set(BenchmarkFiles Benchmarks/DPS-Coursework-2023Benchmarks.cpp)

add_executable(Tests ${ImplementationFiles} ${TestFiles})
set_property(TARGET Tests PROPERTY CXX_STANDARD 20)
target_include_directories(Tests SYSTEM PUBLIC ${DPS-Coursework-2023_BINARY_DIR}/deps/include)
add_dependencies(Tests catch2)
target_include_directories(Tests SYSTEM PUBLIC ${VELOX_PATH})
target_include_directories(Tests SYSTEM PUBLIC ${VELOX_PATH}/third_party/xsimd/include)
target_link_libraries(Tests PUBLIC ${VELOX_LIBRARIES})
target_link_directories(Tests PUBLIC ${VELOX_LINK_DIRECTORIES})
target_compile_options(Tests PRIVATE -march=native)


add_executable(Benchmarks ${ImplementationFiles} ${BenchmarkFiles})
set_property(TARGET Benchmarks PROPERTY CXX_STANDARD 20)
target_include_directories(Benchmarks SYSTEM PUBLIC ${VELOX_PATH})
target_include_directories(Benchmarks SYSTEM PUBLIC ${VELOX_PATH}/third_party/xsimd/include)
target_link_libraries(Benchmarks PUBLIC ${VELOX_LIBRARIES})
target_link_directories(Benchmarks PUBLIC ${VELOX_LINK_DIRECTORIES})
target_compile_options(Benchmarks PRIVATE -march=native)

# set(THREADS_PREFER_PTHREAD_FLAG ON)
# find_package(Threads REQUIRED)
# target_link_libraries(Benchmarks Public Threads::Threads)

add_dependencies(Benchmarks googlebenchmark)
target_include_directories(Benchmarks SYSTEM PUBLIC ${DPS-Coursework-2023_BINARY_DIR}/deps/include)
target_link_libraries(Benchmarks PRIVATE ${DPS-Coursework-2023_BINARY_DIR}/deps/lib/${CMAKE_SHARED_LIBRARY_PREFIX}benchmark.a)
target_link_libraries(Benchmarks PRIVATE ${DPS-Coursework-2023_BINARY_DIR}/deps/lib/${CMAKE_SHARED_LIBRARY_PREFIX}benchmark_main.a)

