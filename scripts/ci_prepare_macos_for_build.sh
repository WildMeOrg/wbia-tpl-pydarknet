#!/bin/bash

set -ex

# Install Python package dependencies
python -m pip install -r requirements/build.txt

sudo port selfupdate

# Install ports if MacPorts install location is not present
sudo port install \
    pkgconfig \
    tbb
