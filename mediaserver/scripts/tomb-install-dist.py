#!/usr/bin/env python

# source location of shared files, accessible by all users
DEFAULT_SOURCE = "__DEFAULT_SOURCE__" 
# server configuration directory
DEFAULT_SERVER = "~/.mediatomb"
# name of the config file
DEFAULT_CONFIG = "config.xml"
# name of the sqlite3 database
DEFAULT_DABASE = "sqlite3.db"
# default charset used by the filesystem
DEFAULT_FSCSET = "ISO-8859-1"
# default storage driver
DEFAULT_DRIVER = "sqlite3"
# default server friendly name
DEFAULT_FRNAME = "MediaTomb"
# default webroot directory (shared between users)
DEFAULT_WBROOT = "web"
#default state for the UI is disabled!
DEFAULT_UISTAT = "no"
try:
    import os.path
    import sys
    import string
    import getopt
    import commands

# that is very unlikely... but you never know :>
except ImportError:
    print "Error: Could not import os.path and sys modules!"
    print "Please check your python installation."

class InstallError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return str(self.value)
    
def print_usage():
    a = sys.argv[0]
    print 
    print "Usage:", a[0].lstrip("./"), "[options]"
    print
    print "Supported options:"
    print "   --sourcedir or -s     Directory where the original files reside."
    print "   --db-only or -d       Only create the database, must specify name"
    print "   --help or -h          This help text."
    print 
   
def is_clean():
    if os.path.isdir(os.path.expanduser(DEFAULT_SERVER)):
        raise InstallError("\n\nA previous server configuration was found. " +\
                            "If you want to reinstall the server,\nremove "  +\
                            "the " + DEFAULT_SERVER + " directory manually"  +\
                            " and run this script again.")

# checks if the source of the installation is available and looks ok        
def check_source(dir):
    if (not os.path.isdir(dir)):
        raise InstallError("\n\nSource directory " + dir + " does not exist!")
        
    if (not os.path.isdir(os.path.join(dir, "icons"))):
        raise InstallError("\n\nSource directory does not seem to be valid:\n"+\
                           os.path.join(dir, "icons") + " not found.")

# checks write permission in the users home directory... well, that should
# always work, but sure is sure...
# also checks if we can actually read from the source directory
def check_permissions(dir):
    if (not os.access(os.path.expanduser("~"), os.W_OK)):
        raise InstallError("No write access to " + os.path.expanduser("~"))

    if (not os.access(dir, os.R_OK)):
        raise InstallERror("\nNo read access to source directory " + dir)
   
# create directory structure        
def create_dirs():
    try:
        os.makedirs(os.path.expanduser(DEFAULT_SERVER))
    except IOError, (errno, strerror):
        message =  "I/O error(%s): %s" % (errno, strerror)
        raise InstallError(message)
    except: 
        raise InstallError("\nUnexpected error when creating directory " +\
                           "structure.")

# create links to the icons, the web ui files and the service description
# files
# since I am too lazy to write an own routine that would create links 
# recursively, I will simply run "ln -s /sourcedir/* ~/.mediatomb"
def create_links(dir):
    try:
        os.symlink(dir, os.path.join(os.path.expanduser(DEFAULT_SERVER), DEFAULT_WBROOT))
    except:
        raise InstallError("Could not link files!")

# create sqlite3 database
def create_database(name):
    s =  "CREATE TABLE media_files \
( \
    id              INTEGER PRIMARY KEY DEFAULT 0, \
    parent_id       INTEGER NOT NULL, \
    upnp_class      VARCHAR(80), \
    dc_title        VARCHAR(256), \
    dc_description  TEXT, \
    restricted      INTEGER NOT NULL DEFAULT 0, \
\
    update_id       INTEGER NOT NULL DEFAULT 0, \
    searchable      INTEGER NOT NULL DEFAULT 0, \
\
    location        VARCHAR(256), \
    mime_type       VARCHAR(80), \
\
    action          VARCHAR(256), \
    state           VARCHAR(256), \
\
    object_type     INTEGER NOT NULL \
); \
\n\
CREATE INDEX media_files_parent_id ON media_files(parent_id);\n\
CREATE INDEX media_files_object_type ON media_files(object_type);\n\
\n\
INSERT INTO media_files(id, parent_id, object_type, dc_title, upnp_class) \
    VALUES (0, -1, 1, 'Root', 'object.container');"

    out = commands.getstatusoutput("echo \"" + s + "\" | sqlite3 " +\
          os.path.join(os.path.expanduser(DEFAULT_SERVER), name))

    if (out[0] != 0):
        raise InstallError("Could not create database:\n" +\
            os.path.join(os.path.expanduser(DEFAULT_SERVER), name) +\
            "\n" + out[1])

