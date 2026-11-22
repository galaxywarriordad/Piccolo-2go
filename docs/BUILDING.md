### 🚀 Building from Source

# Clone Buildroot and the Piccolo-2go external tree
git clone [https://github.com/buildroot/buildroot.git](https://github.com/buildroot/buildroot.git)
git clone [https://github.com/galaxywarriordad/Piccolo-2go.git](https://github.com/galaxywarriordad/Piccolo-2go.git)

# Configure out-of-tree build with BR2_EXTERNAL
cd buildroot
make O=../build-piccolo-2go BR2_EXTERNAL=../Piccolo-2go piccolo_2go_rk3326_defconfig

# Compile the < 180 MB SquashFS image
cd ../build-piccolo-2go
make -j$(nproc)
