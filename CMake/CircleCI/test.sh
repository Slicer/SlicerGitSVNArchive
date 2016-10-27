#!/bin/bash

# This is a script to build the modules and run the test suite in the base
# Docker container.

die() {
  echo "Error: $@" 1>&2
  exit 1;
}

mkdir /usr/src/Slicer-build
cd /usr/src/Slicer-build || die "Could not cd into the build directory"

mkdir /usr/src/Slicer-build/Slicer-build
cd /usr/src/Slicer-build/Slicer-build || die "Could not cd into the build directory"

ctest \
  -S /usr/src/CircleCI_Slicer_Docker.cmake \
  -VV || die "ctest failed"
