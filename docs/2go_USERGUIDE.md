# ☕📖 Piccolo-2go — Complete User Guide & Hardware Manual (BATLEXP G350 & R36S)

* **Target Hardware:** BATLEXP G350 (default) & R36S Handheld Consoles (dtb-select runtime panel selector)  
* **SoC & PMIC:** Rockchip RK3326 (4× ARM Cortex-A35, ARM Mali-G31 MP2, 1 GB LPDDR3) \+ Rockchip RK817  
* **Firmware:** Piccolo-2go v1.1 (aarch64 Buildroot BR2\_EXTERNAL, 128 KB\-block zstd SquashFS \< 140 MB)  
* **Ecosystem:** **The Silicon Preservation Society** (Optimised with AX Tech)

**Piccolo-2go** is a deterministic 64-bit (aarch64) Buildroot operating system (128 KB\-block zstd SquashFS \< 140 MB) engineered as a closed, fixed-hardware retro console and bit-perfect audiophile transport for the **BATLEXP G350** and **R36S** handhelds on **Rockchip RK3326** silicon. The initial v1.1 release focuses on **FinalBurn Neo (FBNeo)** and the **FLAC\_AX** media player, with additional emulator cores planned for later releases. It eliminates desktop bloat, X11/Wayland compositors, background daemons, and user-space audio mixers (PulseAudio/PipeWire) to turn a pocket handheld into a deterministic arcade system and bit-perfect S32\_LE / DoP DAC transport.

# 1\. 🧠 Silicon & 1:2:1 Core/IRQ Partition (4× ARM Cortex-A35 \+ 1 GB LPDDR3)

