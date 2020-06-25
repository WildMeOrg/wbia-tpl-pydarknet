#!/bin/bash

#################################
#echo 'Removing old build'
#rm -rf build
#rm -rf CMakeFiles
#rm -rf CMakeCache.txt
#rm -rf cmake_install.cmake
python -c "import utool as ut; print('keeping build dir' if ut.get_argflag(('--fast', '--no-rmbuild')) else ut.delete('build'))" $@

#TODO: use default python 2.7 or 3.
# Whatever the default is in the virtualenv


#################################
echo 'Creating new build'
mkdir -p build
cd build
#################################


export PYEXE=$(which python)
export IN_VENV=$($PYEXE -c "import sys; print(hasattr(sys, 'real_prefix'))")
echo "IN_VENV = $IN_VENV"

if [[ "$IN_VENV" -eq "True" ]]; then
    export LOCAL_PREFIX=$($PYEXE -c "import sys; print(sys.prefix)")/local
else
    export LOCAL_PREFIX=/usr/local
fi

echo 'Configuring with cmake'
if [[ '$OSTYPE' == 'darwin'* ]]; then
    export CONFIG="-DCMAKE_OSX_ARCHITECTURES=x86_64 -DCMAKE_C_COMPILER=clang2 -DCMAKE_CXX_COMPILER=clang2++ -DCMAKE_INSTALL_PREFIX=$LOCAL_PREFIX"
else
    export CONFIG="-DCMAKE_BUILD_TYPE='Release' -DCMAKE_INSTALL_PREFIX=$LOCAL_PREFIX"
fi


# Use Virtual Env OpenCV if available
echo "LOCAL_PREFIX=$LOCAL_PREFIX"

if [[ "$IN_VENV" -eq "True" ]]; then
    export OpenCV_Dir="$LOCAL_PREFIX/share/OpenCV"
    if [ -d "$OpenCV_Dir" ]; then
        export CONFIG=" $CONFIG -DOpenCV_DIR='$OpenCV_Dir'"
    fi
fi

echo "CONFIG = $CONFIG"
cmake $CONFIG -G 'Unix Makefiles' \
    -DCMAKE_C_FLAGS="${CMAKE_C_FLAGS}" \
    -DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS}" \
    -DCMAKE_SHARED_LINKER_FLAGS="${CMAKE_SHARED_LINKER_FLAGS}" \
    -DCMAKE_EXE_LINKER_FLAGS="${CMAKE_EXE_LINKER_FLAGS}" \
    ..

#################################
echo 'Building with make'
export NCPUS=$(grep -c ^processor /proc/cpuinfo)
if [[ "$OSTYPE" == "msys"* ]]; then
    # Handle mingw on windows
    cmake $CONFIG -G 'MSYS Makefiles' ..
    make
else
    cmake $CONFIG -G 'Unix Makefiles' ..
    make -j$NCPUS -w
fi

#################################
if [[ '$OSTYPE' == 'darwin'* ]]; then
    echo 'Fixing OSX libiomp'
    # install_name_tool -change libiomp5.dylib ~/code/libomp_oss/exports/mac_32e/lib.thin/libiomp5.dylib lib*
    install_name_tool -change libiomp5.dylib /opt/local/lib/libomp/libiomp5.dylib lib*
fi
#################################

export MAKE_EXITCODE=$?
echo "MAKE_EXITCODE=$MAKE_EXITCODE"

if [[ $MAKE_EXITCODE == 0 ]]; then
    echo 'Moving the shared library'
    cp -v lib* ../pyrf
else
    FAILCMD='echo "FAILED PYRF BUILD" ; exit 1;'
    $FAILCMD
fi

cd ..
