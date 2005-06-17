
SUBDIRS = mxml zmm zmmf storage web uuid md5 scripting

noinst_LIBRARIES = libmediatomb.a

libmediatomb_a_SOURCES = \
    action_request.h \
    action_request.cc \
    cds_objects.h \
    cds_objects.cc \
    cds_resource_manager.h \
    cds_resource_manager.cc \
    common.h \
    config_manager.h \
    config_manager.cc \
    content_manager.h \
    content_manager.cc \
    destroyer.h \
    destroyer.cc \
    dictionary.h \
    dictionary.cc \
    exceptions.h \
    exceptions.cc \
    file_io_handler.h \
    file_io_handler.cc \
    file_request_handler.h \
    file_request_handler.cc \
    io_handler.h \
    io_handler.cc \
    logger.h \
    logger.cc \
    metadata_reader.h \
    metadata_reader.cc \
    mem_io_handler.h \
    mem_io_handler.cc \
    process.h \
    process.cc \
    rexp.h \
    rexp.cc \
    request_handler.h \
    request_handler.cc\
    server.h \
    server.cc \
    session_manager.h \
    session_manager.cc \
    storage.h \
    storage.cc \
    string_converter.h \
    string_converter.cc \
    subscription_request.h \
    subscription_request.cc \
    tools.h \
    tools.cc \
    update_manager.h \
    update_manager.cc \
    upnp_cds.h \
    upnp_cds.cc \
    upnp_cds_actions.cc \
    upnp_cds_subscriptions.cc \
    upnp_cm.h \
    upnp_cm.cc \
    upnp_cm_actions.cc \
    upnp_cm_subscriptions.cc \
    upnp_xml.h \
    upnp_xml.cc \
    web_callbacks.h \
    web_callbacks.cc \
    web_request_handler.h \
    web_request_handler.cc \
    xpath.h \
    xpath.cc

INCLUDES=@common_includes@

bin_PROGRAMS = mediatomb

COMMON_LDADD = \
    libmediatomb.a \
    web/libweb.a \
    storage/sqlite3/libsqlite3.a \
    libmediatomb.a \
    storage/fs/libfs.a \
    storage/libstorage.a \
    mxml/libmxml.a \
    zmmf/libzmmf.a \
    zmm/libzmm.a \
    uuid/libuuid.a \
    md5/libmd5.a \
    scripting/libscripting.a \
    -lupnp -lixml -lthreadutil -lsqlite3 -lmagic -lid3 -lexiv2 @js_libs@

mediatomb_SOURCES = main.cc
mediatomb_LDADD = $(COMMON_LDADD)
   
