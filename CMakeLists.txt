CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#Change PROJECT_NAME to the name of your project
PROJECT(OpenDungeons)
SET( PROJECT_BINARY_NAME "MapEditor")

# project version
SET( ${PROJECT_NAME}_MAJOR_VERSION 0 )
SET( ${PROJECT_NAME}_MINOR_VERSION 1 )
SET( ${PROJECT_NAME}_PATCH_LEVEL 1 )

SET( CMAKE_MODULE_PATH    ${CMAKE_MODULE_PATH}
                          ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules )

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

#Declare any external dependencies that your project may have here.
SET(Required_Packages
   OIS
   OGRE
   CEGUI
   CEGUIOGRE
)


# searches for all .cpp and .h files in the current directory and add them 
# to the current project

FILE(GLOB folder_source ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
SOURCE_GROUP(${PROJECT_BINARY_NAME} FILES ${folder_source})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

IF (NOT WIN32) # Unix
    # Add compiler warnings
    ADD_DEFINITIONS( "-Wall -Wunused -ansi" )
ENDIF (NOT WIN32)

IF (MSVC_IDE)
	IF (EXISTS $ENV{OGRE_HOME})
		SET(OGRESDK $ENV{OGRE_HOME})
		STRING(REGEX REPLACE "[\\]" "/" OGRESDK "${OGRESDK}")
		MESSAGE(STATUS "Adding Additional Link Directories")
		LINK_DIRECTORIES(${OGRESDK}/lib)
	ENDIF(EXISTS $ENV{OGRE_HOME})

	SET(PLATFORM_C_FLAGS "/W3 /MD /Od /D \"WIN32\" /D \"_WINDOWS\" /Gm /Gy /fp:fast /ZI /EHsc /SUBSYSTEM:WINDOWS /ENTRY:\"main\"")
	SET(PLATFORM_C_FLAGS_DEBUG "/W3 /MDd /Od /Gm /Gy /fp:fast /ZI")

	SET(CMAKE_CXX_FLAGS "${PLATFORM_C_FLAGS}")
	SET(CMAKE_CXX_FLAGS_RELEASE "${PLATFORM_C_FLAGS}")
	SET(CMAKE_CXX_FLAGS_DEBUG "${PLATFORM_C_FLAGS_DEBUG}")
ENDIF (MSVC_IDE)

# create the binary file
ADD_EXECUTABLE(${PROJECT_BINARY_NAME} WIN32 ${folder_source})

#this foreach loads all of the packages that you specified as required.
#It shouldn't need to be modified.
FOREACH(Package ${Required_Packages})
  FIND_PACKAGE(${Package} REQUIRED)
  IF (${Package}_FOUND)
      INCLUDE_DIRECTORIES(${${Package}_INCLUDE_DIR})
      IF (${Package}_LIBRARIES)
         TARGET_LINK_LIBRARIES(${PROJECT_BINARY_NAME} ${${Package}_LIBRARIES})
      ELSE (${Package}_LIBRARIES)
         LINK_DIRECTORIES(${${Package}_LIBRARY_DIRS})
      ENDIF (${Package}_LIBRARIES)
  ELSE (${Package}_FOUND)
      MESSAGE(STATUS "${Package} not found")
  ENDIF(${Package}_FOUND)
ENDFOREACH(Package)

