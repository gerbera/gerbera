#!/bin/sh
egrep -R -- $1 src/ |grep -v '\.svn'  | grep -v '~'
