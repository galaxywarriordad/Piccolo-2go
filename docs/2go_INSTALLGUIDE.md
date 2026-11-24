# 🚀 Piccolo-2go — Installation & First-Boot Setup Guide

* **Target Hardware:** BATLEXP G350 (default) & R36S Handheld Consoles (dtb-select runtime panel selector)  
* **SoC & PMIC:** Rockchip RK3326 (4× ARM Cortex-A35, ARM Mali-G31 MP2, 1 GB LPDDR3) \+ Rockchip RK817  
* **Firmware:** Piccolo-2go v1.1 (aarch64 Buildroot BR2\_EXTERNAL, 128 KB\-block zstd SquashFS \&lt; 140 MB)  
* **Ecosystem: The Silicon Preservation Society** (Optimised with AX Tech)

**Piccolo-2go** is a deterministic 64-bit (aarch64) Buildroot operating system (128 KB\-block zstd SquashFS \&lt; 140 MB) engineered as a closed, fixed-hardware retro console and bit-perfect audiophile transport for the **BATLEXP G350** and **R36S** handhelds on **Rockchip RK3326** silicon. The initial v1.1 release focuses on **FinalBurn Neo (FBNeo)** and the **FLAC\_AX** media player, with additional emulator cores planned for later releases. This guide covers flashing the TF1 (64 GB) OS image, preparing the TF2 (128 GB) media card, selecting Device Tree Blobs (DTBs), configuring /boot/ax-2go.conf profiles, performing safe shutdowns via ax-powerd, and applying on-device hardware calibrations.

# 💿 1\. Flash TF1 (64 GB SanDisk Extreme PRO) & Select DTB

## 🛠️ Flash sdcard.img to TF1

1. Obtain or compile sdcard.img (build-piccolo-2go/images/sdcard.img or output/images/sdcard.img from the GitHub Actions workflow artifact piccolo-2go-rk3326-sdcard).  
2. Flash sdcard.img onto your **64 GB SanDisk Extreme PRO (A2 / V30)** card using **Rufus** (Windows), **balenaEtcher** (Windows/macOS/Linux), or dd on Linux:

```sh
sudo dd if=sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
```

## 🖥️ Select Your Device Tree (DTB) on BOOT (p1)

Piccolo-2go is developed and primarily tested on the **BATLEXP G350**, so **rk3326-g350-linux.dtb** is active by default on BOOT (p1).

1. Re-insert the 64 GB card into your host PC and open the BOOT (256 MB, FAT32) partition.  
2. If you are using a different RK3326 revision, copy the matching file from dtbs/ on BOOT over **rk3326-g350-linux.dtb** (or run dtb-select \<alias\> once booted):  
   1. **BATLEXP G350 (Default):** rk3326-g350-linux.dtb (already active by default)  
   2. **R36S (Panel 4 Type 1 / Type 2):** Copy dtbs/rk3326-r36s-panel4-type1-linux.dtb (or type2) → rk3326-g350-linux.dtb  
   3. **R36S (Original / Panel 0–3):** Copy dtbs/rk3326-r36s-panel0-linux.dtb (or panel1..3) → rk3326-g350-linux.dtb  
   4. **R36S (V21 / V22 PCB):** Copy dtbs/rk3326-r36s-v21-linux.dtb → rk3326-g350-linux.dtb  
   5. **R36S Plus / R36H / R46H:** Copy dtbs/rk3326-r36splus-linux.dtb, rk3326-r36h-linux.dtb, or rk3326-r46h-linux.dtb → rk3326-g350-linux.dtb

💡 **Hold-Button Safe Recovery:** If you switch to an incompatible .dtb and get a black screen, hold **SELECT**, **FN**, or **B** while powering on—ax-boot-recovery restores rk3326-g350-linux.dtb, 1.296 GHz (1.30 GHz) safe CPU clocks, 520 MHz GPU, and 666 MHz DDR3 backup timings.

# 📂 2\. Prepare TF2 (128 GB SanDisk Extreme PRO)

## 🗂️ Partition & Directory Layout

1. Format your **128 GB SanDisk Extreme PRO (A2 / V30)** card as a single **exFAT** (or ext4) partition labeled PICCOLO\_MEDIA (\~119 GB usable).  
2. Create directories on TF2 (automatically bound to /userdata/{roms,music,bios} on boot by S02piccolo-2go-storage):  
   1. roms/fbneo/ (for initial **FBNeo** arcade/Neo Geo ROMs, plus folders for future cores as they are added)  
   2. music/ (lossless .flac albums, .dsf/.dffDSD64/DSD128 files, and .mod/.xm/.s3m/.it tracker modules)  
   3. bios/ (firmware/BIOS files as needed, such as neogeo.zip for FBNeo)

# ⚡ 3\. First Boot, Storage Expansion & Safe Shutdown (ax-powerd)

## ▶️ First Boot & Automatic SDR50 Verification

