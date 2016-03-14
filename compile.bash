#!/usr/bin/env bash

set -x

rm -f -- main

gcc -Wall -ansi -pedantic -o datapipe main.c

exit 0
