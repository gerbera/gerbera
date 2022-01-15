#!/usr/bin/env bash

set -xe

sed -i "s#GERBERA_VERSION [[:digit:]].[[:digit:]].[[:digit:]]_git#GERBERA_VERSION $1#" CMakeLists.txt
sed -i "s#version = u'[[:digit:]].[[:digit:]].[[:digit:]]'#version = u'$1'#" doc/conf.py
sed -i "s#release = u'[[:digit:]].[[:digit:]].[[:digit:]]_git'#release = u'$1'#" doc/conf.py

git add CMakeLists.txt doc/conf.py
git commit -m "Gerbera $1"
git tag v$1

sed -i "s#GERBERA_VERSION $1#GERBERA_VERSION $2_git#" CMakeLists.txt
sed -i "s#version = u'$1'#version = u'$2'#" doc/conf.py
sed -i "s#release = u'$1'#release = u'$2_git'#" doc/conf.py

git add CMakeLists.txt doc/conf.py
git commit -m "Bump master"
