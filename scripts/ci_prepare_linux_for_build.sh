#!/bin/bash

set -ex

export CUR_LOC="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

pip install -r requirements/build.txt

if command -v yum &> /dev/null
then
    yum install -y \
        pkgconfig \
        eigen3-devel \
        tbb
else
    apt-get install -y \
        pkg-config \
        libeigen3-dev \
        libtbb-dev
fi
