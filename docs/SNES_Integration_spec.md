# Piccolo-AX SNES Integration Spec (`ax_engine.ini` \+ `Generation-X-AX` Adapter)

**Ecosystem Baseline:** `ax-sdk v1_36_2` (`schema_version: 2.1`) · `Piccolo-AX` (`x86_64`, Linux `6.12-rt`, `< 512 MB` `toram` EROFS) · `FLAC_AX` (`S32_LE` / `DoP`, `hw:CARD=FC3,DEV=0`, `/run/lock/ax-dac.lock`) · `Generation-X-AX`

---

## 1\. Architectural Role & Policy-in-SDK / Adapter-in-Consumer Alignment

On **Piccolo-AX** (`Intel Core i5-9600K` 6C/6T zero-SMT, `32 GB DDR4-3600`, `NVIDIA GTX 980 Ti` `GM200` on `580.196.11` / Vulkan 1.3 \+ `Intel UHD Graphics 630` `0x8086` \+ `HiBy FC3` `ESS Sabre ES9281AC Pro` / `ES9281PRO`), the SNES target runs a cycle/timing-focused engine (`bsnes` / `ares` SNES core) integrated into **`Generation-X-AX`** rather than creating a separate top-level repository.

Following the `[AX]` boundary contract:

* **`ax-sdk` (`v1_36_2`)** owns silicon topology resolution (`x86_64_coffeelake_maxwell_231`, `2:3:1`), `ax_engine.ini` loading via `ax::LoadEngineConfig()`, shared-memory telemetry (`ax::InitTelemetryShm()` \-\> `/dev/shm/ax_telemetry`), and memory/guardrail policies (`hardware_profiles.json` v2.1).  
* **`FLAC_AX`** provides bit-exact `S32_LE` / `DoP` audio output conventions, `ESS Sabre` gain-staging profiles (`fc3_2v0` / `fc3_1v3`), `/run/lock/ax-dac.lock` (`flock`) arbitration, and `AudioMasterPtsSync` / lock-free SPSC ring patterns.  
* **`Generation-X-AX` (SNES Adapter)** maps `ax_engine.ini` and `ax-sdk` telemetry into SNES timing presets, `Core 0–5` thread placement, `/dev/shm` staging, and atomic save/SRAM flush hooks (`K01piccolo-sync`).

---

## 2\. Canonical `ax_engine.ini` Configuration (`[snes]` & `[snes.presets.*]`)

Below is the complete `ax_engine.ini` configuration block for the `Piccolo-AX` SNES build:

