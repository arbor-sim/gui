#!/bin/sh
iconpng=$1
iconsvg=$2
desktop=$3
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