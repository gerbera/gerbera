# Find a UUID Implementation
#
# Supports e2fsprogs UUID or BSD native UUID
#
# UUID_FOUND               True if a uuid got found
# UUID_INCLUDE_DIRS        Location of uuid headers
# UUID_LIBRARIES           List of libraries to use uuid
# FOUND_BSD_UUID           BSD UUID implementation found
# FOUND_LIBUUID            e2fsprogs UUID found
#

INCLUDE(CheckCXXSourceRuns)
INCLUDE(FindPackageHandleStandardArgs)

message(STATUS "Looking for BSD native UUID")
set(CMAKE_REQUIRED_QUIET 1)
check_cxx_source_runs("
			#include <uuid.h>
			int main(void) {
				 uuid_t uuid;
				 uint32_t status;
				 uuid_create(&uuid, &status);
				 return 0;
			}" FOUND_BSD_UUID)

if (FOUND_BSD_UUID)
	message(STATUS "Looking for BSD native UUID - found")
	set(UUID_FOUND 1)

	FIND_PATH(UUID_INCLUDE_DIRS uuid.h)
	add_library(UUID::UUID INTERFACE IMPORTED)
	set_property(TARGET UUID::UUID PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${UUID_INCLUDE_DIRS}")
	set_property(TARGET UUID::UUID PROPERTY INTERFACE_COMPILE_DEFINITIONS "BSD_NATIVE_UUID")
endif()

if(NOT UUID_FOUND)
	message(STATUS "Looking for libuuid")
	find_package(PkgConfig QUIET)
	pkg_search_module(PC_UUID QUIET IMPORTED_TARGET GLOBAL uuid)
	if(PC_UUID_FOUND)
		message(STATUS "Looking for libuuid - found")
		add_library(UUID::UUID ALIAS PkgConfig::PC_UUID)
		set(UUID_FOUND 1)
	else()
		FIND_PATH(UUID_INCLUDE_DIRS uuid/uuid.h)
		FIND_LIBRARY(UUID_LIBRARIES uuid)

		if(UUID_INCLUDE_DIRS)
			message(STATUS "Looking for libuuid - found")

			if(NOT TARGET UUID::UUID)
				add_library(UUID::UUID INTERFACE IMPORTED)
				set_property(TARGET UUID::UUID PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${UUID_INCLUDE_DIRS}")
				set_property(TARGET UUID::UUID PROPERTY INTERFACE_LINK_LIBRARIES "${UUID_LIBRARIES}")
			endif()

			set(UUID_FOUND 1)
		endif()
	endif()
endif()

find_package_handle_standard_args(UUID DEFAULT_MSG UUID_FOUND)
MARK_AS_ADVANCED(UUID_LIBRARIES UUID_INCLUDE_DIRS)
