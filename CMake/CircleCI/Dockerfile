FROM thewtex/opengl:centos
MAINTAINER 3D Slicer Community <slicer-devel@bwh.harvard.edu>
# Test Slicer
# Graphic rendering support is required, so we use a base image that support
# rendering.

# Build the same version of CMake that is in thewtex/centos-build
WORKDIR /usr/src
RUN git clone git://cmake.org/cmake.git CMake && \
  cd CMake && \
  git checkout v3.4.1 && \
  mkdir /usr/src/CMake-build && \
  cd /usr/src/CMake-build && \
  /usr/src/CMake/bootstrap \
    --parallel=$(grep -c processor /proc/cpuinfo) \
    --prefix=/usr && \
  make -j$(grep -c processor /proc/cpuinfo) && \
  ./bin/cmake \
    -DCMAKE_BUILD_TYPE:STRING=Release \
    -DCMAKE_USE_OPENSSL:BOOL=ON . && \
  make install && \
  cd .. && rm -rf CMake*

RUN yum update -y && \
  yum install -y glx-utils \
  subversion \
  build-essential \
  qt4-dev-tools \
  x11-xserver-utils \
  x11vnc \
  xinit \
  libxt-dev \
  ninja \
  libboost-all-dev \
  wget \
  vim \
  valgrind \
  python2.7-dev

COPY test.sh /usr/src/test.sh
COPY CircleCI_Slicer_Docker.cmake /usr/src/CircleCI_Slicer_Docker.cmake

# Authorize to write in Slicer-build to recover logs of ctest
ENV APP "sudo chown -R user.user /usr/src/Slicer-build && /usr/src/test.sh"