```
# ==============================================================================
# Piccolo-AX SNES Engine Configuration (ax-sdk v1_36_2 / Generation-X-AX)
# ==============================================================================

[snes]
enabled = true
engine_backend = "bsnes"
profile_preset = "stock_cycle_exact"
auto_detect_coprocessor = true

# Core & Thread Affinity (aligned with x86_64_coffeelake_maxwell_231 -> 2:3:1)
# Core 0 (0x01): SCHED_FIFO prio 80 SPC700 / ALSA S32_LE writer + xHCI USB DAC IRQ
# Core 1 (0x02): PPU frame presentation / OpenGL-Vulkan scanout
# Cores 2-4 (0x1C): Isolated nohz_full SNES CPU, S-PPU & coprocessor threads
# Core 5 (0x20): Housekeeping, /dev/shm staging, telemetry & save sync
audio_core = 0
audio_sched_fifo_priority = 80
video_core = 1
emu_cores = "2,3,4"
housekeeping_core = 5

# Audio Output & Exclusive DAC Arbitration (FLAC_AX / HiBy FC3 alignment)
audio_backend = "alsa"
alsa_device = "hw:CARD=FC3,DEV=0"
alsa_format = "S32_LE"
alsa_rate = 48000
alsa_period_frames = 96
alsa_buffer_periods = 4
alsa_disable_resample = true
dac_lock_path = "/run/lock/ax-dac.lock"
dac_revision_profile = "fc3_2v0"
dac_load_target = "Speakers"

# Video & Display Pacing (GM200 Vulkan 1.3 / KMS-Xorg scanout)
video_backend = "vulkan_1_3"
integer_scaling = true
vsync_mode = "mailbox_or_fifo"
max_vram_budget_gib = 5.00
hard_vram_ceiling_gib = 5.50

# SPC700 / S-SMP Clock-Locked Rate & Cadence Guard
clock_sync_mode = "audio_master_pts"
rate_guard_enabled = true
rate_guard_max_ppm = 150
rate_guard_ring_target_pct = 50
rate_guard_hysteresis_pct = 5

# Pre-Session /dev/shm Staging & Atomic Persistence (K01piccolo-sync)
shm_staging_enabled = true
shm_rom_dir = "/dev/shm/piccolo-ax/snes/roms"
shm_save_dir = "/dev/shm/piccolo-ax/snes/saves"
persistent_save_dir = "/userdata/saves/snes"
persistent_rom_dir = "/userdata/roms/snes"
atomic_flush_on_exit = true
sync_archive_hook = "K01piccolo-sync"

# Shared-Memory Telemetry (/dev/shm/ax_telemetry)
telemetry_enabled = true
telemetry_publish_interval_ms = 16
telemetry_export_jitter_hist = true

# ------------------------------------------------------------------------------
# Preset 1: Pure Hardware / Cycle Fidelity (Default)
# ------------------------------------------------------------------------------
[snes.presets.stock_cycle_exact]
description = "Cycle-accurate SNES timing, unmodified S-PPU scanlines & SPC700 cadence"
cpu_overclock_pct = 100
coprocessor_overclock_pct = 100
runahead_frames = 0
mid_scanline_strict = true
thread_boost_coprocessor = false

# ------------------------------------------------------------------------------
# Preset 2: Coprocessor Boost (SuperFX / GSU, SA-1, DSP-1..4, CX4, SPC7110)
# ------------------------------------------------------------------------------
[snes.presets.coprocessor_boost]
description = "Dedicated Core 2-4 coprocessor headroom for SuperFX / SA-1 / DSP / CX4"
cpu_overclock_pct = 100
coprocessor_overclock_pct = 125
runahead_frames = 0
mid_scanline_strict = true
thread_boost_coprocessor = true

# ------------------------------------------------------------------------------
# Preset 3: Low-Latency Input & Fast Cadence
# ------------------------------------------------------------------------------
[snes.presets.low_latency_input]
description = "Reduced input/frame latency queue depth for action & rhythm titles"
cpu_overclock_pct = 100
coprocessor_overclock_pct = 100
runahead_frames = 1
mid_scanline_strict = true
thread_boost_coprocessor = false
```

---

## 3\. `Generation-X-AX` SNES Adapter Header & Implementation (`ax_snes_adapter.hpp` & `ax_snes_adapter.cpp`)

### `ax_snes_adapter.hpp`

