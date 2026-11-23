# 🚀 Piccolo-2go — Installation & First-Boot Setup Guide

* **Target Hardware:** BATLEXP G350 (default) & R36S Handheld Consoles (`dtb-select` runtime panel selector)
* **SoC & PMIC:** Rockchip RK3326 (`4× ARM Cortex-A35`, `ARM Mali-G31 MP2`, `1 GB LPDDR3`) + Rockchip `RK817`
* **Firmware:** `Piccolo-2go` (`aarch64` Buildroot `BR2_EXTERNAL`, `128 KB`-block `zstd` SquashFS `< 140 MB`)
* **Ecosystem:** **The Silicon Preservation Society** (Optimised with AX Tech)[cite: 12]

**Piccolo-2go** is a deterministic 64-bit (`aarch64`) Buildroot operating system (`128 KB`-block `zstd` SquashFS `< 140 MB`) engineered as a closed, fixed-hardware retro console and bit-perfect audiophile transport for the **BATLEXP G350** and **R36S** handhelds on **Rockchip RK3326** silicon. The initial release focuses on **FinalBurn Neo (`FBNeo`)** and a **media player** (`FLAC_AX`), with additional emulator cores planned for later releases. This guide covers flashing the `TF1` (`64 GB`) OS image, preparing the `TF2` (`128 GB`) media card, selecting Device Tree Blobs (`DTBs`), performing safe shutdowns via `ax-powerd`, and applying on-device hardware and clock calibrations[cite: 3, 12, 16].

---

## 💿 1. Flash `TF1` (`64 GB` SanDisk Extreme PRO) & Select DTB

### 🛠️ Flash `sdcard.img` to `TF1`

1. Obtain or compile `sdcard.img` (`build-piccolo-2go/images/sdcard.img` or `output/images/sdcard.img`)[cite: 12].
2. Flash `sdcard.img` onto your **64 GB SanDisk Extreme PRO (`A2` / `V30`)** card using **Rufus** (Windows), **balenaEtcher** (Windows/macOS/Linux), or `dd` on Linux[cite: 12]:

```sh
sudo dd if=sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
```

### 🖥️ Select Your Device Tree (DTB) on `BOOT` (`p1`)

`Piccolo-2go` is developed and primarily tested on the **BATLEXP G350**, so **`rk3326-g350-linux.dtb`** is active by default[cite: 3, 16].

1. Re-insert the `64 GB` card into your host PC and open the `BOOT` (`256 MB`, `FAT32`) partition[cite: 12, 16].
2. If you are using a different RK3326 revision, copy the matching file from `dtbs/` on `BOOT` over **`rk3326-g350-linux.dtb`** (or run `dtb-select <alias>` once booted)[cite: 3, 16]:
   * **BATLEXP G350 (Default):** `rk3326-g350-linux.dtb` (already active by default)[cite: 3, 16]
   * **R36S (Panel 4):** Copy `dtbs/rk3326-r36s-panel4-type1-linux.dtb` → `rk3326-g350-linux.dtb`[cite: 16]
   * **R36S (Original / Panel 0):** Copy `dtbs/rk3326-r36s-panel0-linux.dtb` → `rk3326-g350-linux.dtb`[cite: 16]

> 💡 **Hold-Button Safe Recovery:** If you switch to an incompatible `.dtb` and get a black screen, hold **`SELECT`**, **`FN`**, or **`B`** while powering on—`ax-boot-recovery` restores `rk3326-g350-linux.dtb`, `1.30 GHz` safe CPU clocks, and `666 MHz` DDR3 backup timings[cite: 16].

---

## 📂 2. Prepare `TF2` (`128 GB` SanDisk Extreme PRO)

### 🗂️ Partition & Directory Layout

1. Format your **128 GB SanDisk Extreme PRO (`A2` / `V30`)** card as a single **`exFAT`** (or `ext4`) partition labeled `PICCOLO_MEDIA` (`~119 GB` usable)[cite: 12, 16].
2. Create directories on `TF2` (automatically bound to `/userdata/{roms,music,bios}` on boot by `S02piccolo-2go-storage`)[cite: 12, 16]:
   * `roms/fbneo/` (for initial **FBNeo** arcade/Neo Geo ROMs, plus folders for future cores as they are added)[cite: 3, 12, 16]
   * `music/` (lossless `.flac` albums, `.dsf`/`.dff` `DSD64`/`DSD128` files, and `.mod`/`.xm`/`.s3m`/`.it` tracker modules)[cite: 12, 16]
   * `bios/` (firmware/BIOS files as needed)[cite: 12, 16]

---

