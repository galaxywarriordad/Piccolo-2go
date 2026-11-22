### 🚀 Step-by-Step Installation & First-Boot Setup

## 💿 Step 1: Flash `TF1` (`64 GB` SanDisk Extreme PRO)

1. Download or build `sdcard.img` (`output/images/sdcard.img`).
2. Flash `sdcard.img` onto your **64 GB SanDisk Extreme PRO** using **Rufus** (Windows), **balenaEtcher** (Windows/macOS/Linux), or `dd` on Linux:
   ```sh
   sudo dd if=sdcard.img of=/dev/sdX bs=4M status=progress conv=fsync
