#!/bin/bash

# 2013 Matthieu Dubet
# Use this script to get llvm and boehm gc into vendor/

head=false
llvm=false
boehmgc=true
optimized=false

# Find number of CPUs
cpu=1
uname -s | grep -qs Linux && cpu=`cat /proc/cpuinfo | grep '^processor' | wc -l`
uname -s | grep -qs 'Darwin\|BSD\|DragonFly' && cpu=`sysctl -n hw.ncpu`
echo "You seem to have $cpu cpu cores"

#mkdir vendor
cd vendor
dir=`pwd`

#Compile LLVM
if $llvm ; then
  if $head ; then
    git clone git://github.com/chapuni/llvm.git || git pull || exit $?
    cd llvm
    git checkout release_32
  else
    wget -N http://llvm.org/releases/3.2/llvm-3.2.src.tar.gz || exit $?
    tar xzf llvm-3.2.src.tar.gz || exit $?
    cd llvm-3.2.src || exit $?
  fi
    if $optimized ; then
        ./configure --prefix=$dir \
            --enable-optimized \
            --enable-jit \
            --enable-targets=host \
            --enable-shared \
            --disable-docs \
            --disable-doxygen \
            --disable-assertions \
            --disable-timestamps \
            --enable-bindings=none  \
            || exit $?
    else
        ./configure --prefix=$dir \
            --disable-optimized \ 
            --enable-jit \
            --enable-targets=host \
            --enable-shared \
            --disable-docs \
            --disable-doxygen \
            --enable-assertions \
            --disable-timestamps \
            --enable-bindings=none || exit $?
    fi
    make install -j $cpu || exit $?
    cd .. || exit $?
fi


#Compile Boehm GC
if $boehmgc ; then
  if $head ; then
    git clone git://github.com/ivmai/libatomic_ops.git || git pull || exit $?
    git clone git://github.com/ivmai/bdwgc.git || git pull || exit $?
    ln -s $dir/libatomic_ops $dir/bdwgc/libatomic_ops || exit $?
    cd bdwgc
    autoreconf -vif|| exit $?
    automake --add-missing|| exit $?
  else
    version="gc-7.2"
    wget -N http://www.hpl.hp.com/personal/Hans_Boehm/gc/gc_source/$version.tar.gz  || exit $?
    tar xzf $version.tar.gz || exit $?
    cd $version
  fi
  ./configure --prefix=$dir --disable-debug --disable-dependency-tracking --enable-cplusplus || exit $?
  make || exit $?
  make install #the install can generate errors but still working fine ... 
  cd ..
fi

#Back to root
cd ..
