# ~~~
#
#  FIND_LIBRARY_WITH_DEBUG
#  -> enhanced FIND_LIBRARY to allow the search for an
#     optional debug library with a WIN32_DEBUG_POSTFIX similar
#     to CMAKE_DEBUG_POSTFIX when creating a shared lib
#     it has to be the second and third argument
#
# Copyright (c) 2007, Christian Ehrlicher, <ch.ehrlicher@gmx.de>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
# ~~~

macro(FIND_LIBRARY_WITH_DEBUG var_name win32_dbg_postfix_name dgb_postfix libname)

    if(NOT "${win32_dbg_postfix_name}" STREQUAL "WIN32_DEBUG_POSTFIX")

        # no WIN32_DEBUG_POSTFIX -> simply pass all arguments to FIND_LIBRARY
        find_library(${var_name} ${win32_dbg_postfix_name} ${dgb_postfix} ${libname} ${ARGN})

    else(NOT "${win32_dbg_postfix_name}" STREQUAL "WIN32_DEBUG_POSTFIX")

        if(NOT WIN32)
            # on non-win32 we don't need to take care about WIN32_DEBUG_POSTFIX

            find_library(${var_name} ${libname} ${ARGN})

        else(NOT WIN32)

            # 1. get all possible libnames
            set(args ${ARGN})
            set(newargs "")
            set(libnames_release "")
            set(libnames_debug "")

            list(LENGTH args listCount)

            if("${libname}" STREQUAL "NAMES")
                set(append_rest 0)
                list(APPEND args " ")

                foreach(i RANGE ${listCount})
                    list(GET args ${i} val)

                    if(append_rest)
                        list(APPEND newargs ${val})
                    else(append_rest)
                        if("${val}" STREQUAL "PATHS")
                            list(APPEND newargs ${val})
                            set(append_rest 1)
                        else("${val}" STREQUAL "PATHS")
                            list(APPEND libnames_release "${val}")
                            list(APPEND libnames_debug "${val}${dgb_postfix}")
                        endif("${val}" STREQUAL "PATHS")
                    endif(append_rest)

                endforeach(i)

            else("${libname}" STREQUAL "NAMES")

                # just one name
                list(APPEND libnames_release "${libname}")
                list(APPEND libnames_debug "${libname}${dgb_postfix}")

                set(newargs ${args})

            endif("${libname}" STREQUAL "NAMES")

            # search the release lib
            find_library(${var_name}_RELEASE NAMES ${libnames_release} ${newargs})

            # search the debug lib
            find_library(${var_name}_DEBUG NAMES ${libnames_debug} ${newargs})

            if(${var_name}_RELEASE AND ${var_name}_DEBUG)

                # both libs found
                set(${var_name} optimized ${${var_name}_RELEASE} debug ${${var_name}_DEBUG})

            else(${var_name}_RELEASE AND ${var_name}_DEBUG)

                if(${var_name}_RELEASE)

                    # only release found
                    set(${var_name} ${${var_name}_RELEASE})

                else(${var_name}_RELEASE)

                    # only debug (or nothing) found
                    set(${var_name} ${${var_name}_DEBUG})

                endif(${var_name}_RELEASE)

            endif(${var_name}_RELEASE AND ${var_name}_DEBUG)

            mark_as_advanced(${var_name}_RELEASE)
            mark_as_advanced(${var_name}_DEBUG)

        endif(NOT WIN32)

    endif(NOT "${win32_dbg_postfix_name}" STREQUAL "WIN32_DEBUG_POSTFIX")

endmacro(FIND_LIBRARY_WITH_DEBUG)
