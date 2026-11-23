# 🏗️ Building Piccolo-2go from Source

**Piccolo-2go** is built as an out-of-tree Buildroot (`BR2_EXTERNAL`) `aarch64` image for **BATLEXP G350** and **R36S** handhelds on **Rockchip RK3326** silicon (`1:2:1` core/IRQ isolation, `1024 MB` `LZ4` ZRAM, `SDR50` bus verification, and `128 KB`-block `zstd` SquashFS `< 140 MB`).

---

## 📦 1. Clone Repositories

### 🌳 Upstream Buildroot and Piccolo-2go External Tree

```bash
git clone [https://github.com/buildroot/buildroot.git](https://github.com/buildroot/buildroot.git)
git clone [https://github.com/galaxywarriordad/Piccolo-2go.git](https://github.com/galaxywarriordad/Piccolo-2go.git)
```

---

## ⚙️ 2. Configure Out-of-Tree Build

### 🧩 Apply the RK3326 Defconfig

```bash
cd buildroot
make O=../build-piccolo-2go BR2_EXTERNAL=../Piccolo-2go piccolo_2go_rk3326_defconfig
```

---

## 🛠️ 3. Compile the Image

### 🚀 Build the `aarch64` Target Image

```bash
cd ../build-piccolo-2go
make -j 4
```

### 💾 Output Artifact

```text
build-piccolo-2go/images/sdcard.img
```

---

## 🧭 4. Next Steps

* 💿 Flash `sdcard.img` onto your `64 GB` SanDisk Extreme PRO (`TF1`) and prepare `TF2` (`128 GB`) using the installation guide.
* 🎧 Review the `1:2:1` core topology, `1024 MB` `LZ4` ZRAM tuning, and early `HiBy FC3` (`1.3 Vrms`) audio routing in the user and architecture guides.