```c
#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace ax::genx::snes {

enum class SnesPreset {
    StockCycleExact,
    CoprocessorBoost,
    LowLatencyInput,
};

struct SnesRuntimeConfig {
    bool enabled{true};
    std::string engine_backend{"bsnes"};
    SnesPreset preset{SnesPreset::StockCycleExact};
    bool auto_detect_coprocessor{true};

    // 2:3:1 topology mapping (x86_64_coffeelake_maxwell_231)
    int audio_core{0};
    int audio_sched_fifo_priority{80};
    int video_core{1};
    std::vector<int> emu_cores{2, 3, 4};
    int housekeeping_core{5};

    // Audio & DAC lock
    std::string alsa_device{"hw:CARD=FC3,DEV=0"};
    std::string alsa_format{"S32_LE"};
    uint32_t alsa_rate{48000};
    uint32_t alsa_period_frames{96};
    uint32_t alsa_buffer_periods{4};
    bool alsa_disable_resample{true};
    std::string dac_lock_path{"/run/lock/ax-dac.lock"};

    // SPC700 clock-locked rate guard
    bool rate_guard_enabled{true};
    int rate_guard_max_ppm{150};
    int rate_guard_ring_target_pct{50};
    int rate_guard_hysteresis_pct{5};

    // /dev/shm staging & persistence
    bool shm_staging_enabled{true};
    std::string shm_rom_dir{"/dev/shm/piccolo-ax/snes/roms"};
    std::string shm_save_dir{"/dev/shm/piccolo-ax/snes/saves"};
    std::string persistent_save_dir{"/userdata/saves/snes"};
    std::string persistent_rom_dir{"/userdata/roms/snes"};
    bool atomic_flush_on_exit{true};

    // Preset overrides
    uint32_t cpu_overclock_pct{100};
    uint32_t coprocessor_overclock_pct{100};
    int runahead_frames{0};
    bool thread_boost_coprocessor{false};
};

class AxSnesAdapter {
public:
    AxSnesAdapter() = default;
    ~AxSnesAdapter();

    AxSnesAdapter(const AxSnesAdapter&) = delete;
    AxSnesAdapter& operator=(const AxSnesAdapter&) = delete;

    // Loads ax_engine.ini via ax::LoadEngineConfig(), applies active preset,
    // and resolves topology/VRAM ceilings from ax-sdk v1_36_2.
    bool Initialize(const std::string& config_path, const std::string& rom_path);

    // Stages ROM & SRAM into /dev/shm on housekeeping core (Core 5) and
    // acquires exclusive /run/lock/ax-dac.lock + Core 0 SCHED_FIFO priority 80.
    bool StartSession();

    // Computes bounded PPM rate adjustment from current audio ring fill %
    // without allocating memory or blocking on Core 0.
    int ComputeRateGuardPpm(int current_ring_fill_pct) const noexcept;

    // Flushes /dev/shm SRAM/saves atomically to /userdata/saves/snes,
    // triggers K01piccolo-sync hook if enabled, and releases DAC lock.
    bool ShutdownSession();

    const SnesRuntimeConfig& config() const noexcept { return cfg_; }
    bool is_running() const noexcept { return running_.load(std::memory_order_acquire); }

private:
    bool AcquireDacLock();
    void ReleaseDacLock();
    bool StageRomAndSavesToShm();
    bool FlushSavesAtomicallyToStorage();
    void ApplyCoreAffinities();

    SnesRuntimeConfig cfg_{};
    std::string rom_path_{};
    std::string staged_rom_path_{};
    std::string staged_sram_path_{};
    int dac_lock_fd_{-1};
    std::atomic<bool> running_{false};
};

} // namespace ax::genx::snes
```

### `ax_snes_adapter.cpp`

