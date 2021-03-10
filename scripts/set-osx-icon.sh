#!/bin/sh
icon=$1
dest=$2
temp=$(mktemp /tmp/icon.XXXXXXXXX)
rsrc=$(mktemp /tmp/rsrc.XXXXXXXXX)
cp $icon $temp
sips -i $icon &> /dev/null
DeRez -only icns $icon > $rsrc
SetFile -a C $dest
Rez -append $rsrc -o $dest
