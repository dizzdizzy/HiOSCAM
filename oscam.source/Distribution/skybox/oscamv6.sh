#!/system/bin/sh
OSCAM_PATH=/system/oscam
OSCAM_DATAPATH=/data/oscam

if [ -f /var/.debug ]; then
	exit 1
else
	if [ -d $OSCAM_DATAPATH ]; then
	    echo "start oscam"
	    cd $OSCAM_PATH
	    ./oscam-1.20 > /dev/null
	else
        mkdir $OSCAM_DATAPATH
        cp $OSCAM_PATH/oscam.* $OSCAM_DATAPATH
        cp $OSCAM_PATH/constant.cw  $OSCAM_DATAPATH
        chmod 666 $OSCAM_DATAPATH/oscam.*
        exit 1
	fi
fi

