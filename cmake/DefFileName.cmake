# Based on deffilename.cmake from
# https://github.com/cdesjardins/include
#
# Helper function to add preprocesor definition of __FILENAME__
# to pass the filename without directory path for debugging use.
#
# Example:
#
#   define_file_path_for_sources(my_target)
#
# Will add -D__FILENAME__="filename" for each source file depended on
# by my_target, where filename is the path relative to CMAKE_SOURCE_DIR.
#
function(define_file_path_for_sources targetname)
    get_target_property(source_files "${targetname}" SOURCES)
    foreach(sourcefile ${source_files})
        # Get source file's current list of compile definitions.
        get_property(defs SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS)
        # Add the __FILENAME__=filename compile definition to the list.
        string(REPLACE "${CMAKE_SOURCE_DIR}" "" NAME "${sourcefile}")
        list(APPEND defs "__FILENAME__=\"${NAME}\"")
        # Set the updated compile definitions on the source file.
        set_property(
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS ${defs})
    endforeach()
endfunction()