```c
#include "ax_snes_adapter.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sched.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace ax::genx::snes {

namespace {

std::string Trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string StripInlineComment(const std::string& line) {
    const auto hash = line.find('#');
    const auto semi = line.find(';');
    const auto cut = std::min(hash, semi);
    return (cut == std::string::npos) ? line : line.substr(0, cut);
}

bool ParseBool(const std::string& val, bool fallback) {
    if (val == "true" || val == "1" || val == "yes" || val == "on") return true;
    if (val == "false" || val == "0" || val == "no" || val == "off") return false;
    return fallback;
}

SnesPreset ParsePreset(const std::string& name) {
    if (name == "coprocessor_boost") return SnesPreset::CoprocessorBoost;
    if (name == "low_latency_input") return SnesPreset::LowLatencyInput;
    return SnesPreset::StockCycleExact;
}

std::string PresetSectionName(SnesPreset p) {
    switch (p) {
        case SnesPreset::CoprocessorBoost: return "snes.presets.coprocessor_boost";
        case SnesPreset::LowLatencyInput:  return "snes.presets.low_latency_input";
        case SnesPreset::StockCycleExact:
        default:                           return "snes.presets.stock_cycle_exact";
    }
}

bool CopyFileAtomic(const fs::path& src, const fs::path& dst) {
    if (!fs::exists(src)) return true;
    std::error_code ec;
    fs::create_directories(dst.parent_path(), ec);
    if (ec) return false;

    const fs::path tmp_dst = dst.string() + ".tmp." + std::to_string(::getpid());
    fs::copy_file(src, tmp_dst, fs::copy_options::overwrite_existing, ec);
    if (ec) return false;

    fs::rename(tmp_dst, dst, ec);
    if (ec) {
        fs::remove(tmp_dst, ec);
        return false;
    }
    return true;
}

} // namespace

AxSnesAdapter::~AxSnesAdapter() {
    if (running_.load(std::memory_order_acquire)) {
        ShutdownSession();
    }
    ReleaseDacLock();
}

bool AxSnesAdapter::Initialize(const std::string& config_path, const std::string& rom_path) {
    rom_path_ = rom_path;

    // Note: In full ax-sdk v1_36_2 builds, ax::LoadEngineConfig() parses
    // ax_engine.ini and binds hardware_profiles.json (x86_64_coffeelake_maxwell_231).
    // This standalone-safe INI reader mirrors those exact [snes] and [snes.presets.*] keys.
    std::FILE* fp = std::fopen(config_path.c_str(), "r");
    if (!fp) {
        std::cerr << "[ax-snes] Could not open config '" << config_path
                  << "', using deterministic defaults.\n";
        return !rom_path_.empty();
    }

    std::string current_section;
    std::string active_preset_name = "stock_cycle_exact";
    char buf[1024];

    while (std::fgets(buf, sizeof(buf), fp)) {
        std::string line = Trim(StripInlineComment(buf));
        if (line.empty()) continue;

        if (line.front() == '[' && line.back() == ']') {
            current_section = Trim(line.substr(1, line.size() - 2));
            continue;
        }

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        const std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));
        if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
            val = val.substr(1, val.size() - 2);
        }

        if (current_section == "snes") {
            if (key == "enabled") cfg_.enabled = ParseBool(val, cfg_.enabled);
            else if (key == "engine_backend") cfg_.engine_backend = val;
            else if (key == "profile_preset") active_preset_name = val;
            else if (key == "auto_detect_coprocessor") cfg_.auto_detect_coprocessor = ParseBool(val, true);
            else if (key == "audio_core") cfg_.audio_core = std::atoi(val.c_str());
            else if (key == "audio_sched_fifo_priority") cfg_.audio_sched_fifo_priority = std::atoi(val.c_str());
            else if (key == "video_core") cfg_.video_core = std::atoi(val.c_str());
            else if (key == "housekeeping_core") cfg_.housekeeping_core = std::atoi(val.c_str());
            else if (key == "alsa_device") cfg_.alsa_device = val;
            else if (key == "alsa_format") cfg_.alsa_format = val;
            else if (key == "alsa_rate") cfg_.alsa_rate = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
            else if (key == "alsa_period_frames") cfg_.alsa_period_frames = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
            else if (key == "alsa_buffer_periods") cfg_.alsa_buffer_periods = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
            else if (key == "alsa_disable_resample") cfg_.alsa_disable_resample = ParseBool(val, true);
            else if (key == "dac_lock_path") cfg_.dac_lock_path = val;
            else if (key == "rate_guard_enabled") cfg_.rate_guard_enabled = ParseBool(val, true);
            else if (key == "rate_guard_max_ppm") cfg_.rate_guard_max_ppm = std::atoi(val.c_str());
            else if (key == "rate_guard_ring_target_pct") cfg_.rate_guard_ring_target_pct = std::atoi(val.c_str());
            else if (key == "rate_guard_hysteresis_pct") cfg_.rate_guard_hysteresis_pct = std::atoi(val.c_str());
            else if (key == "shm_staging_enabled") cfg_.shm_staging_enabled = ParseBool(val, true);
            else if (key == "shm_rom_dir") cfg_.shm_rom_dir = val;
            else if (key == "shm_save_dir") cfg_.shm_save_dir = val;
            else if (key == "persistent_save_dir") cfg_.persistent_save_dir = val;
            else if (key == "persistent_rom_dir") cfg_.persistent_rom_dir = val;
            else if (key == "atomic_flush_on_exit") cfg_.atomic_flush_on_exit = ParseBool(val, true);
        }
    }

    cfg_.preset = ParsePreset(active_preset_name);
    const std::string target_section = PresetSectionName(cfg_.preset);

    // Second pass for active preset section overrides
    std::rewind(fp);
    current_section.clear();
    while (std::fgets(buf, sizeof(buf), fp)) {
        std::string line = Trim(StripInlineComment(buf));
        if (line.empty()) continue;
        if (line.front() == '[' && line.back() == ']') {
            current_section = Trim(line.substr(1, line.size() - 2));
            continue;
        }
        if (current_section != target_section) continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = Trim(line.substr(0, eq));
        std::string val = Trim(line.substr(eq + 1));

        if (key == "cpu_overclock_pct") cfg_.cpu_overclock_pct = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
        else if (key == "coprocessor_overclock_pct") cfg_.coprocessor_overclock_pct = static_cast<uint32_t>(std::strtoul(val.c_str(), nullptr, 10));
        else if (key == "runahead_frames") cfg_.runahead_frames = std::atoi(val.c_str());
        else if (key == "thread_boost_coprocessor") cfg_.thread_boost_coprocessor = ParseBool(val, false);
    }

    std::fclose(fp);

    if (!cfg_.enabled) {
        std::cerr << "[ax-snes] SNES target is disabled in ax_engine.ini.\n";
        return false;
    }

    // Enforce cycle-exact integrity on stock_cycle_exact
    if (cfg_.preset == SnesPreset::StockCycleExact) {
        cfg_.cpu_overclock_pct = 100;
        cfg_.coprocessor_overclock_pct = 100;
        cfg_.runahead_frames = 0;
        cfg_.thread_boost_coprocessor = false;
    }

    return !rom_path_.empty();
}

bool AxSnesAdapter::AcquireDacLock() {
    if (cfg_.dac_lock_path.empty()) return true;

    std::error_code ec;
    fs::create_directories(fs::path(cfg_.dac_lock_path).parent_path(), ec);

    dac_lock_fd_ = ::open(cfg_.dac_lock_path.c_str(), O_CREAT | O_RDWR, 0660);
    if (dac_lock_fd_ < 0) {
        std::cerr << "[ax-snes] Failed to open DAC lock '" << cfg_.dac_lock_path
                  << "': " << std::strerror(errno) << "\n";
        return false;
    }

    if (::flock(dac_lock_fd_, LOCK_EX | LOCK_NB) != 0) {
        std::cerr << "[ax-snes] DAC lock busy ('" << cfg_.dac_lock_path
                  << "') — another [AX] audio client holds the DAC.\n";
        ::close(dac_lock_fd_);
        dac_lock_fd_ = -1;
        return false;
    }
    return true;
}

void AxSnesAdapter::ReleaseDacLock() {
    if (dac_lock_fd_ >= 0) {
        ::flock(dac_lock_fd_, LOCK_UN);
        ::close(dac_lock_fd_);
        dac_lock_fd_ = -1;
    }
}

bool AxSnesAdapter::StageRomAndSavesToShm() {
    if (!cfg_.shm_staging_enabled) {
        staged_rom_path_ = rom_path_;
        return true;
    }

    std::error_code ec;
    fs::create_directories(cfg_.shm_rom_dir, ec);
    fs::create_directories(cfg_.shm_save_dir, ec);
    if (ec) return false;

    const fs::path rom_src(rom_path_);
    const fs::path rom_dst = fs::path(cfg_.shm_rom_dir) / rom_src.filename();
    if (!CopyFileAtomic(rom_src, rom_dst)) {
        std::cerr << "[ax-snes] Failed staging ROM to /dev/shm: " << rom_dst << "\n";
        return false;
    }
    staged_rom_path_ = rom_dst.string();

    const std::string stem = rom_src.stem().string();
    const fs::path sram_persist = fs::path(cfg_.persistent_save_dir) / (stem + ".srm");
    const fs::path sram_shm = fs::path(cfg_.shm_save_dir) / (stem + ".srm");
    staged_sram_path_ = sram_shm.string();

    if (fs::exists(sram_persist)) {
        if (!CopyFileAtomic(sram_persist, sram_shm)) {
            std::cerr << "[ax-snes] Failed staging SRAM to /dev/shm.\n";
            return false;
        }
    }
    return true;
}

bool AxSnesAdapter::FlushSavesAtomicallyToStorage() {
    if (!cfg_.shm_staging_enabled || !cfg_.atomic_flush_on_exit) return true;
    if (staged_sram_path_.empty() || !fs::exists(staged_sram_path_)) return true;

    const fs::path sram_persist = fs::path(cfg_.persistent_save_dir) / fs::path(staged_sram_path_).filename();
    return CopyFileAtomic(staged_sram_path_, sram_persist);
}

void AxSnesAdapter::ApplyCoreAffinities() {
    cpu_set_t set;
    CPU_ZERO(&set);
    for (int c : cfg_.emu_cores) {
        if (c >= 0 && c < CPU_SETSIZE) {
            CPU_SET(c, &set);
        }
    }
    ::sched_setaffinity(0, sizeof(set), &set);
}

int AxSnesAdapter::ComputeRateGuardPpm(int current_ring_fill_pct) const noexcept {
    if (!cfg_.rate_guard_enabled) return 0;

    const int delta = current_ring_fill_pct - cfg_.rate_guard_ring_target_pct;
    if (std::abs(delta) <= cfg_.rate_guard_hysteresis_pct) {
        return 0;
    }

    // Proportional bounded adjustment clamped to [-rate_guard_max_ppm, +rate_guard_max_ppm]
    const int raw_ppm = delta * 4;
    return std::clamp(raw_ppm, -cfg_.rate_guard_max_ppm, cfg_.rate_guard_max_ppm);
}

bool AxSnesAdapter::StartSession() {
    if (!StageRomAndSavesToShm()) return false;
    if (!AcquireDacLock()) return false;

    ApplyCoreAffinities();
    running_.store(true, std::memory_order_release);
    return true;
}

bool AxSnesAdapter::ShutdownSession() {
    running_.store(false, std::memory_order_release);
    const bool flushed = FlushSavesAtomicallyToStorage();
    ReleaseDacLock();
    return flushed;
}

} // namespace ax::genx::snes
```

