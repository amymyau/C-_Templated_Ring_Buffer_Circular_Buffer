


## 🛠️ Root Cause Analysis & Resolution: CM4 Hardware Reset During KDB Traps

### 📌 Summary & Problem Statement
While executing a high-stress `ring_buffer` workload pinned to **CPU 2** on a **Raspberry Pi Compute Module 4 (Rev 1.1)**, entering interactive KDB via SysRq-G (`echo g > /proc/sysrq-trigger`) caused an immediate system hard-reset over UART (`ttyAMA5`).

* **Symptom:** Terminal input froze instantly upon entering `[0]kdb>`, followed by a full SoC boot sequence within seconds.
* **Failed Mitigations:** Disabling software watchdogs via kernel cmdline (`nowatchdog`, `modprobe.blacklist=bcm2835_wdt`) and firmware overlays (`dtparam=watchdog=off`) failed to stop the resets.

---

### 🔬 Technical Root Cause

The unexpected reboots were caused by **hardware-level power management thresholds**, not software watchdog timers:

1. **PMIC I^2C Keep-Alive Stall:** Halting execution via KDB freezes all kernel/firmware tasks managing the MxL7704 PMIC via I^2C. The PMIC interprets the missing keep-alive signal as a system fault, drops the `GLOBAL_EN` line, and cuts V_DD_CORE.
2. **DVFS $dI/dt$ Voltage Droop:** Halting CPU 2 abruptly while running heavy ring-buffer load causes a high dI/dt transient on V_CORE. Without fixed DVFS parameters, V_CORE dips below the PMIC Power-Good threshold, triggering a Power-On Reset (POR).

---

### 💡 Validation Solution

Instead of pausing execution in KDB (which trips the PMIC $I^2C$ timeout), the system was reconfigured to **lock processor power states** and execute an **atomic backtrace dump to UART via SysRq-C panic** in a single pass.

<details>
<summary><b>🔍 View Configuration Changes</b></summary>

#### 1. Power & DVFS Stabilization (`/boot/firmware/config.txt`)
Fixed CPU frequency and voltage to eliminate dynamic power gating and supply rail transients:
```ini
# Prevent PMIC voltage droop and maintain stable power rails
force_turbo=1
arm_freq=1500
```


# CM4 Ringbuffer & Kernel Debugging Environment

This repository contains the C++ Ringbuffer workload generator (`workload_gen_gpio_v2_PCIE.cpp`) and the associated Linux kernel debugging infrastructure for the Raspberry Pi Compute Module 4 (CM4).

## 1. Kernel Symbol Setup (`vmlinux`)

When cross-debugging Linux kernel panics or driver issues via GDB, distribution-provided debugging packages (`-dbg`) can omit full DWARF section tables or use stub symbols. To resolve source-level symbols (line numbers, function arguments, and stack frames), we cross-compile an unstripped, full-DWARF `vmlinux` binary directly from source.

### Host Prerequisites (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
                    bison flex libssl-dev libncurses-dev bc kmod cpio rsync
Kernel Build Procedure (6.18.y)
Bash
# 1. Clone Raspberry Pi Linux source
git clone --depth 1 --branch rpi-6.18.y [https://github.com/raspberrypi/linux.git](https://github.com/raspberrypi/linux.git) rpi-linux
cd rpi-linux

# 2. Configure for BCM2711 (ARM64)
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
make bcm2711_defconfig

# 3. Explicitly enable full DWARF debug symbols
scripts/config --enable CONFIG_DEBUG_INFO
scripts/config --enable CONFIG_DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT
scripts/config --disable CONFIG_DEBUG_INFO_REDUCED
make olddefconfig

# 4. Compile raw unstripped vmlinux
make -j$(nproc) vmlinux
Output: vmlinux (~325 MB) containing full .debug_info, .debug_line, and .debug_frame sections.

Host GDB Verification
Bash
gdb-multiarch ~/rpi-linux/vmlinux
Code snippet
(gdb) info functions panic
(gdb) list start_kernel

```
Markdown
# C++ Templated Ring Buffer (Circular Buffer)

A lightweight, header-only, templated circular buffer implementation written in modern C++17. Designed for predictable FIFO streaming, low overhead, and safe memory management on embedded targets like the **Raspberry Pi Compute Module 4 (CM4)**.

---

## Key Features

* **C++17 Type Safety:** Uses `std::optional<T>` for error-free dequeueing without relying on exceptions or magic dummy values on empty buffer states.
* **Header-Only & Templated:** Easily supports arbitrary payload types (`int`, structs, raw byte buffers, floats) without heap allocation overhead after initialization.
* **Bounds-Checked Protection:**
  * Rejects enqueue operations when full (`enqueue` returns `false`).
  * Rejects dequeue operations when empty (`dequeue` returns `std::nullopt`).
* **Zero Dynamic Reallocation:** Allocates underlying vector storage once at construction time to ensure bounded memory usage and deterministic performance during runtime.

---

## API Overview

```cpp
template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t size);           // Allocates buffer capacity
    bool enqueue(const T& item);                // Pushes item; returns false if full
    std::optional<T> dequeue();                 // Pops oldest item; returns std::nullopt if empty
    
    [[nodiscard]] size_t size() const;          // Returns current stored element count
    [[nodiscard]] bool empty() const;           // Returns true if count == 0
    [[nodiscard]] bool full() const;            // Returns true if count == capacity
};
```

---

## Hardware Execution & Build Instructions

Tested and executed on **Raspberry Pi CM4** running Linux.

### 1. Compile
Compile using `g++` or `clang++` with standard C++17 support enabled:

```bash
g++ -std=c++17 -O2 main.cpp -o ring_buffer
```

### 2. Run
```bash
./ring_buffer
```

---

## Verification Output

Below is the verified execution trace confirming modulo wrap-around logic and strict overflow/underflow bounds handling:

```text
--- Enqueueing Items ---
Enqueued: 10
Enqueued: 20
Enqueued: 30
Enqueued: 40
Enqueued: 50
Enqueue 60 failed: Buffer is FULL!

--- Dequeueing Items ---
Dequeued: 10
Dequeued: 20
Dequeued: 30
Dequeued: 40
Dequeued: 50
Dequeue failed: Buffer is EMPTY!
```
