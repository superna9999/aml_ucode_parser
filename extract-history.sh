#!/bin/bash

set -ex

GITDIR=`readlink -f $1`
DIFF=$2
TOPDIR=$PWD

[ -d firmwares ] || mkdir firmwares
[ -d tmp-extract ] || mkdir tmp-extract

while read p; do
	SHA=$(echo $p | cut -d" " -f 1)
	LOG=$(echo $p | cut -d" " -f 2-)
	echo "SHA=$SHA LOG='$LOG'"
	git -C $GITDIR checkout $SHA --force
	git -C $GITDIR show $SHA --pretty=medium >  $TOPDIR/firmwares/changes.log
	(cd $TOPDIR/tmp-extract && $TOPDIR/builddir/aml_ucode_parser $GITDIR/firmware/video_ucode.bin | tee $TOPDIR/firmwares/extract.log)
	cp $TOPDIR/tmp-extract/*/*.bin firmwares
	git add firmwares/*
	git commit firmwares -m "Firmware from $SHA Change $LOG" --allow-empty
	rm -fr  $TOPDIR/tmp-extract/*
done < $DIFF

exit 0