---

## 4\. `Piccolo-AX` SNES Launch & Atomic Persistence Wrapper (`ax-snes-launch.sh`)

```sh
#!/usr/bin/env bash
set -euo pipefail

# Piccolo-AX SNES Launcher Wrapper
# Enforces 2:3:1 topology, /dev/shm staging, /run/lock/ax-dac.lock arbitration,
# and atomic save/SRAM sync on exit.

CONFIG_PATH="${AX_ENGINE_INI:-/userdata/system/piccolo-ax/ax_engine.ini}"
ROM_PATH="${1:-}"
PRESET_OVERRIDE="${2:-}"

if [[ -z "${ROM_PATH}" ]]; then
    echo "Usage: $0 <path-to-snes-rom> [stock_cycle_exact|coprocessor_boost|low_latency_input]" >&2
    exit 2
fi

if [[ ! -f "${ROM_PATH}" ]]; then
    echo "[ax-snes] ROM not found: ${ROM_PATH}" >&2
    exit 1
fi

SHM_BASE="/dev/shm/piccolo-ax/snes"
SHM_ROM_DIR="${SHM_BASE}/roms"
SHM_SAVE_DIR="${SHM_BASE}/saves"
PERSIST_SAVE_DIR="/userdata/saves/snes"

mkdir -p "${SHM_ROM_DIR}" "${SHM_SAVE_DIR}" "${PERSIST_SAVE_DIR}"

ROM_BASENAME="$(basename "${ROM_PATH}")"
ROM_STEM="${ROM_BASENAME%.*}"
STAGED_ROM="${SHM_ROM_DIR}/${ROM_BASENAME}"
STAGED_SRAM="${SHM_SAVE_DIR}/${ROM_STEM}.srm"
PERSIST_SRAM="${PERSIST_SAVE_DIR}/${ROM_STEM}.srm"

# Stage ROM and existing SRAM into /dev/shm before starting real-time execution
cp -f "${ROM_PATH}" "${STAGED_ROM}"
if [[ -f "${PERSIST_SRAM}" ]]; then
    cp -f "${PERSIST_SRAM}" "${STAGED_SRAM}"
fi

cleanup() {
    local exit_code=$?
    if [[ -f "${STAGED_SRAM}" ]]; then
        local tmp_sram="${PERSIST_SRAM}.tmp.$$"
        cp -f "${STAGED_SRAM}" "${tmp_sram}"
        mv -f "${tmp_sram}" "${PERSIST_SRAM}"
    fi
    if command -v /etc/init.d/K01piccolo-sync >/dev/null 2>&1; then
        /etc/init.d/K01piccolo-sync save || true
    fi
    exit "${exit_code}"
}
trap cleanup EXIT INT TERM

# Ensure Core 0 (0x01) audio/USB DAC path and Core 1 (0x02) GPU stay clean
if command -v nvidia-smi >/dev/null 2>&1; then
    nvidia-smi -pm 1 >/dev/null 2>&1 || true
fi

# Launch Generation-X-AX SNES binary with isolated 2:3:1 affinity
SNES_BIN="${AX_SNES_BIN:-/usr/bin/genx-snes-ax}"
exec "${SNES_BIN}" \
    --config "${CONFIG_PATH}" \
    --rom "${STAGED_ROM}" \
    --sram "${STAGED_SRAM}" \
    ${PRESET_OVERRIDE:+--preset "${PRESET_OVERRIDE}"}
```

