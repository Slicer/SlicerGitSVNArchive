#!/bin/sh

script_dir="`cd $(dirname $0); pwd`"

# Run the images which contains the build of slicer (to link with the testing image with mounted volume)
docker run -d --name slicer-build-with-test slicer/slicer-test bash

# Run the opengl docker image which will run the tests located on the volume shared by slicer/slicer-build
$script_dir/run_opengl.sh \
    -i slicer/slicer-test:opengl \
    -p 6081 \
    -r --env="CIRCLE_SHA1=$1" -r --env="CIRCLE_BRANCH=$2" -r --env="SITE_BUILD_TYPE=$3" \
    -r --volumes-from -r slicer-build-with-test

# Remove the container used to mount volumes from slicer-build
docker rm slicer-build-with-test
