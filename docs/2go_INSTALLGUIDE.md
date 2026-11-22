## 3. 🚀 Step-by-Step Installation & First-Boot Setup

### 💿 Step 1: Flash `TF1` (`64 GB` SanDisk Extreme PRO)

1. Download or build `sdcard.img` (`output/images/sdcard.img`).
2. Flash `sdcard.img` onto your **64 GB SanDisk Extreme PRO** using **Rufus** (Windows), **balenaEtcher** (Windows/macOS/Linux), or `dd` on Linux:

   ```sh
   sudo dd if=sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
   ```

3. **Select Your Device Tree (DTB) on `BOOT` (`p1`):**
   * Re-insert the `64 GB` card into your PC and open the `BOOT` (`FAT32`) partition.
   * Copy the matching `.dtb` for your hardware to the root of `BOOT` renamed as **`rk3326-linux.dtb`**:
     * **BATLEXP G350 (Default):** Copy `dtbs/rk3326-batlexp-g350.dtb` → `rk3326-linux.dtb`
     * **R36S (Original V1 Screen):** Copy `dtbs/rk3326-r36s-v1.dtb` → `rk3326-linux.dtb`
     * **R36S (Panel 4 Revision):** Copy `dtbs/rk3326-r36s-panel4.dtb` → `rk3326-linux.dtb`

> 💡 **Tip:** If you ever boot with a black screen on a clone revision, swap `rk3326-linux.dtb` on `BOOT`—or run `dtb-select g350` / `dtb-select r36s-panel4` over SSH/serial[cite: 2].

### 📂 Step 2: Prepare `TF2` (`128 GB` SanDisk Extreme PRO)

1. Format your **128 GB SanDisk Extreme PRO** as a single **`exFAT`** (or `ext4`) partition labeled `PICCOLO_MEDIA`.
2. Create three top-level folders on `TF2`:
   * `roms/` (`amiga`, `amigacd32`, `atarist`, `megadrive`, `psx`, `snes`, `gba`, `arcade`)
   * `music/` (your lossless `.flac` albums, `.dsf`/`.dff` DSD files, and Amiga/ST `.mod`/`.xm`/`.s3m`/`.it` tracker modules)
   * `bios/` (Amiga Kickstart `kick34005.A500` / `kick40068.A1200`, Atari ST `tos.img`, PS1 `scph5501.bin`)[cite: 2]

### ⚡ Step 3: First Boot & Safe Shutdown

1. Insert the **64 GB** card into **`TF1` (OS)** and the **128 GB** card into **`TF2` (Media)**.
2. Hold the **Power** button for ~2 seconds to boot. On first boot, `S02piccolo-2go-storage` automatically expands `TF1` `p3` (`~59 GB` `ext4`) and binds `TF2` (`128 GB`) into `/userdata`.
3. **Safe Shutdown (`ax-powerd`):** Always **hold the Power button for `1.5` seconds** to power off. `ax-powerd` (pinned to Core `3`) sends `SIGUSR1` to release the HiBy FC3 DAC cleanly, flushes save files, unmounts both SanDisk cards, and powers down the RK817 PMIC without SD corruption[cite: 2].

### 🔧 Step 4: On-Device Hardware Calibration & Clock Profile (`dtb-select` & `fdtput`)

Because the **BATLEXP G350** and **R36S** ship across multiple factory revisions with different MIPI DSI panels, analog stick wiring, and battery fuel-gauge values, `Piccolo-2go` includes `dtc` (`fdtput`) so you can calibrate your exact board directly on the handheld (or via SSH/serial):

1. **Preserve Your Factory Stock DTB (Recommended Backup):**
   * Before formatting your original factory microSD card, copy its `.dtb` file (`rk3326-rg351mp-linux.dtb` or `rk3326-r36s-linux.dtb`) into `/boot/dtbs/factory-backup.dtb` on your **64 GB SanDisk Extreme PRO (`TF1`)**. If you ever encounter a new screen revision, `dtb-select factory-backup.dtb` restores exact stock panel timings immediately.

2. **Calibrate the BATLEXP G350 `3,200 mAh` Battery Fuel Gauge (`rk817`):**
   * Many R36S DTBs default to `2500–3000 mAh`, causing inaccurate battery percentages on the G350's **3,200 mAh** LiPo cell. `Piccolo-2go` sets `design_capacity` and `design_qmax` to **`3200` mAh (`0x0c80`)** in the `rk817` battery node:
     ```sh
     fdtput -t i /boot/rk3326-linux.dtb /i2c@ff3c0000/rk817@20/battery design_capacity 3200
     fdtput -t i /boot/rk3326-linux.dtb /i2c@ff3c0000/rk817@20/battery design_qmax 3200
     ```

3. **Gate Unused Onboard SDIO Wi-Fi (`dwmmc@ff380000`):**
   * Neither the BATLEXP G350 nor the R36S has built-in Wi-Fi. Disabling the unpopulated `dwmmc@ff380000` SDIO controller in the DTB eliminates idle clock/pin leakage and saves battery (USB-C OTG Wi-Fi dongles remain unaffected):
     ```sh
     fdtput -t s /boot/rk3326-linux.dtb /dwmmc@ff380000 status "disabled"
     ```

4. **Fix Inverted Analog Sticks or Deadzones (`odroidgo3-joypad`):**
   * If a board revision has inverted X/Y thumbstick axes, toggle the `invert-abs*` properties directly in `/boot/rk3326-linux.dtb` without recompiling anything:
     ```sh
     # Example: invert Left-Y or Right-X axis if reversed on your board
     fdtput -t empty /boot/rk3326-linux.dtb /odroidgo3-joypad invert-absy
     fdtput -t i /boot/rk3326-linux.dtb /odroidgo3-joypad button-adc-deadzone 120
     ```

5. **Authentic `50 Hz` PAL vs. `60 Hz` NTSC Panel Refresh Switching:**
   * For silky-smooth `50.00 Hz` scrolling in **Amiberry** (Commodore Amiga) and **Hatari** (Atari ST) without `50→60 Hz` frame duplication, switch the `panel@0` `native-mode` phandle to the `50 Hz` timing node when supported by your panel, then reboot.

6. **CPU Clock & Thermal Profile (`1.3 GHz` Safe Default vs. `1.4 GHz` Overclock):**
   * **Default Safe Mode (`1.296 GHz` / `1.3 GHz`):** Locked out of the box in `S01piccolo-2go-isolate` because the BATLEXP G350 and R36S have passive, unvented shells that overheat at `1.4–1.5 GHz`. Keeps SoC temperatures around `40–52°C` with zero thermal throttling.
   * **Opt-In `1.4 GHz` Overclock (`1416000 kHz`):** For demanding PlayStation 1 or AGA Amiga titles (recommended if you have added a thin copper heatsink shim), create `/boot/ax-cpu.conf` containing `OVERCLOCK_1400=1`. `ax-powerd` monitors `thermal_zone0` and automatically steps the cluster back down to `1.3 GHz` if temperatures reach `75°C`.
