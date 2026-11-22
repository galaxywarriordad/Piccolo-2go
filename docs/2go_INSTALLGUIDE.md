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
