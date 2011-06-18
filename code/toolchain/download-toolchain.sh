#!/bin/bash

# This was going to be a fancy build script
# Instead, just document the easy way

# This will install the toolchain in the directory it is run from
# as the user who runs it
# If run as root, be sure to copy the lines from root's .bash_profile
# to your own

TOOLCHAIN_URL="http://blackfin.uclinux.org/gf/download/frsrelease/344/6723/"
TOOLCHAIN_ARCHIVE="blackfin-toolchain-elf-SVN.x86_64.tar.bz2"

wget ${TOOLCHAIN_URL}/${TOOLCHAIN_ARCHIVE}

tar -xjf ${TOOLCHAIN_ARCHIVE}

CWD=`pwd`

echo >>~/.bash_profile <<zzzEOFzzz

### Blackfin Toolchain ###
PATH=${CWD}/bfin-elf/bin:${PATH}
####

zzzEOFzzz
