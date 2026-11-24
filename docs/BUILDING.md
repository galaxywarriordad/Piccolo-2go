# 🏗️ Building Piccolo-2go from Source

**Piccolo-2go** (**v1.1**) is built as an out-of-tree Buildroot (**BR2\_EXTERNAL**) **aarch64** image for **BATLEXP G350** (default) and **R36S** handhelds on **Rockchip RK3326** silicon (**1:2:1** core/IRQ isolation, **1024 MB** **LZ4** ZRAM, **RGA2-Lite** **2×** scaling offload, **mlock()** \+ **ax\_hugepage\_shim.so**, **SDR50** bus verification, and **128 KB**\-block **zstd** SquashFS **\< 140 MB**).

## 📦 1\. Clone Repositories

### 🌳 Upstream Buildroot and Piccolo-2go External Tree

```sh
git clone https://github.com/buildroot/buildroot.git
git clone https://github.com/galaxywarriordad/Piccolo-2go.git
```

## \#\# ⚙️ 2\. Configure Out-of-Tree Build

### \#\#\# 🧩 Apply the RK3326 Defconfig

```sh
cd buildroot
make O=../build-piccolo-2go BR2_EXTERNAL=../Piccolo-2go piccolo_2go_rk3326_defconfig
```

## \#\# 🛠️ 3\. Compile the Image

### \#\#\# 🚀 Build the aarch64 Target Image

```sh
cd ../build-piccolo-2go
make -j 4
```

### \#\#\# 💾 Output Artifact

```
build-piccolo-2go/images/sdcard.img
```

## \#\# 🎛️ 4\. Optional CMake DTB & Hardware Profile Staging (arkos4clone)

To stage device-specific **.dtb** files (**g350** or any R36S/clone variant from **lcdyk0517/arkos4clone**) alongside your **Piccolo-2go 640×480** Espresso/Copper **logo.bmp** splash and **safe** / **default** / **oc\_boost** profile metadata:

* 

```sh
cmake -S Piccolo-2go -B build-cmake \
  -DARKOS4CLONE_DIR=/path/to/arkos4clone \
  -DPICCOLO_CONSOLE=g350 \
  -DPICCOLO_KERNEL=5.10 \
  -DPICCOLO_PROFILE=default
cmake --build build-cmake
```

* build-cmake/stage\_boot/ contains the staged .dtb, boot.ini, piccolo-profile.env, and Piccolo-2go's logo.bmp splash (never upstream dArkOS splash artwork).

* In GitHub Actions, run the **Verify Piccolo-2go CMake Build** (cmake-verify.yml) or **Validate Piccolo-2go External Tree** (ci.yml) workflows to validate scripts, C tools, and sdcard.img packaging in the cloud.

## 🧭 5\. Next Steps

* 💿 Flash **sdcard.img** onto your **64 GB** SanDisk Extreme PRO (**TF1**) and prepare **TF2** (**128 GB**) using the Installation & First-Boot Setup Guide.  
* 🎧 Review the **1:2:1** core topology, **HW\_PROFILE** / **SD\_PROFILE** tuning, **3.5mm** wired **Moondrop Chu** \+ **HiBy FC3 (£29.61, ES9281 PRO / ES9281AC Pro)** (**1.3 Vrms**) primary setup, and **JCALLY JM6 Pro (£12.99, CX31993 \+ MAX97220)** / **HiBy Digital D1 (£11.11, CS46L41)** backup USB-C DACs in the Complete User Guide.  
* 🔌 Inspect the **RK3326** \+ **RK817** chip interconnect diagram and **v1.1** mixed-signal refinements in System Architecture and Technical Engineering.

