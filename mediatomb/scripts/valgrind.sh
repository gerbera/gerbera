#!/bin/sh
valgrind --tool=memcheck --show-reachable=yes --leak-check=full -v --trace-children=yes --run-libc-freeres=yes ./build/mediatomb
