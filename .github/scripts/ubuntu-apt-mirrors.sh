#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
set -euo pipefail

. /etc/os-release
[[ "$ID" == ubuntu ]] || exit 0

shopt -s nullglob
sources=(/etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources)
if [[ -f /etc/apt/sources.list ]]; then
    sources=(/etc/apt/sources.list "${sources[@]}")
fi
(( ${#sources[@]} )) || exit 0

# Minimal Ubuntu images may need APT to install ca-certificates first.
scheme=http
if [[ -s /etc/ssl/certs/ca-certificates.crt ]]; then
    scheme=https
fi
mirror="$scheme://mirrors.mit.edu"

# The APT mirror transport tries the next URI when an index or package download fails.
for source in archive.ubuntu.com security.ubuntu.com ports.ubuntu.com; do
    archive=ubuntu
    if [[ "$source" == ports.ubuntu.com ]]; then
        archive=ubuntu-ports
    fi
    mirrorlist="/etc/apt/gerbera-$source.txt"
    printf '%s://%s/%s/\tpriority:1\n%s/%s/\n' "$scheme" "$source" "$archive" "$mirror" "$archive" > "$mirrorlist"
    uri="https?://${source//./[.]}/$archive/?"
    sed -Ei "/^[[:space:]]*(deb(-src)?[[:space:]]|URIs:)/ s@$uri([[:space:]]|$)@mirror+file:$mirrorlist\1@g" "${sources[@]}"
done

# GitHub's Ubuntu runners already use a local list for the main archive.
runner_mirrors=/etc/apt/apt-mirrors.txt
if [[ -f "$runner_mirrors" ]] && ! grep -Fxq "$mirror/ubuntu/" "$runner_mirrors"; then
    printf '\n%s/ubuntu/\n' "$mirror" >> "$runner_mirrors"
fi
