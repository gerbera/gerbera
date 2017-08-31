<img src="https://github.com/v00d00/gerbera/blob/master/artwork/logo-horiz.png?raw=true" />

# Building Gerbera on FreeBSD

_The following has bee tested on FreeBSD 11.0 using a clean jail environment._ 

1. Install the required prerequisites as root using either ports or packages. This can be done via Package manager using "pkg install wget git autoconf automake libtool taglib cmake gcc libav ffmpeg libexif pkgconf liblastfm gmake".

2. If mysql is to be used install mysql as well. (If MySql is not installed SqlLite3 will be used.)

3. git clone https://github.com/v00d00/gerbera.git 

4. mkdir temp

5. cd temp

6. sh ../gerbera/scripts/install-pupnp18.sh

7. sh ../gerbera/scripts/install-duktape.sh

8. cmake ../gerbera -DWITH_MAGIC=1 -DWITH_MYSQL=0 -DWITH_CURL=1 -DWITH_JS=1 -DWITH_TAGLIB=1 -DWITH_AVCODEC=1 -DWITH_EXIF=1 -DWITH_LASTFM=0

9. make -j4

10. sudo make install