# finally create and write the config.xml with default settings
def write_config():
    cfg = "<config>\n" +\
          "  <server>\n" +\
          "    <name>" + DEFAULT_FRNAME + "</name>\n" +\
          "    <udn/>\n" +\
          "    <home>" + os.path.expanduser(DEFAULT_SERVER) + "</home>\n" +\
          "    <webroot>" + DEFAULT_WBROOT + "</webroot>\n" +\
          "    <storage driver=\"" + DEFAULT_DRIVER + "\">\n" +\
          "      <database-file>" + DEFAULT_DABASE + "</database-file>\n" +\
          "    </storage>\n" +\
          "    <ui enabled=\"" + DEFAULT_UISTAT + "\"/>" +\
          "  </server>\n" +\
          "  <import>\n" +\
          "    <filesystem-charset>"+ DEFAULT_FSCSET + "</filesystem-charset>\n" +\
          "  </import>\n" +\
          "</config>\n\n"

    try:
        f = open(os.path.join(os.path.expanduser(DEFAULT_SERVER),\
                    DEFAULT_CONFIG), 'w')


        f.write(cfg)
        f.close()

    except IOError, (errno, strerror):
        message =  "I/O error(%s): %s" % (errno, strerror)
        raise InstallError(message)
#    except:
#        raise InstallError("\nUnexpected error when writing config file.")

# this function sets up a ~/.mediatomb directory with all necessary
# components
# the source of the installation is the "dir" parameter
# the source directory must have a specific strucutre for this function to
# understand it
def install_from(dir):
    check_source(dir)
    check_permissions(dir)
    print "Creating directories...",
    create_dirs()
    print "ok"
    print "Creating links...",
    create_links(dir)
    print "ok"
    print "Creating database...",
    create_database(DEFAULT_DABASE)
    print "ok"
    print "Writing configuration...",
    write_config()
    print "ok"
    print
    print "All done. You are now ready to launch MediaTomb!"
    print
    print "WARNING: Because of security reasons the UI is disabled by default!"
    print "Please refer to the README file on how to enable it."
    print 
    print "Once it is enabled:"
    print "When the server is running you can access the UI by opening"
    print DEFAULT_SERVER + "/index.html in your web browser."
    print


# check command line arguments and start installation if appropriate
def main():
    print
    print "MediaTomb installer v0.1"
    print
    
    src = DEFAULT_SOURCE
    db  = "" 

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:s:", ["help", "db-only=",\
                                                          "sourcedir="])

    except getopt.GetoptError:
        print_usage()
        sys.exit(2)
        
    for o, a in opts:
        if o in ("-h", "--help"):
            print_usage()
            sys.exit()
    
        if o in ("-d", "--db-only"):
            db = a.lstrip()

        if o in ("-s", "--sourcedir"):
            src = a.lstrip()
            if (not os.path.isdir(src)):
                print
                print "Aborting MediaTomb installation! Could not find source directory:", src
                print
                sys.exit(1)

    exitval = 0
    
    try:
        if (db == ""):
            print "Proceeding normally"
            is_clean()
            install_from(src)
        else:
            if (os.path.exists(os.path.join(os.path.expanduser(DEFAULT_SERVER),\
                                            db))):
                raise InstallError("\nWill not overwrite existing database! " +\
                        os.path.join(os.path.expanduser(DEFAULT_SERVER), db))

            print "Creating database:", db, "...",
            create_database(db)
            print "OK"
            print
            print "Make sure that your config file reflects the database name."
            print
            
    except InstallError, e:
        print
        print "Aborting MediaTomb installation! " + str(e)
        print
        exitval = 1
    except:
        print
        print "Aborting MediaTomb installation! Unexpected error."
        print
        exitval = 1
        
    sys.exit(exitval)


if __name__ == "__main__":
    main()
