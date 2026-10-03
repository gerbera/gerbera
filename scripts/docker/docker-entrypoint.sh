#!/usr/bin/env sh
# Gerbera - https://gerbera.io/
#
# docker-entrypoint.sh - this file is part of Gerbera.
#
# Copyright (C) 2021-2026 Gerbera Contributors
#
# Gerbera is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# Gerbera is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# NU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
#
# $Id$
#
# Maps the image user to the PUID/PGID passed in the environment, makes sure
# everything under the gerbera home is owned by PUID:PGID and then runs
# gerbera as that user.
#
# The Dockerfiles replace the IMAGE_* placeholders below with the build args.

GRB_USER=$IMAGE_USER
GRB_GROUP=$IMAGE_GROUP
GRB_USER=${GRB_USER:-gerbera}
GRB_GROUP=${GRB_GROUP:-gerbera}
GRB_HOME=/var/run/gerbera
GRB_CONFIG=$GRB_HOME/config.xml

log() {
  echo "docker-entrypoint: $*"
}

# Read a variable from the process environment. bash defines UID itself as a
# read-only shell variable (always the current uid) and ignores the
# value passed with "docker run -e UID=...", so $UID cannot be used directly.
env_value() {
  printenv "$1" 2>/dev/null || env | sed -n "s/^$1=//p"
}

is_number() {
  case "$1" in
    '' | *[!0-9]*) return 1 ;;
  esac
  return 0
}

# Name of the user/group that owns an id, empty if there is none
user_with_uid() {
  awk -F: -v id="$1" '$3 == id { print $1; exit }' /etc/passwd
}
group_with_gid() {
  awk -F: -v id="$1" '$3 == id { print $1; exit }' /etc/group
}

# Run a command as root: directly when we are root, else through sudo when it
# is allowed without a password (the image user has NOPASSWD sudo).
as_root() {
  if [ "$(id -u)" -eq 0 ]; then
    "$@"
  elif command -v sudo > /dev/null 2>&1 && sudo -n true > /dev/null 2>&1; then
    sudo -n "$@"
  else
    "$@"
  fi
}

CAN_ROOT=no
if [ "$(id -u)" -eq 0 ] || (command -v sudo > /dev/null 2>&1 && sudo -n true > /dev/null 2>&1); then
  CAN_ROOT=yes
fi

# ------------------------------------------------------------------
# Resolve the requested ids
# ------------------------------------------------------------------
# CUR_UID/CUR_GID: ids the image user has now (the IMAGE_UID/IMAGE_GID build
#                  args, or whatever an earlier start of this container set)
# RUN_UID/RUN_GID: ids gerbera will run as (PUID/PGID, default CUR_UID/CUR_GID)
CUR_UID=$(id -u "$GRB_USER")
CUR_GID=$(id -g "$GRB_USER")

# PUID/PGID select the ids. UID/GID are still read for containers set up
# before the rename, but are deprecated: bash, docker and many host shells
# give UID a meaning of their own.
requested_id() { # <new name> <legacy name>
  value=$(env_value "$1")
  legacy=$(env_value "$2")
  if [ -n "$legacy" ]; then
    if [ -z "$value" ]; then
      log "warning: $2 is deprecated, use $1=$legacy instead" >&2
      value=$legacy
    elif [ "$legacy" != "$value" ]; then
      log "warning: ignoring deprecated $2=$legacy, using $1=$value" >&2
    fi
  fi
  echo "$value"
}

RUN_UID=$(requested_id PUID UID)
RUN_GID=$(requested_id PGID GID)
RUN_UID=${RUN_UID:-$CUR_UID}
RUN_GID=${RUN_GID:-$CUR_GID}

if ! is_number "$RUN_UID" || [ "$RUN_UID" -eq 0 ]; then
  log "ignoring invalid PUID '$RUN_UID', using $CUR_UID"
  RUN_UID=$CUR_UID
fi
if ! is_number "$RUN_GID" || [ "$RUN_GID" -eq 0 ]; then
  log "ignoring invalid PGID '$RUN_GID', using $CUR_GID"
  RUN_GID=$CUR_GID
fi

if [ "$(id -u)" -ne 0 ] && [ "$CAN_ROOT" = no ]; then
  # Started with "docker run --user ...": ids cannot be changed, use the ones we have
  if [ "$RUN_UID" != "$CUR_UID" ] || [ "$RUN_GID" != "$CUR_GID" ]; then
    log "not running as root, cannot apply PUID=$RUN_UID PGID=$RUN_GID; running as $(id -u):$(id -g)"
  fi
  RUN_UID=$(id -u)
  RUN_GID=$(id -g)
fi

