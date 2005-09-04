#!/usr/bin/env python
TOMB_INSTALL_VERSION = "0.3"
# source location of shared files, accessible by all users
DEFAULT_SOURCE = "__DEFAULT_SOURCE__" 
# server configuration directory
DEFAULT_SERVER = "~/.mediatomb"
# name of the config file
DEFAULT_CONFIG = "config.xml"
# name of the distribution config file
DEFAULT_DISCFG = "config-dist.xml"
# name of the sqlite3 database
DEFAULT_DABASE = "sqlite3.db"
# default server friendly name
DEFAULT_FRNAME = "MediaTomb"
# default webroot directory (shared between users)
DEFAULT_WBROOT = "web"
#default file that is holding the database table
DEFAULT_DBTABL = "sqlite3.sql"
#default import script
DEFAULT_ISCRPT = "import.js"
#default directory for the import scripts
DEFAULT_IJSDIR = "js"

try:
    import os.path
    import sys
    import string
    import getopt
    import commands
    import socket
    import getpass

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
    print "   --targetdir or -t     Target directory for the installation (default: ~/.mediatomb)"
    print "   --sourcedir or -s     Directory where the original files reside."
    print "   --db-only or -d       Only create the database, must specify name"
    print "   --help or -h          This help text."
    print 
   
def is_clean(DEFAULT_SERVER):
    if os.path.isdir(os.path.expanduser(DEFAULT_SERVER)):
        raise InstallError("\n\nA previous server configuration was found. " +\
                            "If you want to reinstall the server,\nremove "  +\
                            "the " + DEFAULT_SERVER + " directory manually"  +\
                            " and run this script again.")

# checks if the source of the installation is available and looks ok        
def check_source(dir):
    if (not os.path.isdir(dir)):
        raise InstallError("\n\nSource directory " + dir + " does not exist!")
       
    if (not os.path.isdir(os.path.join(dir, DEFAULT_WBROOT))):
        raise InstallError("\n\nSource directory does not seem to be valid:\n"+\
                           os.path.join(dir, DEFAULT_WBROOT) + " not found.")

    if (not os.path.isdir(os.path.join(os.path.join(dir, DEFAULT_WBROOT), "icons"))):
        raise InstallError("\n\nSource directory does not seem to be valid:\n"+\
                           os.path.join(os.path.join(dir, DEFAULT_WBROOT), "icons") + " not found.")

# checks write permission in the users home directory... well, that should
# always work, but sure is sure...
# also checks if we can actually read from the source directory
def check_permissions(dir):
    if (not os.access(os.path.expanduser("~"), os.W_OK)):
        raise InstallError("No write access to " + os.path.expanduser("~"))

    if (not os.access(dir, os.R_OK)):
        raise InstallERror("\nNo read access to source directory " + dir)
   
# create directory structure        
def create_dirs(DEFAULT_SERVER):
    try:
        os.makedirs(os.path.expanduser(DEFAULT_SERVER))
    except IOError, (errno, strerror):
        message =  "Could not create" + os.path.expanduser(DEFAULT_SERVER)  +\
                   " I/O error(%s): %s" % (errno, strerror)
        raise InstallError(message)
    except: 
        print "Create dirs failed"
        raise InstallError("\nUnexpected error when creating directory " +\
                           "structure.")

# create sqlite3 database
def create_database(name, DEFAULT_SERVER):
    try:
        f = open(os.path.join(os.path.expanduser(DEFAULT_SOURCE), DEFAULT_DBTABL), 'r')
        s = f.read()
        f.close()
    except IOError, (errno, strerror):
        message =  "Error opening: " +\
                    os.path.join(os.path.expanduser(DEFAULT_SOURCE), DEFAULT_DBTABL) +\
                    " I/O error(%s): %s" % (errno, strerror)
        raise InstallError(message)

    out = commands.getstatusoutput("echo \"" + s + "\" | sqlite3 " +\
          os.path.join(os.path.expanduser(DEFAULT_SERVER), name))

    if (out[0] != 0):
        raise InstallError("Could not create database:\n" +\
            os.path.join(os.path.expanduser(DEFAULT_SERVER), name) +\
            "\n" + out[1])

# finally prepare the config file 
def write_config(dir, DEFAULT_SERVER):
    try:
        f_in = open(os.path.join(DEFAULT_SOURCE, DEFAULT_DISCFG), 'r');
        cfg = f_in.read();
        f_in.close()

        if ((string.find(cfg, "__DEFAULT_HOME__") == -1) or\
            (string.find(cfg, "__DEFAULT_NAME__") == -1)):
            raise InstallError("Distribution config file corrupted, could not set serverhome or name.")

        cfg = string.replace(cfg, "__DEFAULT_HOME__", os.path.expanduser(DEFAULT_SERVER), 1)
        
        cfg = string.replace(cfg, "__DEFAULT_NAME__", DEFAULT_FRNAME +\
                             " (" + socket.gethostname() + ") / " + getpass.getuser(), 1)

        cfg = string.replace(cfg, "__DEFAULT_WEBROOT__", os.path.join(DEFAULT_SOURCE, DEFAULT_WBROOT), 1)

        cfg = string.replace(cfg, "__DEFAULT_SCRIPT__", os.path.join(DEFAULT_SOURCE, os.path.join(DEFAULT_IJSDIR, DEFAULT_ISCRPT)), 1)

        f_out = open(os.path.join(os.path.expanduser(DEFAULT_SERVER),\
                    DEFAULT_CONFIG), 'w')


        f_out.write(cfg)
        f_out.close()

    except IOError, (errno, strerror):
        message =  "I/O error(%s): %s when writing configuration file." % (errno, strerror)
        raise InstallError(message)

# this function sets up a ~/.mediatomb directory with all necessary
# components
# the source of the installation is the "dir" parameter
# the source directory must have a specific strucutre for this function to
# understand it
def install_from(dir, dest):
    check_source(dir)
    check_permissions(dir)
    print "Creating directories...",
    create_dirs(dest)
    print "ok"
    print "Creating database...",
    create_database(DEFAULT_DABASE, dest)
    print "ok"
    print "Writing configuration...",
    write_config(dir, dest)
    print "ok"
    print
    print "All done. You are now ready to launch MediaTomb!"
    print
    print "WARNING: Because of security reasons the UI is disabled by default!"
    print "Please refer to the README file on how to enable it."
    print 
    print "Once it is enabled:"
    print "When the server is running you can access the UI by opening"
    print dest + "/mediatomb.html in your web browser."
    print


# check command line arguments and start installation if appropriate
def main():
    print
    print "MediaTomb installer", TOMB_INSTALL_VERSION
    print
    
    src = DEFAULT_SOURCE
    db  = ""
    dest = DEFAULT_SERVER 

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hd:s:t:", ["help", "db-only=",\
                                                          "sourcedir=", "targetdir="])

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

        if o in ("-t", "--targetdir"):
            dest = a.lstrip()

    exitval = 0
    
    try:
        if (db == ""):
            print "Proceeding normally"
            is_clean(dest)
            install_from(src, dest)
        else:
            if (os.path.exists(os.path.join(os.path.expanduser(dest), db))):
                raise InstallError("\nWill not overwrite existing database! " +\
                        os.path.join(os.path.expanduser(dest), db))

            print "Creating database:", db, "...",
            create_database(db, dest)
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
