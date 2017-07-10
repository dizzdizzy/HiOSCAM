#!/bin/sh
# sky(0ld)
# export PATH=$PATH:/pj/His/android-14-toolchain/bin
# export USE_HISKY=1
# export CONF_DIR=/data/oscam
# export ANDROID_BUILD_TOP=/pj/His/HiSTBAndroidV400R001C00SPC050/software/HiSTBAndroidV400R001C00SPC050B012
# export ANDROID_DRV_PATH=$ANDROID_BUILD_TOP/device/hisilicon/godbox/driver/sdk/msp_base
# make android-arm

# set standalone toolchain path
# ./make-standalone-toolchain.sh --ndk-dir=../../ --install-dir=`pwd`/../../../android-toolchain/android-14 --platform=android-14

ANDROID_TOOLCHAIN_PATH=/pj/His/android-14-toolchain/bin
export USE_HISKY=1
export HISDK_BUILD_TOP=/pj/His/HiSTBAndroidV400R001C00SPC050/software/HiSTBAndroidV400R001C00SPC050B012
export HISDK_DRV_PATH=$ANDROID_BUILD_TOP/device/hisilicon/godbox/driver/sdk/msp_base
export HISDK_INC_PATH=$HISDK_BUILD_TOP/device/hisilicon/godbox/driver/sdk/msp_base
export HISDK_LIB_PATH=$HISDK_BUILD_TOP/out/target/product/godbox/system/lib

PATH=$PATH:$ANDROID_TOOLCHAIN_PATH

# if hasn't trunk
# svn checkout http://www.streamboard.tv/svn/oscam/trunk oscam
# for (-lhi_ecs)hi_sci (-lhi_mpi)hi_demux (-lhi_common)sys
make android-arm-hisky -j16 \
		 HISKY_LIB="-L$HISDK_LIB_PATH -lhi_mpi -lhi_common" \
		 HISKY_FLAGS="-D__ARMEL__ -I$HISDK_INC_PATH/common/include -I$HISDK_INC_PATH/ecs/include -I$HISDK_INC_PATH/mpi/include -I$HISDK_INC_PATH/ha_codec/include -DWITH_HISILICON=1" \


