file(GLOB sources *.cc)
#add_library(${CMAKE_PROJECT_NAME} ${sources})
add_executable( ../${CMAKE_PROJECT_NAME} ${sources} )


find_package( Boost 1.37 COMPONENTS system program_options regex REQUIRED )
include_directories( ${Boost_INCLUDE_DIR} )
target_link_libraries( ../${CMAKE_PROJECT_NAME} ${Boost_LIBRARIES} mysqlcppconn )

include_directories( ../lib ../lib/mysql )

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
  message(STATUS "GCC detected, adding compile flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O1")
endif(CMAKE_COMPILER_IS_GNUCXX)
