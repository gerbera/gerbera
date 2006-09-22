#!/bin/sh

hexdump -ve  '"+" 20/1 "~x%02x" "+\n"' |sed 's/~/\\/g' | sed 's/+/"/g'
