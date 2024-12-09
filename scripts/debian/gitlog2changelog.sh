#!/bin/bash
#
# Convert git log into changelog in Debian format
#
# Tags in format 1.2.3-4 become version entries. Log entries between them
# become changelog entries. Merge commits are not excluded, so you probably
# have to clean up the result manually.

# based on
# https://gist.github.com/hfs/0ff1cf243b163bd551ec22604ce702a5

# More info under
# https://decovar.dev/blog/2021/09/23/cmake-cpack-package-deb-apt/
# https://cmake.org/cmake/help/latest/module/CPack.html
# https://www.debian.org/doc/debian-policy/ch-controlfields.html#debian-changes-files-changes
# https://www.debian.org/doc/debian-policy/ch-source.html#s-dpkgchangelog

RE_VERSION='^v\?[1-9]\+\([.-][0-9]\+\)*'
# The name of the package name is the first argument of the script
PACKAGE=${1-Gerbera}
distribution=$(lsb_release -c --short)
if [[ "${distribution}" == "n/a" ]]; then
  distribution="unstable"
fi
if [[ -z "${distribution}" ]]; then
  distribution="unstable"
fi

function logentry() {
        local tool=$1
        local previous=$2
        local version=$3
        local distribution=$4
        local level=${5-low}
        echo "$tool ($version) ${distribution}; urgency=${level}"
        echo
        git --no-pager log --format="  * %s" $previous${previous:+..}$version
        echo
        git --no-pager log --format=" -- %an <%ae>  %aD" -n 1 $version | sed "s/@/ at /g" | sed "s/\./ dot /g"
        echo
}

git tag --sort "-creatordate" | grep "$RE_VERSION" | (
        read VERSION
        logentry "${PACKAGE}" "$VERSION" "HEAD" "$distribution" "low"
        while read previous; do
                logentry "${PACKAGE}" "$previous" "$VERSION" "$distribution" "low"
                export VERSION="$previous"
        done
)

VERSION=$(git tag --sort "-creatordate" | grep "$RE_VERSION" | tail -n1)

git tag --sort "-creatordate" | grep -v "$RE_VERSION" | (
        while read previous; do
                logentry Mediatomb "$previous" "$VERSION" "$distribution" "low"
                VERSION="$previous"
        done
        logentry Mediatomb "" "$VERSION" "$distribution" "low"
)