# ------------------------------------------------------------------
# Remap the image user and group
# ------------------------------------------------------------------
if [ "$CAN_ROOT" = yes ]; then
  if [ "$RUN_GID" != "$CUR_GID" ]; then
    existing=$(group_with_gid "$RUN_GID")
    if [ -z "$existing" ]; then
      log "changing gid of group $GRB_GROUP to $RUN_GID"
      as_root groupmod -g "$RUN_GID" "$GRB_GROUP"
      # if the user's primary group was not GRB_GROUP, point it at the new gid
      [ "$(id -g "$GRB_USER")" = "$RUN_GID" ] || as_root usermod -g "$RUN_GID" "$GRB_USER"
    else
      # gid is taken (e.g. 100 "users"): make that group the primary group
      log "gid $RUN_GID belongs to group '$existing', using it as primary group of $GRB_USER"
      as_root usermod -g "$RUN_GID" "$GRB_USER"
    fi
  fi

  if [ "$RUN_UID" != "$CUR_UID" ]; then
    existing=$(user_with_uid "$RUN_UID")
    if [ -n "$existing" ]; then
      log "uid $RUN_UID already belongs to '$existing', sharing it with $GRB_USER"
    else
      log "changing uid of user $GRB_USER to $RUN_UID"
    fi
    as_root usermod -o -u "$RUN_UID" "$GRB_USER"
  fi

  if [ "$(id -u "$GRB_USER")" != "$RUN_UID" ] || [ "$(id -g "$GRB_USER")" != "$RUN_GID" ]; then
    log "ERROR: failed to map $GRB_USER to $RUN_UID:$RUN_GID (now $(id -u "$GRB_USER"):$(id -g "$GRB_USER"))"
    exit 1
  fi
fi

# ------------------------------------------------------------------
# Home directory and default configuration
# ------------------------------------------------------------------
as_root mkdir -p "$GRB_HOME/.config/gerbera"

CONFIG_PROVIDED=yes
if [ ! -f "$GRB_CONFIG" ]; then
  CONFIG_PROVIDED=no
  as_root chown "$RUN_UID:$RUN_GID" "$GRB_HOME" 2> /dev/null

  # Generate a config file with home set
  gerbera --create-config --home "$GRB_HOME" --scripts /mnt/customization/js --modules=Autoscan | as_root tee "$GRB_CONFIG" > /dev/null

  # Automatically scan /content with inotify (for a volume mount)
  echo \
'<autoscan use-inotify="yes">
  <directory location="/mnt/content" mode="inotify"
             recursive="yes" hidden-files="no"/>
</autoscan>' | as_root tee "$GRB_HOME/autoscan.xml" > /dev/null

  # Allow customization of Gerbera configuration file
  if [ -x /mnt/customization/shell/gerbera_config.sh ]; then
    # shellcheck source=/dev/null
    . /mnt/customization/shell/gerbera_config.sh
  fi
fi

# Give the device nodes needed for hardware transcoding to the video group
for dev in /dev/video10 /dev/video11 /dev/video12 /dev/dri; do
  if [ -e "$dev" ]; then
    as_root chown root:video "$dev"
  fi
done

# Everything gerbera writes lives in its home: hand it to the runtime user.
# A config.xml supplied by the user keeps its owner (it may be shared with the
# host) but must be readable by the group. One generated by an earlier start
# has the same owner as the home directory and simply follows the new ids.
if [ "$CAN_ROOT" = yes ]; then
  CONFIG_OWNER=
  if [ "$CONFIG_PROVIDED" = yes ]; then
    CONFIG_OWNER=$(as_root stat -c "%u" "$GRB_CONFIG")
    if [ "$CONFIG_OWNER" = "$(as_root stat -c "%u" "$GRB_HOME")" ]; then
      CONFIG_OWNER=
    fi
  fi
  as_root chown -R "$RUN_UID:$RUN_GID" "$GRB_HOME" 2> /dev/null ||
    log "warning: could not change ownership of everything in $GRB_HOME"
  if [ -n "$CONFIG_OWNER" ]; then
    as_root chown "$CONFIG_OWNER" "$GRB_CONFIG" 2> /dev/null
    as_root chmod g+r "$GRB_CONFIG" 2> /dev/null
  fi
fi

# ------------------------------------------------------------------
# Start
# ------------------------------------------------------------------
if [ "$1" = "--" ]; then
  shift
fi

# gerbera itself runs unprivileged; other commands (e.g. a debug shell) run as given
RUN_GERBERA=no
case "$1" in
  gerbera | */gerbera) RUN_GERBERA=yes ;;
  sh | bash | */sh | */bash)
    if [ "$2" = "-c" ]; then
      case "$3" in
        gerbera | "gerbera "* | */gerbera | */"gerbera "*) RUN_GERBERA=yes ;;
      esac
    fi
    ;;
esac

if [ "$RUN_GERBERA" = yes ] && [ "$(id -u)" -eq 0 ]; then
  log "running as user $GRB_USER ($RUN_UID:$RUN_GID) instead of root"
  export HOME="$GRB_HOME"
  if command -v su-exec > /dev/null 2>&1; then
    exec su-exec "$GRB_USER" "$@"
  elif command -v setpriv > /dev/null 2>&1; then
    # explicit group list: --init-groups would look up the user by uid, which
    # is ambiguous when the uid is shared with another user
    exec setpriv --reuid="$RUN_UID" --regid="$RUN_GID" \
      --groups="$(id -G "$GRB_USER" | tr ' ' ',')" "$@"
  else
    log "ERROR: neither su-exec nor setpriv is available to drop root privileges"
    exit 1
  fi
fi

exec "$@"
