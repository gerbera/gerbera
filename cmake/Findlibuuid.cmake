# Find a UUID Implementaion
#
# Supports e2fsprogs libuuid or BSD native UUID
#
# libuuid_FOUND               True if a uuid got found
# libuuid_INCLUDE_DIRS        Location of uuid headers
# libuuid_LIBRARIES           List of libraries to use uuid
# FOUND_BSD_UUID           BSD UUID implementation found
# FOUND_LIBUUID            e2fsprogs UUID found
#

INCLUDE(FindPkgConfig)
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
	set(libuuid_FOUND 1)
endif()

if(NOT libuuid_FOUND)
	message(STATUS "Looking for libuuid")
	pkg_search_module(_UUID libuuid QUIET)
	if(NOT _UUID_FOUND)
		FIND_PATH(libuuid_INCLUDE_DIRS uuid/uuid.h)
		FIND_LIBRARY(libuuid_LIBRARIES uuid)

		if(libuuid_INCLUDE_DIRS)
			message(STATUS "Looking for libuuid - found")
			set(libuuid_FOUND 1)
			set(FOUND_LIBUUID 1)
		endif()
	endif()
endif()

find_package_handle_standard_args(libuuid DEFAULT_MSG libuuid_FOUND)
MARK_AS_ADVANCED(libuuid_LIBRARIES libuuid_INCLUDE_DIRS)
