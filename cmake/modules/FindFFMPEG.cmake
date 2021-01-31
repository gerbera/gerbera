# Copyright (c) 1993-2015 Ken Martin, Will Schroeder, Bill Lorensen
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#  * Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
#  * Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
#  * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
#    of any contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# Find the native FFMPEG includes and library
#
# This module defines
# FFMPEG_INCLUDE_DIR, where to find avcodec.h, avformat.h ...
# FFMPEG_LIBRARIES, the libraries to link against to use FFMPEG.
# FFMPEG_FOUND, If false, do not try to use FFMPEG.

# also defined, but not for general use are
# FFMPEG_avformat_LIBRARY and FFMPEG_avcodec_LIBRARY, where to find the FFMPEG library.
# This is usefull to do it this way so that we can always add more libraries
# if needed to FFMPEG_LIBRARIES if ffmpeg ever changes...

# if ffmpeg headers are all in one directory
FIND_PATH(FFMPEG_INCLUDE_DIR avformat.h
        PATHS
        $ENV{FFMPEG_DIR}/include
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local/include
        /usr/include
        /sw/include # Fink
        /opt/local/include # DarwinPorts
        /opt/csw/include # Blastwave
        /opt/include
        /usr/freeware/include
        PATH_SUFFIXES ${CMAKE_LIBRARY_ARCHITECTURE} ffmpeg
        DOC "Location of FFMPEG Headers"
        )

# if ffmpeg headers are seperated to each of libavformat, libavcodec etc..
IF (NOT FFMPEG_INCLUDE_DIR)
    FIND_PATH(FFMPEG_INCLUDE_DIR libavformat/avformat.h
            PATHS
            $ENV{FFMPEG_DIR}/include
            ~/Library/Frameworks
            /Library/Frameworks
            /usr/local/include
            /usr/include
            /sw/include # Fink
            /opt/local/include # DarwinPorts
            /opt/csw/include # Blastwave
            /opt/include
            /usr/freeware/include
            PATH_SUFFIXES ${CMAKE_LIBRARY_ARCHITECTURE} ffmpeg
            DOC "Location of FFMPEG Headers"
            )

ENDIF (NOT FFMPEG_INCLUDE_DIR)

# we want the -I include line to use the parent directory of ffmpeg as
# ffmpeg uses relative includes such as <ffmpeg/avformat.h> or <libavcodec/avformat.h>
get_filename_component(FFMPEG_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR} ABSOLUTE)

FIND_PACKAGE(PkgConfig QUIET)
PKG_CHECK_MODULES(FFMPEG IMPORTED_TARGET GLOBAL libavformat libavutil)
IF (NOT FFMPEG_FOUND)
	FIND_LIBRARY(FFMPEG_avformat_LIBRARY avformat
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_avcodec_LIBRARY avcodec
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_avutil_LIBRARY avutil
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_swresample_LIBRARY swresample
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_vorbis_LIBRARY vorbis
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_ogg_LIBRARY ogg
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_dc1394_LIBRARY dc1394_control
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_vorbisenc_LIBRARY vorbisenc
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_theora_LIBRARY theora
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_dts_LIBRARY dts
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_gsm_LIBRARY gsm
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_swscale_LIBRARY swscale
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_z_LIBRARY z
		/usr/local/lib
		/usr/lib
		)

	FIND_LIBRARY(FFMPEG_bz2_LIBRARY bz2
		/usr/local/lib
		/usr/lib
		)
ENDIF(NOT FFMPEG_FOUND)

SET(FFMPEG_LIBRARIES)
IF (FFMPEG_INCLUDE_DIR)
    IF (FFMPEG_avformat_LIBRARY)
        IF (FFMPEG_avcodec_LIBRARY)
            IF (FFMPEG_avutil_LIBRARY)
                SET(FFMPEG_FOUND TRUE)
                SET(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
                SET(FFMPEG_BASIC_LIBRARIES
                        ${FFMPEG_avformat_LIBRARY}
                        ${FFMPEG_avcodec_LIBRARY}
                        ${FFMPEG_avutil_LIBRARY}
                        )

                # swresample is a dependency of avformat and avcodec
                IF (FFMPEG_swresample_LIBRARY)
                    LIST(APPEND FFMPEG_BASIC_LIBRARIES ${FFMPEG_swresample_LIBRARY})
                ENDIF (FFMPEG_swresample_LIBRARY)

                # swscale is always a part of newer ffmpeg distros
                IF (FFMPEG_swscale_LIBRARY)
                    LIST(APPEND FFMPEG_BASIC_LIBRARIES ${FFMPEG_swscale_LIBRARY})
                ENDIF (FFMPEG_swscale_LIBRARY)

                SET(FFMPEG_LIBRARIES ${FFMPEG_BASIC_LIBRARIES})

                IF (FFMPEG_vorbis_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_vorbis_LIBRARY})
                ENDIF (FFMPEG_vorbis_LIBRARY)

                # ogg is a dependency of vorbis 
                IF (FFMPEG_ogg_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_ogg_LIBRARY})
                ENDIF (FFMPEG_ogg_LIBRARY)

                IF (FFMPEG_dc1394_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_dc1394_LIBRARY})
                ENDIF (FFMPEG_dc1394_LIBRARY)

                IF (FFMPEG_vorbisenc_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_vorbisenc_LIBRARY})
                ENDIF (FFMPEG_vorbisenc_LIBRARY)

                IF (FFMPEG_theora_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_theora_LIBRARY})
                ENDIF (FFMPEG_theora_LIBRARY)

                IF (FFMPEG_dts_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_dts_LIBRARY})
                ENDIF (FFMPEG_dts_LIBRARY)

                IF (FFMPEG_gsm_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_gsm_LIBRARY})
                ENDIF (FFMPEG_gsm_LIBRARY)

                IF (FFMPEG_z_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_z_LIBRARY})
                ENDIF (FFMPEG_z_LIBRARY)

                IF (FFMPEG_bz2_LIBRARY)
                    LIST(APPEND FFMPEG_LIBRARIES ${FFMPEG_bz2_LIBRARY})
                ENDIF (FFMPEG_bz2_LIBRARY)

                SET(FFMPEG_LIBRARIES ${FFMPEG_LIBRARIES} CACHE INTERNAL "All presently found FFMPEG libraries.")

                # It is probablu not a good idea to define a target in PkgConfig namespace,
                # But alias targets are not correctly exported to try_compile
                IF (NOT TARGET PkgConfig::FFMPEG)
                    ADD_LIBRARY(PkgConfig::FFMPEG INTERFACE IMPORTED)
                    SET_PROPERTY(TARGET PkgConfig::FFMPEG PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}")
                    SET_PROPERTY(TARGET PkgConfig::FFMPEG PROPERTY INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}")                  
                ENDIF()
            ENDIF (FFMPEG_avutil_LIBRARY)
        ENDIF (FFMPEG_avcodec_LIBRARY)
    ENDIF (FFMPEG_avformat_LIBRARY)
ENDIF (FFMPEG_INCLUDE_DIR)

MARK_AS_ADVANCED(
        FFMPEG_INCLUDE_DIR
        FFMPEG_avformat_LIBRARY
        FFMPEG_avcodec_LIBRARY
        FFMPEG_avutil_LIBRARY
        FFMPEG_vorbis_LIBRARY
        FFMPEG_dc1394_LIBRARY
        FFMPEG_vorbisenc_LIBRARY
        FFMPEG_theora_LIBRARY
        FFMPEG_dts_LIBRARY
        FFMPEG_gsm_LIBRARY
        FFMPEG_swscale_LIBRARY
        FFMPEG_z_LIBRARY
)
