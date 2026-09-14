#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

if ! mkdir -p ${OUTDIR}
then
    echo Failed to create directory
    exit 1
fi

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    echo "make clean executing"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper #clean
    echo "make defconfig executing"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig #defconfig
    echo "make vmlinux executing"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all #vmlinux
    #echo "make modules executing"
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    echo "make devicetree executing"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
    echo "Adding the Image in outdir"
    cp arch/${ARCH}/boot/Image "${OUTDIR}/"
    echo compile complete
fi


echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

if ! mkdir ${OUTDIR}/rootfs
then
    echo Failed to create rootfs dir
    exit 1
fi
cd ${OUTDIR}/rootfs
# TODO: Create necessary base directories
if ! mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
then
    echo Failed to create base directories
    exit 1
fi
if ! mkdir -p usr/bin usr/lib usr/sbin
then
    echo Failed to create base directories
    exit 1
fi
if ! mkdir -p var/log
then
    echo Failed to create base directories
    exit 1
fi    

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox

else
    cd busybox
fi

# TODO: Make and install busybox
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
cd ${OUTDIR}/rootfs
INTERPRETER=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter")
LIBS=$(${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library")
echo "libs print of ${LIBS}"
# TODO: Add library dependencies to rootfs
#used google gemini for the 2 lib dependencoes fucntions plase see README
INTERPRETER=$(${CROSS_COMPILE}readelf -l bin/busybox | grep "program interpreter" | sed -r 's/.*Requesting program interpreter: (.*)]/\1/')

if [ -n "$INTERPRETER" ]; then
    echo "Found interpreter: $INTERPRETER"
    
    # Extract only the filename from the path
    INT_NAME=$(basename "$INTERPRETER")
    
    # Search for it explicitly in lib64 subpaths inside sysroot
    INT_PATH=$(find "$SYSROOT/lib64" "$SYSROOT/usr/lib64" -name "$INT_NAME" -print -quit 2>/dev/null)
    
    if [ -n "$INT_PATH" ]; then
        echo "Copying interpreter from $INT_PATH to $OUTDIR/rootfs/lib64"
        cp -d "$INT_PATH" "$OUTDIR/rootfs/lib64"
    else
        echo "Warning: Interpreter $INT_NAME not found in sysroot lib64 paths."
    fi
fi
# Automatically find the sysroot folder
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
# Use a while loop instead of a for loop to avoid word splitting
${CROSS_COMPILE}readelf -d bin/busybox | grep "Shared library" | sed -r 's/.*\[(.*)\]/\1/' | while read -r LIB; do
    # Skip any unexpected empty lines
    [ -z "$LIB" ] && continue
    
    echo "Searching for: $LIB"
    
    # Search inside the toolchain sysroot lib64 paths
    LIB_PATH=$(find "$SYSROOT/lib64" "$SYSROOT/usr/lib64" -name "$LIB" -print -quit 2>/dev/null)
    
    if [ -n "$LIB_PATH" ]; then
        echo "Found! Copying $LIB_PATH to $OUTDIR/rootfs/lib64"
        cp -d "$LIB_PATH" "$OUTDIR/rootfs/lib64"
    else
        echo "ERROR: Could not find $LIB inside $SYSROOT lib64 directories"
    fi
done

# TODO: Make device nodes
echo making device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
# TODO: Clean and build the writer utility
cd /home/clint/Documents/AESD_work/assignment-1-0xBEE5-ClintFurrer/finder-app
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-
# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo copying my assignment files over
cp writer ${OUTDIR}/rootfs/home
cp finder-test.sh ${OUTDIR}/rootfs/home
mkdir ${OUTDIR}/rootfs/home/conf
cp conf/username.txt ${OUTDIR}/rootfs/home/conf
cp conf/assignment.txt ${OUTDIR}/rootfs/home/conf
cd ${OUTDIR}/rootfs/home
sed -i "35s#.*#assignment='cat conf/assignment.txt'#" finder-test.sh

# TODO: Chown the root directory
echo changing owner to root
cd ${OUTDIR}/rootfs
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
echo create initramfs system
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root.root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio
echo finished!!!