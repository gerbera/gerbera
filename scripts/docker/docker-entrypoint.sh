#!/usr/bin/env bash
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
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Gerbera.  If not, see <http://www.gnu.org/licenses/>.
#
# $Id$

# _UID/_GID are the runtime source of truth
_UID=${UID:-$IMAGE_UID}
_GID=${GID:-$IMAGE_GID}

# Detect the privilege-drop helper available on this image.
# Alpine ships su-exec; openSUSE ships neither su-exec nor gosu by default,
# so we fall back to the run.sh/su approach in that case.
_SUEXEC=""
if command -v su-exec > /dev/null 2>&1; then
  _SUEXEC="su-exec"
elif command -v gosu > /dev/null 2>&1; then
  _SUEXEC="gosu"
fi

# Resolve a no-login shell that actually exists on this image.
_NOLOGIN=/bin/false
for _nl in /usr/sbin/nologin /sbin/nologin /bin/nologin; do
  if [ -x "$_nl" ]; then
    _NOLOGIN="$_nl"
    break
  fi
done
unset _nl

# --- UID remapping ----------------------------------------------------------
if [ "${_UID}" -ne "$IMAGE_UID" ] && [ "${_UID}" -gt 0 ]; then
  sudo usermod -u "${_UID}" "$IMAGE_USER"
fi

# --- GID remapping ----------------------------------------------------------
if [ "${_GID}" -ne "$IMAGE_GID" ] && [ "${_GID}" -gt 0 ]; then
  # groupmod needs a group name, not a GID number.
  _GROUP_NAME=$(getent group "${_GID}" | cut -d: -f1)
  sudo groupmod -g "${_GID}" "$IMAGE_GROUP" \
    || sudo usermod -a -G "${_GROUP_NAME:-$IMAGE_GROUP}" "$IMAGE_USER"
fi



# --- Config directory setup -------------------------------------------------
# Create the run directory when it is missing; _UID/_GID own it.
if [ ! -d /var/run/gerbera/ ] && [ "$(id -u)" -ne '0' ]; then
  sudo mkdir -p /var/run/gerbera
fi


if [ ! -f /var/run/gerbera/config.xml ]; then
  # Generate a config file with home set
  gerbera --create-config --home /var/run/gerbera --scripts /mnt/customization/js --modules=Autoscan > /var/run/gerbera/config.xml
  # Automatically scan /content with inotify (for a volume mount)
  echo \
'<autoscan use-inotify="yes">
  <directory location="/mnt/content" mode="inotify"
             recursive="yes" hidden-files="no"/>
</autoscan>' > /var/run/gerbera/autoscan.xml
  # Allow customization of Gerbera configuration file
  if [ -x /mnt/customization/shell/gerbera_config.sh ]; then
    . /mnt/customization/shell/gerbera_config.sh
  fi
fi

# --- Device permissions -----------------------------------------------------
for dev in /dev/video10 /dev/video11 /dev/video12 /dev/dri; do
  if [ -e "$dev" ]; then
    sudo chown root:video "$dev"
  fi
done

# --- Ownership of directory's -----------------------------------------
# Restamp everything under /var/run/gerbera to the runtime _UID:_GID.
sudo chown -R "${_UID}:${_GID}" /var/run/gerbera
# Restamp everything under /mnt/customization to the runtime _UID:_GID.
sudo chown -R "${_UID}:${_GID}" /mnt/customization

# Ensure the group can read config.xml (e.g. for tooling running as the same group).
if [ -f /var/run/gerbera/config.xml ]; then
  sudo chmod g+r /var/run/gerbera/config.xml
fi

# --- Privilege drop ---------------------------------------------------------
# The Docker CMD is: /bin/sh -c "gerbera --port ..."
# When tini invokes this entrypoint: $1=/bin/sh  $2=-c  $3="gerbera --port ..."
# The regex must match $3 for the standard CMD path.
# The exact-match on $1 handles direct invocations: `docker run ... gerbera ...`
grb_pattern="^gerbera "
if [[ "$3" =~ $grb_pattern || "$1" == "gerbera" ]]; then
  if [ "$(id -u)" -eq '0' ]; then
    echo "Running as user $IMAGE_USER (uid=${_UID}) instead of root"
    if [ -n "$_SUEXEC" ] && ("$_SUEXEC" "$IMAGE_USER" /bin/true 2>&1) > /dev/null; then
      # Fast path: su-exec or gosu available and working.
      echo "$_SUEXEC $IMAGE_USER"
      exec "$_SUEXEC" "$IMAGE_USER" "$@"
    else
      # Slow path: minimal wrapper script and hand off via su.
      _run=/var/run/gerbera/run.sh
      {
        printf '#!/bin/sh\nexec'
        printf ' %q' "$@"
        printf '\n'
      } | sudo tee "$_run" > /dev/null
      sudo chown "${_UID}:${_GID}" "$_run"
      sudo chmod 700 "$_run"
      echo "su - $IMAGE_USER"
      exec su -c "$_run" - "$IMAGE_USER"
    fi
  fi
fi

# --- Otherwise run as the user provided -------------------------------------
if [ "$1" = "--" ]; then
  shift
fi
exec "$@"
