#!/bin/bash
#################################
echo 'Removing old build'
# ./unix_build.sh --no-rmbuild
# ./unix_build.sh --no-rmbuild
#rm -rf build
#rm -rf CMakeFiles
#rm -rf CMakeCache.txt
#rm -rf cmake_install.cmake

python2.7 -c "import utool as ut; print('keeping build dir' if ut.get_argflag(('--no-rmbuild', '--norm')) else ut.delete('build'))" $@
#################################
echo 'Creating new build'
mkdir -p build
cd build
#################################
if [[ "$(which nvcc)" == "" ]]; then
    export CMAKE_CUDA=Off
else
    export CMAKE_CUDA=On
fi

export PYEXE=$(which python)
if [[ "$VIRTUAL_ENV" == ""  ]] && [[ "$CONDA_PREFIX" == ""  ]] ; then
    export LOCAL_PREFIX=/usr/local
    export _SUDO="sudo"
else
    if [[ "$CONDA_PREFIX" == ""  ]] ; then
        export LOCAL_PREFIX=$($PYEXE -c "import sys; print(sys.prefix)")/local
    else
        export LOCAL_PREFIX=$($PYEXE -c "import sys; print(sys.prefix)")
    fi
    export _SUDO=""
fi

echo 'Configuring with cmake'
if [[ '$OSTYPE' == 'darwin'* ]]; then
    export CONFIG="-DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_INSTALL_PREFIX=$LOCAL_PREFIX -DOpenCV_DIR=$LOCAL_PREFIX/share/OpenCV"
else
    export CONFIG="-DCMAKE_BUILD_TYPE='Release' -DCMAKE_INSTALL_PREFIX=$LOCAL_PREFIX -DOpenCV_DIR=$LOCAL_PREFIX/share/OpenCV"
fi
echo "$CONFIG"

# Make official (GPU or CPU) version
cmake $CONFIG -DCUDA=$CMAKE_CUDA -G 'Unix Makefiles' ..
echo 'Building with make'
export NCPUS=$(grep -c ^processor /proc/cpuinfo)
make -j$NCPUS -w

echo 'Moving the shared library'
cp -v lib* ../pydarknet

# Make CUDA is enabled, also make CPU version (this may be a duplicated build)
if [[ $CMAKE_CUDA == On ]]; then
    echo "BUILDING CPU ONLY LIBRARY"
    cd ..
    rm -rf build
    mkdir -p build
    cd build
    cmake $CONFIG -DCUDA=Off -G 'Unix Makefiles' ..
    echo 'Building with make'
    export NCPUS=$(grep -c ^processor /proc/cpuinfo)
    make -j$NCPUS -w

    echo 'Moving the shared library'
    cp -v lib* ../pydarknet
fi

cd ..
