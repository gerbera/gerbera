#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-only
# Select a reachable package mirror before setup-alpine bootstraps its rootfs.
set -euo pipefail

branch=${1:?Alpine branch required}
arch=${2:?Alpine architecture required}
primary=${3:-https://dl-cdn.alpinelinux.org/alpine}
fallback=${4:-https://mirrors.edge.kernel.org/alpine}
[[ $branch =~ ^(v[0-9]+\.[0-9]+|edge)$ ]] || exit 2
case "$arch" in
  x86_64|x86|armhf|armv7|aarch64|loongarch64|ppc64le|riscv64|s390x) ;;
  *) exit 2 ;;
esac
for mirror in "$primary" "$fallback"; do
  [[ $mirror =~ ^https://[A-Za-z0-9.:-]+(/[A-Za-z0-9._-]+)*$ ]] || exit 2
done

temp=$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/alpine-mirror.XXXXXX")
trap 'rm -rf -- "$temp"' EXIT
for mirror in "$primary" "$fallback"; do
  available=true
  for repository in main community; do
    url="$mirror/$branch/$repository/$arch/APKINDEX.tar.gz"
    printf 'Checking Alpine package index: %s\n' "$url" >&2
    if ! curl --ipv4 --proto '=https' --proto-redir '=https' \
      --fail --silent --show-error --location --connect-timeout 10 --max-time 30 \
      --retry 2 --retry-delay 5 --retry-max-time 100 --retry-connrefused \
      --output "$temp/index.tar.gz" "$url" \
      || ! tar -xzOf "$temp/index.tar.gz" APKINDEX > "$temp/APKINDEX" \
      || ! grep -q '^P:' "$temp/APKINDEX"; then
      available=false
      break
    fi
  done
  if "$available"; then
    printf 'mirror-url=%s\n' "$mirror"
    # Keep a second source for failures after the probe. apk still verifies
    # repository/package signatures using setup-alpine's existing trusted keys.
    if [[ $mirror == "$primary" ]]; then
      printf 'extra-repositories<<ALPINE_REPOSITORIES\n%s/%s/main\n%s/%s/community\nALPINE_REPOSITORIES\n' \
        "$fallback" "$branch" "$fallback" "$branch"
    else
      printf 'extra-repositories=\n'
      printf '::warning::Using the fallback Alpine package mirror: %s\n' "$mirror" >&2
    fi
    exit 0
  fi
  printf 'Alpine package mirror unavailable: %s\n' "$mirror" >&2
done
echo '::error::Neither Alpine package mirror supplied usable main/community indexes.' >&2
exit 1
