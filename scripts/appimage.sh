back_again=$(pwd)
if [ -z "$GITHUB_ACTIONS" ]; then CMAKECOMP=""; else CMAKECOMP="-DCMAKE_CXX_COMPILER=g++-10"; fi
bld_dir=$(mktemp -d)
app_dir=$(mktemp -d)
tmp_dir=$(mktemp -d)
cmake -S . -B $bld_dir "$CMAKECOMP" --install-prefix $app_dir/usr -DARB_ARCH=x86-64-v2
cmake --build $bld_dir -j 4
cmake --install $bld_dir
cd "$tmp_dir"
wget https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage
# TODO: if versioning is solved, and we have a stable URL for releases, we might enable self-updating.
# if [ ! -z "$GITHUB_ACTIONS" ]
# then
#     export ARCH=x86_64
#     export UPDATE_INFORMATION="gh-releases-zsync|${GITHUB_REPOSITORY//\//|}|${VERSION:-"continuous"}|CPU-X-*$ARCH.AppImage.zsync"
# fi
./linuxdeploy-x86_64.AppImage --appdir "$app_dir" --output appimage
rm ./linuxdeploy-x86_64.AppImage
mv ./*.AppImage* "$back_again"