The **BATLEXP G350** and **R36S** run on the **Rockchip RK3326** (4× ARM Cortex-A35, 32 KB L1-I / 32 KB L1-D per core, 256 KB shared L2, ARM Mali-G31 MP2 GPU, 1 GB 32-bit LPDDR3, 3.5" 640×480 4:3 OCA MIPI DSI IPS screen). Because the Cortex-A35 uses a dual-issue **in-order** microarchitecture sharing a single 256 KB L2 cache with no dedicated VRAM, unpinned hardware interrupts and background tasks immediately flush L1 caches and inject multi-millisecond audio clicks and frame-pacing judder.  
Piccolo-2go enforces a strict **1:2:1 physical core and IRQ partition** via tickless kernel boot flags (isolcpus=0,1,2 nohz\_full=0,1,2 rcu\_nocbs=0,1,2 threadirqs) and S01piccolo-2go-isolate (pinning both hardware IRQ affinities and threaded irq/\<N\>-\* handlers):

| Core | Affinity Mask | Scheduler & Governor | Dedicated Hardware Plane & Pinned IRQs |
| :---- | :---- | :---- | :---- |
| **Core 0** | 0x01 | SCHED\_FIFO (prio 80) / performance | **Real-Time Audio Clock & Controller IRQ Plane:** Direct ALSA S32\_LE writer (192\-frame / 4.00 ms period); DWC2 USB-C OTG (HiBy FC3 primary, JCALLY JM6 Pro / HiBy Digital D1 backups, bInterval=4 1 ms microframe coalescing), I2S0 (RK817), GPIO button matrix, and 10-bit SARADC control IRQs |
| **Core 1** | 0x02 | SCHED\_OTHER / performance | **Zero-Copy GPU & Display Scanout Plane:** Drives **Mali-G31 MP2** (Mesa Panfrost OpenGL ES 3.0), fixed-function **RGA2-Lite** 2× integer scaling (CONFIG\_VIDEO\_ROCKCHIP\_RGA=y), and atomic KMS/DRM page flips (SDL\_VIDEODRIVER=kmsdrm, PAN\_MESA\_DEBUG=noafbc) to the 640×480 @ 60.00 Hz panel |
| **Core 2** | 0x04 | SCHED\_OTHER / performance | **Deterministic Emulation & DSP Plane:** Runs FBNeo and DSP workers with hot 32 KB L1-I / 32 KB L1-D caches, mlock() audio rings, and LD\_PRELOAD 2 MB HugePage backing (ax\_hugepage\_shim.so) |
| **Core 3** | 0x08 | SCHED\_OTHER (nice \+5) / schedutil | **Storage DMA, ZRAM & Housekeeping Plane:** dwmmc (TF1/TF2 SanDisk SD) DMA IRQs, 1024 MB LZ4 ZRAM, 64 MiB FLAC\_AX RAM preloader, ax\_appliance\_tui, and ax-powerd |

## ⚙️ RK3326 Clock, Voltage & Memory Tuning Parameters (/boot/ax-2go.conf)

* **Unified Hardware Presets (HW\_PROFILE):**  
  * **default (Default Mode):** 1344 MHz CPU (1112.5 mV), 600 MHz Mali-G31 GPU (1137.5 mV), 720 MHz LPDDR3 (1112.5 mV).  
  * **safe (Safe Mode / Pure Audio Floor):** 1296 MHz CPU (1100.0 mV), 520 MHz GPU (1100.0 mV), 666 MHz LPDDR3 (1100.0 mV).  
  * **oc\_boost (OC / Boost Mode):** 1392 MHz CPU, 620 MHz GPU (1150.0–1162.5 mV), 786+ MHz LPDDR3 (guarded by 75°C thermal rollback).  
* **Safe RK3326 CPU PLL Steps (24 MHz Increments, Passive-Cooled Safe):**  
  * 1344000 kHz (1.344 GHz / 1.34 GHz — Default Mode): Configured via CPU\_PROFILE=1.34 in /boot/ax-2go.conf or S01piccolo-2go-isolate set-1.34 (\~52–58°C).  
  * 1296000 kHz (1.296 GHz / 1.30 GHz — Safe Mode / Audio Floor): Stock safe ceiling (\~42–52°C, zero thermal throttling; also active during pure FLAC\_AX playback to minimize VBUS ripple).  
  * 1392000 kHz (1.392 GHz / 1.39 GHz — OC / Boost Mode): Maximum safe ceiling just under the 1.4 GHz thermal wall (\~58–65°C, guarded by ax-powerd automatic 75°C rollback).  
* **LPDDR3 / DMC Bus Profiles (RK817 DCDC2 — VDD\_LOGIC, 12.5 mV / 12500 µV Steps):**  
  * 720 MHz (1440 MT/s @ 1112.5 mV / 1112500 µV — Default): max\_ddrfreq=720 (\+8.1% memory bandwidth over stock 666 MHz with a single \+12.5 mV step).  
  * 666 MHz (1333 MT/s @ 1100.0 mV / 1100000 µV — Safe Mode): Universal fallback profile (max\_ddrfreq=666) compatible with every clone RAM bin.  
  * 786 MHz / 800–900 MHz (1572–1800 MT/s @ 1137.5–1162.5 mV — OC / Boost & Open-Case): High-bandwidth profile (ALLOW\_HIGH\_DDR=1 for 800+ MHz heatsinked builds).  
* **Mali-G31 MP2 Voltage-Scaled OPP Ladder (12.5 mV / 12500 µV RK817 VDD\_LOGIC Steps):**  
  * 520 MHz (1100.0 mV / 1100000 µV — Safe Baseline): Stock GPU ceiling (GPU\_FREQ=520).  
  * 550 MHz (1112.5 mV / 1112500 µV, \+1 step): Mild shader/fillrate step.  
  * 580 MHz (1125.0 mV / 1125000 µV, \+2 steps): Mid shader/fillrate step.  
  * 600 MHz (1137.5 mV / 1137500 µV, \+3 steps — Default): Standard Default Mode GPU ceiling (GPU\_FREQ=600).  
  * 620 MHz (1150.0–1162.5 mV / 1150000–1162500 µV, \+4–5 steps — OC / Boost Custom Push): Requires explicit DTB/kernel OPP support; guarded by 75°C thermal rollback.  
* **ZRAM, Kernel Pruning, RGA2-Lite & HugePages:**  
  * **1024 MB LZ4 ZRAM:** Configured by S01piccolo-2go-isolate (\~650 MB/s in-order Cortex-A35 decompression, vm.swappiness=120, vm.page-cluster=0, vm.watermark\_scale\_factor=125) alongside bounded /tmp (32 MB) and /run (16 MB) tmpfs.  
  * **Pruned Kernel (linux-rk3326.fragment) \+ RGA2-Lite:** Uses LZ4 kernel compression (\< 180 ms decompression), 64 MB CMA, enables CONFIG\_VIDEO\_ROCKCHIP\_RGA=y for hardware 2× integer scaling, and strips unused SoC blocks (ISP, Crypto, Bluetooth, WLAN), reclaiming \~110 MB of physical LPDDR3.  
  * **Zero-Compositor Linear Scanout & Memory Residency:** Uses PAN\_MESA\_DEBUG=noafbc, RAM shader caching in /tmp/mesa\_shader\_cache (16M cap), explicit mlock() on FLAC\_AX audio rings/preload buffers, and LD\_PRELOAD 2 MB HugePages (ax\_hugepage\_shim.so) for FBNeo ROM arenas.

# 2\. 🎧 Bit-Perfect Audio DAC Subsystem (Early HiBy FC3 1.3 Vrms \+ RK817 Fallback)

## 🔌 A. Primary Setup & Backup USB-C DAC Options (HiBy FC3 1.3 Vrms, JCALLY JM6 Pro & HiBy Digital D1)

Piccolo-2go audio transport design prioritizes a high-fidelity bit-perfect setup. The system explicitly supports three exact external USB-C DAC options available on AliExpress, alongside noting the Moondrop Chu (Chu II / Chu III) paired with the early HiBy FC3 (£29.61, ES9281 PRO / ES9281AC Pro) or Moondrop FreeDSP Pro (CS43198 \+ 2nd-gen DSP) as recommended IEM and cable paths:

* **HiBy FC3 (£29.61, ES9281 PRO / ES9281AC Pro):** Primary reference MQA Dongle Type-C to 3.5mm DAC (112mW output, PCM up to 32-bit/384 kHz, DSD up to DSD128 / 2.8224 MHz & 5.6448 MHz, metal body, Black, 0.010 kg). Deliver **1.3 Vrms** clean output with zero autolock latency spikes or DAC sleep clicks.  
* **JCALLY JM6 Pro (£12.99, CX31993 \+ MAX97220):** High-output Type-C to 3.5mm DAC (up to 500 Ohm impedance rating in listing, metal body, Blue/Red Type-C to 3.5mm, 0.028 kg). Provides high-current Class AB driver capability for full-sized headphones.  
* **HiBy Digital D1 (£11.11, CS46L41):** Ultra-low power Type-C to 3.5mm DAC (PCM 32-bit/384kHz, 44.1/48/96/192/384 kHz, 48 x 13 x 12.25 mm, metal build, Black/Orange/Silver/1YVES, CE & WEEE). Maximizes handheld battery runtime during extended listening sessions.  
* **Zero USB Autosuspend (99-ax-2go-otg.rules):** Immediately locks power/control="on" when the HiBy FC3 (0x32BB / 0x2fc6), JCALLY JM6 Pro, or HiBy Digital D1 is attached, preventing DAC sleep clicks and latency spikes.  
* **Bit-Perfect S32\_LE & Native DoP (DSD64 / DSD128):** Bypasses PulseAudio, PipeWire, and ALSA software resampling (snd\_pcm\_hw\_params\_set\_rate\_resample(0)), outputting **32-bit** integer **S32\_LE (44.1–384 kHz)** PCM and hardware **DoP (DSD64 176.4 kHz / DSD128 352.8 kHz)** with mlock()\-pinned 64 MiB RAM file preloading on Core 3 and bInterval=4 (1 ms) USB microframe coalescing on Core 0\.  
* **Gain-Staged Hardware Volume Presets (fc3\_1v3):** \--chip fc3\_1v3 \--volume 0.95 (\~1.24 Vrms line-out), \--chip fc3\_1v3 \--volume 0.65 (headphones), \--chip fc3\_1v3 \--volume 0.35 (sensitive IEMs).

## 🔊 B. Interactive Connect & Retry (ax-audio-route) & Internal RK817 Fallback

* **Interactive Connect & Retry (ax-audio-route prompt):** If the USB-C OTG DAC is unplugged when launching an audio session, Piccolo-2go prompts you to plug in the DAC and retry (\[R\]) or fall back cleanly (\[S\]) to the internal codec.  
* **Pop-Free Internal RK817 Fallback (hw:CARD=rockchiprk817,DEV=0):** Routes audio to the built-in 3.5 mm stereo headphone jack and 1W mono speaker with clean *Playback Path* initialization and pop-free Class-D shutdown.

## 🎧 C. Recommended IEMs & USB-C Cable Guidance (Moondrop Chu 2 / Chu 3 & FreeDSP Pro)

* For handheld gaming, the Moondrop Chu 2 (and the warmer, smoother Chu 3\) is the clear winner over the original Chu 1, though anyone wanting a USB-C connection should skip the standard DSP cables and specifically buy the Moondrop FreeDSP Pro (or verify a CS43198 DAC with 2nd-gen DSP is included).  
Between the three generations, the Chu 1 has the brightest, most analytical sound, which gives great vocal clarity but suffers from a non-detachable cable that easily tangles or breaks in a travel case. The Chu 2 fixed that flaw by introducing a replaceable 0.78mm 2-pin cable, removable nozzle filters for easy cleaning, and a punchier, V-shaped tuning that adds real weight to sound effects. The Chu 3 acts more as a sidegrade than a strict upgrade; reviewers at SoundGuys and Headphones.com note that it keeps the small metal shell and detachable cable while shifting toward a warmer, thicker mid-bass and slightly more relaxed treble, making it less fatiguing during marathon sessions even if it loses a tiny bit of crispness compared to the Chu 2\.  

* For gaming handhelds, the Chu 2 and Chu 3 shine because of their tiny, bean-shaped zinc-alloy shells. They sit flush inside your ears without sticking out, meaning you can comfortably rest your head against a pillow or headrest while playing in bed or on a flight. Their 10mm dynamic drivers are also super easy to drive (16Ω–18Ω impedance), so they get plenty loud directly from a handheld’s built-in headphone jack or the HiBy FC3 without draining your device's battery.  
Audio-wise, their boosted bass and clean directional imaging make single-player games, retro soundtracks, and action titles feel far more immersive than standard earbuds or tiny handheld speakers. You get satisfying rumble from explosions and footsteps alongside clear dialogue, while the snug in-ear seal blocks out ambient noise. Plus, at around £20–£25 for the standard 3.5mm models, they are cheap enough to toss straight into a handheld carrying case without worrying about damaging expensive gear.  

* When comparing a USB-C setup to a dedicated dongle like the HiBy FC3, you should avoid the budget stock USB-C DSP cables—which suffer from cheap wiring and clunky, limited 1st-gen EQ software—and only opt for USB if you get the Moondrop FreeDSP Pro (or confirm the cable houses a CS43198 DAC chip with 2nd-gen online interactive DSP). Equipped with the flagship CS43198 chip (127dB dynamic range, 2Vrms output) and browser-based parametric EQ, the FreeDSP Pro matches the clean, transparent audio performance and driving power of the HiBy FC3 without leaving a bulky dongle dangling from your handheld—though it costs around £100+, meaning a standard 3.5mm Chu 2 or Chu 3 paired with the HiBy FC3 (or your handheld's built-in 3.5mm RK817 jack, or budget USB backups such as the JCALLY JM6 Pro and HiBy Digital D1) remains far more budget-friendly.

## 🔌 D. USB-C Audio Dongle Topologies: ESS ES9281AC PRO vs. JCALLY JM6 Pro vs. HiBy Digital D1

Understanding the hardware architecture of USB-C audio dongles is essential for selecting the right transport for the BATLEXP G350 and R36S under Piccolo-2go. Different dongle designs use distinct System-on-Chip (SoC) topologies, DAC architectures, and amplifier stages, directly impacting audio performance, power drain, battery runtime, and low-impedance IEM driving capabilities.

### 🔬 1\. USB-C Audio Dongle Topology Analysis

* **Single-Chip Integrated Architecture (e.g., ESS ES9281AC PRO / HiBy FC3):** Integrates the USB bridge, DAC, dynamic clock system, hardware volume registers, and headphone amplifier into a single silicon die. This design minimizes PCB footprint and trace inductance, providing exceptional energy efficiency and dynamic range. The internal architecture features an integrated negative charge pump (PNEG) utilizing two small external flying ceramic capacitors (typically 1 µF–2.2 µF across C1, C2, and PNEG) to generate a symmetric negative power rail. This allows ground-referenced headphone outputs (OUT\_L, OUT\_R) without requiring bulky DC-blocking output capacitors. Furthermore, dedicated Ground-Sense Feedback pins (GSN\_L / GSN\_R) actively monitor the 3.5 mm jack sleeve ground return to dynamically cancel common-mode noise and crosstalk, achieving channel separation of up to \-110 dB. The early HiBy FC3 revision utilizes this topology to deliver a clean **1.3 Vrms** output while operating as Piccolo-2go's primary reference DAC.  
* **Two-Chip Bridge \+ Discrete Op-Amp Architecture (e.g., JCALLY JM6 Pro):** Pairs a low-power USB bridge/DAC SoC (Conexant/Realtek/BES CX31993) with a dedicated high-current operational amplifier (Maxim MAX97220). The DAC handles digital-to-analog conversion while the MAX97220 provides dedicated Class AB differential-input current amplification. In this split architecture, gain is established either via internal fixed \+6 dB configuration (MAX97220A) or external resistor networks (Rin / Rf on MAX97220C), with AC-coupling capacitors (Cin) linking the DAC output to the driver inputs—a junction where subtle power supply ripple and thermal noise can be introduced. The MAX97220 incorporates an internal DirectDrive charge pump (requiring C1P, C1N, and PVSS decoupling capacitors) to generate a \-3.3V supply rail for capacitor-free ground-centered 3.5 mm output, delivering robust drive capability (\~60–70 mW into 32 Ω) at the cost of higher power draw from the USB-C OTG bus.  
* **Ultra-Low Power CODEC Architecture (e.g., HiBy Digital D1):** Utilizes an integrated mobile CODEC (Cirrus Logic CS46L41) engineered specifically for ultra-low power consumption. The device includes a integrated microcontroller/GPIO interface for hardware volume button handling and status LED indication. Dynamic power optimization is achieved via Adaptive Class H supply rails, which continuously track the audio signal envelope to dynamically adjust internal supply voltages, reducing idle power dissipation and heat generation at lower listening levels. An on-chip HID engine handles hardware volume control natively through a 50-step internal digital volume register linked to hardware interrupts and USB HID volume reporting. While delivering a lower overall voltage swing than discrete high-current drivers, it maximizes handheld battery life during extended listening sessions and maintains a silent background noise floor for sensitive IEMs.

### 📊 2\. Architectural & Hardware Comparison

| Feature / Specification | ESS ES9281AC PRO (FC3 Primary) | JCALLY JM6 Pro (Backup) | HiBy Digital D1 (Backup) |
| :---- | :---- | :---- | :---- |
| **Topology** | Single-Chip SoC \+ Amp | Two-Chip (CX31993 \+ MAX97220) | Integrated Mobile CODEC (CS46L41) |
| **Max Output Voltage** | 1.3 Vrms (Single-Ended) | 1.6 Vrms (Single-Ended) | 1.0 Vrms (Single-Ended) |
| **Output Power (@ 32 Ω)** | \~53 mW | \~75 mW | \~30 mW |
| **Signal-to-Noise Ratio (SNR)** | 124 dB | 119 dB | 121 dB |
| **THD+N** | \-112 dB (0.00025%) | \-95 dB (0.0018%) | \-105 dB (0.0005%) |
| **Power Consumption (Idle / Play)** | \~25 mA / \~45 mA | \~35 mA / \~70 mA | \~15 mA / \~28 mA |
| **Native Audio Format Support** | PCM 32-bit/384kHz, DoP/DSD128 | PCM 32-bit/384kHz | PCM 24-bit/96kHz |
| **Hardware Volume Controls** | Yes (Physical buttons \+ ALSA) | Software/ALSA | Software/ALSA |

### ⚡ 3\. Deep Dive: Noise Floor, Voltage Swing & Power Efficiency

* **Noise Floor vs. Current Delivery:** Ultra-sensitive IEMs (e.g., Moondrop Chu II / Chu III, Truthear Hexa) require a near-zero noise floor to eliminate background hiss. The single-chip ESS architecture and CS46L41 CODEC deliver superior signal-to-noise ratios at low output levels. Conversely, the JCALLY JM6 Pro's MAX97220 discrete buffer provides higher peak current delivery, making it ideal for higher-impedance full-sized headphones (32–150 Ω) where current headroom is critical.  
* **Power Efficiency & Battery Impact:** On the RK3326 platform, USB OTG power drain directly impacts device runtime. The HiBy Digital D1 (CS46L41) consumes under 30 mA during active playback, preserving overall battery life. The HiBy FC3 strikes an optimal balance (\~45 mA), whereas the dual-chip JCALLY JM6 Pro draws up to \~70 mA, slightly reducing overall handheld runtime during extended gaming or playback sessions.  
* **Volume Granularity & Gain Staging:** Bypassing software mixers in Piccolo-2go requires precise hardware gain staging via ax-audio-route. The HiBy FC3's physical volume step integration ensures smooth attenuation without reducing bit depth, preventing digital clipping or coarse volume jumps when driving low-impedance IEMs.

### 📐 4\. Schematic & Circuit Architecture Summary

* **Analog Stage Count & Signal Path Complexity:** Single-chip architectures (ESS ES9281AC PRO and Cirrus Logic CS46L41) complete the entire digital-to-analog conversion and headphone amplification within a single silicon die, eliminating external inter-stage trace routing and coupling capacitors. Conversely, dual-chip topologies (JCALLY JM6 Pro with CX31993 \+ MAX97220) split conversion and driving across two discrete ICs, routing analog signals across PCB traces through external coupling passives (Cin) where environmental noise and layout parasitics can impact purity.  
* **Amplifier Topology & Rail Management:**  
  * **ESS ES9281AC PRO (FC3 Primary):** Class G architecture with programmable analog gain and an internal PNEG flying-capacitor charge pump, providing high power efficiency and ultra-low noise floor.  
  * **JCALLY JM6 Pro (Backup):** Fixed-rail Class AB differential amplifier (MAX97220) powered by a DirectDrive charge pump (-3.3V rail), delivering high output current and drive capability at the expense of higher thermal dissipation and power consumption.  
  * **HiBy Digital D1 (Backup):** Envelope-tracking Adaptive Class H architecture (CS46L41) that continuously scales supply rails based on signal volume, offering the lowest power draw and thermal footprint for extended handheld runtime.

### 🎮 5\. Usage & Topology Selection Guidance

* **Retro Gaming & Arcade Emulation (FBNeo):** The primary **HiBy FC3 (1.3 Vrms)** provides low-latency microframe coalescing on Core 0 with pristine transient response for sound effects and chip-tune audio. The **JCALLY JM6 Pro** acts as an ideal backup when driving higher-impedance portable headphones.  
* **Lossless Audiophile Music & Bit-Perfect FLAC\_AX Playback:** For 32-bit PCM and bit-perfect DoP DSD playback, the single-chip **HiBy FC3** delivers maximum fidelity, low distortion, and transparent staging. For maximum battery longevity during background playback, the **HiBy Digital D1** provides ultra-efficient low-power operation.

# 3\. 💾 Dual SanDisk Extreme PRO Storage (64 GB TF1 \+ 128 GB TF2), SD Profiles & SDR50 Guard

Because the RK3326's SDMMC0 (TF1) and SDMMC1 (TF2) controllers operate on UHS-I SDR50 (50 MHz, 3.3V signaling), **random 4K IOPS (A2 / V30 rating)** governs boot speed and media/ROM responsiveness. Keep your original stock card safely stored as a backup and use genuine **SanDisk Extreme PRO (A2 / V30 / U3)** cards:

* **SD Card Profiles (SD\_PROFILE in /boot/ax-2go.conf) & SDR50 Verification (S02piccolo-2go-storage):**  
  * sdr50\_pro *(Default)*: Probes mmc0 (TF1, 256 KB read-ahead) and mmc1 (TF2, 512 KB read-ahead) at boot to verify **SDR50 (50 MHz, 3.3V signaling, \~24 MB/s)**—automatically stripping unstable SDR104 on fixed-3.3V VQMMC boards or locking safe 25 MHz High-Speed mode if CRC tuning fails.  
  * hs\_compat: Forces 25 MHz High-Speed compatibility (128 KB read-ahead) for older or CRC-sensitive cards.  
  * audio\_quiet: Applies conservative 128 KB read-ahead paired with 64 MiB FLAC\_AX RAM preloading on Core 3 so storage I/O stays idle during playback.

| Slot | Recommended Card | Partition Layout & Mount | Purpose & Runtime Guard |
| :---- | :---- | :---- | :---- |
| **TF1 (Primary OS Slot)** | **SanDisk Extreme PRO 64 GB** (A2 / V30) | • Raw 32 KB: idbloader.img (SPL/TPL)• Raw 8 MB: u-boot.itb (ATF BL31)• p1: BOOT (256 MB, FAT32)• p2: ROOTFS (\< 140 MB, 128 KB-block zstd SquashFS)• p3: STORAGE (\~59 GB, auto-expanded ext4) | Holds the immutable read-only OS (immune to sudden power loss), LZ4 kernel, 640×480 Espresso/Copper logo.bmp, ax-2go.conf, /boot/rk3326-g350-linux.dtb (plus bundled DTBs), save states, and local /userdata (noatime,nodiratime,commit=60,data=writeback). |
| **TF2 (Secondary Media Slot)** | **SanDisk Extreme PRO 128 GB** (A2 / V30) | • p1: PICCOLO\_MEDIA (\~119 GB, **exFAT** or **ext4**) | Dedicated portable media library automatically bound to /userdata/{roms,music,bios} on boot (S02piccolo-2go-storage) for FBNeo arcade/Neo Geo ROMs, lossless .flac / DoP albums, and .mod/.xm/.s3m/.it tracker files. |

# 4\. 🕹️ Physical Controls, Espresso TUI & Safe Power Guard (ax-powerd)

* **Zero-Latency Controls (Core 0 IRQ):** Dedicated GPIO lines read the D-pad, quiet face buttons (A/B/X/Y), sloped shoulder triggers (L1/L2, R1/R2), Start, Select, FN (BTN\_TRIGGER\_HAPPY5 / BTN\_MODE), and KEY\_POWER, while the 10-bit SARADC samples dual analog thumbsticks.  
* **Cold Boot into ax\_appliance\_tui (tty1):** Boots directly into the 24-bit Espresso/Copper TUI launcher (\#0B0806 canvas, \#C88A55 copper accent, \#F4EDE4 warm cream text) with live DAC lock status, RAM/ZRAM telemetry, and instant launch into FLAC\_AX or RetroArch \+ FBNeo (640×480 integer-scaled KMS/Panfrost).  
* **Safe Shutdown & Hardware Protection (ax-powerd on Core 3):**  
  * **Graceful Power-Off:** Hold **KEY\_POWER for 1.5 seconds** (or press FN \+ Power). ax-powerd sends SIGUSR1 to release the active USB DAC cleanly, flushes save files, unmounts both SanDisk cards, and powers off the RK817 PMIC without SD corruption.  
  * **Thermal & Brownout Guards:** Automatically downclocks the CPU cluster back to 1.296 GHz (1.30 GHz) if thermal\_zone0 reaches **75°C**, and triggers an emergency clean shutdown if LiPo battery voltage drops to **3280 mV**.

# 5\. 🎬 640x480 Movie Playback & Encoding

Piccolo-2go includes hardware-accelerated video playback support optimized for the 3.5" 640×480 4:3 IPS panel. Videos encoded at 640×480 resolution play seamlessly using FFmpeg / MPV with Direct Rendering and zero-copy KMS/DRM output.

* **Recommended Video Encoding Settings:**  
  * **Video Codec:** H.264 / AVC (Main Profile, Level 3.1) at 640×480 resolution, 60 fps target, CRF 20–22.  
  * **Audio Codec:** FLAC 16-bit / 24-bit or AAC-LC 320 kbps bit-perfect passthrough to ALSA / USB DAC.  
  * **Container:** Matroska (.mkv) or MP4 (.mp4).

# 6\. 🚀 Installation, DTB Calibration & Source Build References

* **\[Installation & First-Boot Setup Guide\](2go\_INSTALLGUIDE.md)** — Flashing sdcard.img onto TF1, formatting TF2 (PICCOLO\_MEDIA), selecting dtb-select aliases (g350, r36s-panel0..4, r36s-panel4-t2, r36s-v21, r36s-plus, r36s-pro, r36s-xx, r36h, r46h), and calibrating 3,200 mAh rk817 battery gauge and panel settings.  
* **\[System Architecture & Fixed-Hardware Topology\](ARCHITECTURE.md)** — Full RK3326 / RK817 chip interconnect diagram, Cortex-A35 NEON/CRC32 compiler flags, RGA2-Lite offload, and v1.1 mixed-signal refinements.  
* **\[Building from Source\](BUILDING.md)** — Out-of-tree Buildroot BR2\_EXTERNAL compilation and CMake DTB/profile staging steps for piccolo\_2go\_rk3326\_defconfig.  
* **\[Technical & Mixed-Signal Engineering Specification\](technical\_engineering.md)** — Deep-dive silicon specification, throughput optimization matrix, and build audit.
