#!/bin/bash -e
# Install some dependencies manually
DEPS_DIR="${HOME}/deps"
mkdir -p ${DEPS_DIR}
cd ${DEPS_DIR}

INSTALL_DIR="${HOME}/install"

# Recent CMake:
CMAKE_URL="https://cmake.org/files/v3.11/cmake-3.11.4-Linux-x86_64.tar.gz"
mkdir cmake && wget --no-check-certificate --quiet -O - ${CMAKE_URL} | tar --strip-components=1 -xz -C cmake
export PATH=${DEPS_DIR}/cmake/bin:${PATH}
cmake --version

############################################################################
# Install libc++ and libc++abi if needed
# Taken from here: https://github.com/boostorg/hana/blob/master/.travis.yml
############################################################################
if [[ "${CXX%%+*}" == "clang" ]]; then
  if [ -d ${TRAVIS_BUILD_DIR}/llvm ]; then
    echo "libc++ already exists";
  else
    if   [[ "${CXX}" == "clang++-3.5" ]]; then LLVM_VERSION="3.5.2";
    elif [[ "${CXX}" == "clang++-3.6" ]]; then LLVM_VERSION="3.6.2";
    elif [[ "${CXX}" == "clang++-3.7" ]]; then LLVM_VERSION="3.7.1";
    elif [[ "${CXX}" == "clang++-3.8" ]]; then LLVM_VERSION="3.8.1";
    elif [[ "${CXX}" == "clang++-3.9" ]]; then LLVM_VERSION="3.9.1";
    elif [[ "${CXX}" == "clang++-4.0" ]]; then LLVM_VERSION="4.0.0";
    elif [[ "${CXX}" == "clang++-5.0" ]]; then LLVM_VERSION="5.0.0";
    fi
    LLVM_URL="http://llvm.org/releases/${LLVM_VERSION}/llvm-${LLVM_VERSION}.src.tar.xz"
    LIBCXX_URL="http://llvm.org/releases/${LLVM_VERSION}/libcxx-${LLVM_VERSION}.src.tar.xz"
    LIBCXXABI_URL="http://llvm.org/releases/${LLVM_VERSION}/libcxxabi-${LLVM_VERSION}.src.tar.xz"
    mkdir -p llvm llvm/build llvm/projects/libcxx llvm/projects/libcxxabi
    wget -O - ${LLVM_URL} | tar --strip-components=1 -xJ -C llvm
    wget -O - ${LIBCXX_URL} | tar --strip-components=1 -xJ -C llvm/projects/libcxx
    wget -O - ${LIBCXXABI_URL} | tar --strip-components=1 -xJ -C llvm/projects/libcxxabi
    (cd llvm/build && cmake .. -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR})
    (cd llvm/build/projects/libcxx && make install -j2)
    (cd llvm/build/projects/libcxxabi && make install -j2)
  fi

  export STD_CXX_FLAGS="-isystem ${INSTALL_DIR}/include/c++/v1 -stdlib=libc++"
  export STD_LINKER_FLAGS="-L ${INSTALL_DIR}/lib -l c++ -l c++abi"
  export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${INSTALL_DIR}/lib"
fi

# Function for creating a new 'build' directory
function renew_build {
  echo "Renew build directory..."
  cd $TRAVIS_BUILD_DIR

  # Remove any existing build directory
  [ -e build ] && rm -r -f build
  mkdir build
  cd build

  # Configure the project and build it
  cmake -GNinja -DCMAKE_CXX_FLAGS="$STD_CXX_FLAGS $CMAKE_CXX_FLAGS -Werror" -DCMAKE_EXE_LINKER_FLAGS="$STD_LINKER_FLAGS" \
        -DCTI_CONTINUABLE_WITH_NO_EXCEPTIONS=$WITH_NO_EXCEPTIONS -DCTI_CONTINUABLE_WITH_EXPERIMENTAL_COROUTINE=$WITH_AWAIT -DCTI_CONTINUABLE_WITH_LIGHT_TESTS=$WITH_LIGHT_TESTS \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=$BUILD_CONFIG ..
}

if [[ $CXX == *"clang"* ]]; then
  if [ $WITH_AWAIT != "ON" ]; then
    # Abort when the sanitizers detect an error
    LSAN_OPTIONS=verbosity=1:log_threads=1:abort_on_error=1
    ASAN_OPTIONS=verbosity=1:log_threads=1:abort_on_error=1
    UBSAN_OPTIONS=print_stacktrace=1:symbolize=1:halt_on_error=1:print_summary=1
  else
    # Continue when the sanitizers detect an error (when using await)
    LSAN_OPTIONS=verbosity=1:log_threads=1
    ASAN_OPTIONS=verbosity=1:log_threads=1
    UBSAN_OPTIONS=print_stacktrace=1:symbolize=1:print_summary=1
  fi

  # Build the test suite with various sanitizers:
  if [ $WITH_AWAIT != "ON" ]; then
    # - ASan (LSan):
    echo "Building with address sanitizer..."
    CMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
    renew_build

    ninja -j2
    ctest --verbose
  else
    echo "Skipping ASan testing because we build with coroutine support...";
  fi

  # - UBSan:
  echo "Building with undefined behaviour sanitizer..."
  CMAKE_CXX_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer"
  renew_build

  ninja -j2
  ctest --verbose
else
  # Build an run the tests suite with valgrind
  renew_build

  ninja -j2
  valgrind --error-exitcode=1 --leak-check=full --show-reachable=yes ctest --verbose
fi
