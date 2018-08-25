#!/bin/sh
grep  -E -R -- "$1" src/ |grep -v '\.svn'  | grep -v '~'
