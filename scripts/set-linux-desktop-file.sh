#!/bin/sh
respath=$1
iconpng=$2
iconsvg=$3
desktop=$4
mkdir -p $respath
cp $iconpng $respath
userid=$(id -u)
if [ $userid -eq 0 ]
  then
  mkdir -p /usr/local/share/applications/
  cp $desktop /usr/local/share/applications/
  mkdir -p /usr/share/icons/hicolor/scalable/apps/
  mkdir -p /usr/share/icons/hicolor/256x256/apps/
  cp $iconpng /usr/share/icons/hicolor/256x256/apps/
  cp $iconsvg /usr/share/icons/hicolor/scalable/apps/
  exit
else
  mkdir -p ~/.local/share/applications/
  cp $desktop ~/.local/share/applications/
  mkdir -p ~/.local/share/icons/hicolor/scalable/apps/
  mkdir -p ~/.local/share/icons/hicolor/256x256/apps/
  cp $iconpng ~/.local/share/icons/hicolor/256x256/apps/
  cp $iconsvg ~/.local/share/icons/hicolor/scalable/apps/
  exit
fi