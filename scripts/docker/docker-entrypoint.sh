#!/usr/bin/env sh

if [ ! -f /var/run/gerbera/config.xml ]; then
  # Generate a config file with home set
  gerbera --create-config --home /var/run/gerbera > /var/run/gerbera/config.xml

  # Automatically scan /content with inotify (for a volume mount)
  sed 's/<import hidden-files="no">/<import hidden-files="no">\n\
    <autoscan use-inotify="yes">\n\
    <directory location="\/content" mode="inotify" \
    recursive="yes" hidden-files="no"\/>\n\
    <\/autoscan>/' -i /var/run/gerbera/config.xml
fi

# If we are root, chown home and drop privs
if [ "$1" = 'gerbera' -a "$(id -u)" = '0' ]; then
    chown -R gerbera /var/run/gerbera
    exec su-exec gerbera "$@"
else
    # Otherwise run as the provided user
    exec "$@"
fi