1. Keep your stock factory microSD card safely stored as a backup, then insert the **64 GB** card into **TF1 (OS)** and the **128 GB** card into **TF2 (Media)**.  
2. Hold the **Power** button for \~2 seconds to boot. On first boot, S02piccolo-2go-storage verifies **SDR50 (50 MHz, 3.3V signaling, \~24 MB/s)** across mmc0 and mmc1, expands TF1 p3 (\~59 GB ext4, noatime,nodiratime,commit=60,data=writeback), initializes 1024 MB LZ4 ZRAM, and binds TF2 into /userdata.

## 🔌 Safe Shutdown (ax-powerd on Core 3\)

* **Always hold KEY\_POWER for 1.5 seconds** (or press FN \+ Power) to power off. ax-powerd sends SIGUSR1 to release the **HiBy FC3 (£29.61, ES9281 PRO / ES9281AC Pro, 1.3 Vrms, VID 0x32BB)** USB-C OTG DAC cleanly, flushes saves, unmounts both SanDisk cards, and powers down the RK817 PMIC without SD corruption. The 3.5mm wired **Moondrop Chu** paired with the **HiBy FC3 (£29.61, ES9281 PRO / ES9281AC Pro, 1.3 Vrms)** is the recommended primary setup, with **JCALLY JM6 Pro (£12.99, CX31993 \+ MAX97220)** and **HiBy Digital D1 (£11.11, CS46L41)** as the two backup USB-C DAC options (and **Moondrop FreeDSP Pro** with CS43198 \+ 2nd-gen DSP noted as the premium USB option).

# 🔧 4\. Hardware Overrides (/boot/ax-2go.conf) & On-Device DTB Calibration

## ⚙️ Configuring /boot/ax-2go.conf

Edit /boot/ax-2go.conf directly on the FAT32BOOT partition from Windows, macOS, or Linux:

* **HW\_PROFILE (safe \| default \| oc\_boost):**  
* **HW\_PROFILE (safe \| default \| oc\_boost):**  
  * default*(Default)*: 1344 MHz CPU (1112.5 mV), 600 MHz Mali-G31 GPU (1137.5 mV), 720 MHz LPDDR3 (1112.5 mV).  
  * oc\_boost: 1392 MHz CPU, 620 MHz GPU (1150.0–1162.5 mV custom OPP push), 786+ MHz LPDDR3—guarded by ax-powerd automatic 75°C thermal rollback and 3280 mV LiPo brownout protection.  
* **SD\_PROFILE (sdr50\_pro \| hs\_compat \| audio\_quiet):**  
  * sdr50\_pro*(Default)*: 50 MHzSDR50 for SanDisk Extreme PRO (A2/V30) cards (256 KBTF1 / 512 KBTF2 read-ahead).  
  * hs\_compat: 25 MHz High-Speed fallback (128 KB read-ahead) for older or CRC-sensitive cards.  
  * audio\_quiet: Conservative 128 KB read-ahead paired with 64 MiBFLAC\_AX RAM preload on Core 3.

## 💾 Backup Your Default DTB

* Copy /boot/rk3326-g350-linux.dtb into /boot/dtbs/g350-backup.dtb on TF1 before modifying it so you can restore the G350 baseline anytime via dtb-select g350.

## 🔋 Calibrate the BATLEXP G350 3,200 mAh Battery Fuel Gauge (rk817)

* Update design\_capacity and design\_qmax to **3200 mAh (0x0c80)** in /boot/rk3326-g350-linux.dtb so battery percentage reporting matches the G350's 3,200 mAh LiPo cell:

```sh
fdtput -t i /boot/rk3326-g350-linux.dtb /i2c@ff3c0000/rk817@20/battery design_capacity 3200
fdtput -t i /boot/rk3326-g350-linux.dtb /i2c@ff3c0000/rk817@20/battery design_qmax 3200
```

## 📡 Gate Unused Onboard SDIO Wi-Fi (dwmmc@ff380000)

```sh
fdtput -t s /boot/rk3326-g350-linux.dtb /dwmmc@ff380000 status "disabled"
```

## 🎮 Fix Inverted Analog Axes or Deadzones (odroidgo3-joypad)

```sh
fdtput -t empty /boot/rk3326-g350-linux.dtb /odroidgo3-joypad invert-absy
fdtput -t i /boot/rk3326-g350-linux.dtb /odroidgo3-joypad button-adc-deadzone 120
```

# 📚 5\. Related Documentation

* [Complete User Guide & Hardware Manual](http://2go_USERGUIDE.md) — Full 1:2:1 isolation, HiBy FC3 gain staging, backup USB DACs, and dual-SD tuning details.  
* [System Architecture & Fixed-Hardware Topology](http://ARCHITECTURE.md) — Full RK3326 \+ RK817 interconnect diagram and v1.1 mixed-signal refinements.  
* [Building from Source](http://BUILDING.md) — Out-of-tree Buildroot compilation and CMake DTB/profile staging guide.  
* [Technical & Mixed-Signal Engineering Specification](http://technical_engineering.md) — Deep-dive silicon engineering, throughput analysis, and build audit.