## ⚡ 3. First Boot, Storage Expansion & Safe Shutdown (`ax-powerd`)

### ▶️ First Boot & Automatic `SDR50` Verification

1. Keep your stock factory microSD card safely stored as a backup, then insert the **64 GB** card into **`TF1` (OS)** and the **128 GB** card into **`TF2` (Media)**[cite: 12].
2. Hold the **Power** button for `~2` seconds to boot. On first boot, `S02piccolo-2go-storage` verifies **`SDR50` (`50 MHz`, `3.3V` signaling, `~24 MB/s`)** across `mmc0` and `mmc1`, expands `TF1` `p3` (`~59 GB` `ext4`, `noatime,nodiratime,commit=60,data=writeback`), initializes `1024 MB` `LZ4` ZRAM, and binds `TF2` into `/userdata`[cite: 12].

### 🔌 Safe Shutdown (`ax-powerd` on `Core 3`)

* **Always hold `KEY_POWER` for `1.5` seconds** (or press `FN + Power`) to power off. `ax-powerd` sends `SIGUSR1` to release the **HiBy FC3 (`1.3 Vrms`, VID `0x32BB`)** USB-C OTG DAC cleanly, flushes saves, unmounts both SanDisk cards, and powers down the `RK817` PMIC without SD corruption[cite: 12].

---

## 🔧 4. On-Device Hardware Calibration & Clock Profiles (`dtb-select` & `fdtput`)

Because **BATLEXP G350** and **R36S** boards ship across multiple revisions with different MIPI DSI panels, thumbstick wiring, and battery fuel-gauge defaults, `Piccolo-2go` provides `dtb-select` and `dtc` (`fdtput`) for direct on-device or SSH/serial calibration[cite: 12]:

### 💾 Backup Your Default DTB

* Copy `/boot/rk3326-g350-linux.dtb` into `/boot/dtbs/g350-backup.dtb` on `TF1` before modifying it so you can restore the G350 baseline anytime via `dtb-select g350`[cite: 12, 16].

### 🔋 Calibrate the BATLEXP G350 `3,200 mAh` Battery Fuel Gauge (`rk817`)

* Update `design_capacity` and `design_qmax` to **`3200` mAh (`0x0c80`)** in `/boot/rk3326-g350-linux.dtb` so battery percentage reporting matches the G350's `3,200 mAh` LiPo cell[cite: 12, 16]:

```sh
fdtput -t i /boot/rk3326-g350-linux.dtb /i2c@ff3c0000/rk817@20/battery design_capacity 3200
fdtput -t i /boot/rk3326-g350-linux.dtb /i2c@ff3c0000/rk817@20/battery design_qmax 3200
```

### 📡 Gate Unused Onboard SDIO Wi-Fi (`dwmmc@ff380000`)

* Disable the unpopulated onboard SDIO controller in `/boot/rk3326-g350-linux.dtb` to reduce idle clock/pin leakage while keeping USB-C OTG adapters functional[cite: 12, 16]:

```sh
fdtput -t s /boot/rk3326-g350-linux.dtb /dwmmc@ff380000 status "disabled"
```

### 🎮 Fix Inverted Analog Axes or Deadzones (`odroidgo3-joypad`)

* Toggle axis inversion or analog deadzone values directly in `/boot/rk3326-g350-linux.dtb` without recompiling[cite: 12, 16]:

```sh
fdtput -t empty /boot/rk3326-g350-linux.dtb /odroidgo3-joypad invert-absy
fdtput -t i /boot/rk3326-g350-linux.dtb /odroidgo3-joypad button-adc-deadzone 120
```

### 🖥️ CPU Clock & Thermal Profiles

* **RK3326 CPU PLL Steps (`24 MHz` Increments, `720 MHz` LPDDR3 @ `1112.5 mV` Default):**
  * `1296000 kHz` (`1.296 GHz` / `1.30 GHz` — Default): Stock safe ceiling for passive, unvented shells (`~42–52°C`)[cite: 12].
  * `1344000 kHz` (`1.344 GHz` / `1.34 GHz` — Sweet-Spot `+48 MHz`): Configured via `CPU_PROFILE=1.34` in `/boot/ax-cpu.conf` or `S01piccolo-2go-isolate set-1.34` (`~52–58°C`)[cite: 12].
  * `1392000 kHz` (`1.392 GHz` / `1.39 GHz` — High `+96 MHz`): High ceiling just under `1.4 GHz` (`~58–65°C`), guarded by `ax-powerd` automatic **`75°C`** thermal rollback and **`3280 mV`** LiPo brownout shutdown[cite: 12].
