#!/bin/sh

die() {
  echo "Error: $@" 1>&2
  exit 1;
}

docker push slicer/slicer-test:opengl
