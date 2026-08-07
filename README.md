


# CM4 Kernel Panic Trace Capture & PMIC Reset Workaround

## 📌 Project Overview
This document outlines the procedure for configuring an **ARM64 Linux Kernel (6.18+)** on a **Raspberry Pi Compute Module 4 (CM4)** to route kernel panic stack traces directly to a serial terminal session (`picocom`) via **UART5 (`/dev/ttyAMA5`)**, alongside post-mortem hardware stabilization findings.

---

## 🛠️ Configuration Steps

### 1. Enable UART5 Overlay
In `/boot/firmware/config.txt`, enable UART5 on GPIO 12/13:

```ini
# Enable UART5 (TX=GPIO12, RX=GPIO13)
dtoverlay=uart5
```





Markdown
### Configure Kernel Command Line Parameters

Update `/boot/firmware/cmdline.txt` (must remain a **single continuous line**) to redirect primary console logging to `ttyAMA5` and prevent standard kernel auto-reboots:

```text
console=ttyAMA5,115200 console=tty1 loglevel=8 sysrq_always_enabled=1 panic=-1 oops=panic bcm2835_wdt.no_reboot=1 bcm2835_wdt.nowayout=0 nmi_watchdog=0 softlockup=0 hung_task_panic=0 nr_cpus=1

console=ttyAMA5,115200: Directs high-priority kernel printk and oops traces out through UART5.
loglevel=8: Forces all debug and panic messages to serial.
panic=-1 & oops=panic: Freezes the kernel immediately on a fault to retain call stacks.

```
# Verifying Crash Trace Capture
Trigger a controlled kernel panic from the host terminal using sysrq:

```Bash
echo c | sudo tee /proc/sysrq-trigger
Captured Output (picocom)
The kernel successfully outputs the full call stack trace to the picocom session before halting:

[  240.410029] sysrq: Trigger a crash
[  240.413793] Kernel panic - not syncing: sysrq triggered crash
[  240.419784] CPU: 0 UID: 0 PID: 812 Comm: tee Tainted: G         C          6.18.34+rpt-rpi-v8 #1
[  240.440921] Call trace:
[  240.443362]  show_stack+0x20/0x38 (C)
[  240.447036]  dump_stack_lvl+0x60/0x80
[  240.457156]  panic+0x68/0x70
[  240.460034]  sysrq_handle_crash+0x24/0x38
[  240.526790] ---[ end Kernel panic - not syncing: sysrq triggered crash ]---
⚠️ Known Issue: Transient PMIC / Hard Reset

```

## Problem Description
Despite soft-watchdog disable parameters (panic=-1, bcm2835_wdt.no_reboot=1), the system executes a hard hardware reset immediately after displaying the panic end-marker:

```
[  240.526790] ---[ end Kernel panic - not syncing: sysrq triggered crash ]---
[    0.000000] Booting Linux on physical CPU 0x0000000000 [0x410fd083]

```

## Previous Workaround (Deprecated / Unstable)
Setting fixed clock frequencies in /boot/firmware/config.txt temporarily stabilized power states on some boots, but ultimately proved unreliable over extended validation runs:

## DEPRECATED: Does not resolve long-term PMIC brownout/reset issues
```
force_turbo=1
arm_freq=1500
```
## Suspected Root Causes

PMIC Hardware Watchdog / Power Rail Trip: The onboard MXL7704 PMIC hardware monitor or DVFS dynamic voltage step drops below threshold during CPU state change, triggering a hardware power reset.

Signal Integrity / Ground Bounce: Potential parasitic backfeeding or line noise over external USB-UART cable connections (GPIO 12/13) tripping PMIC protection during high-transient events.

## Next Steps
Transition to SWD/JTAG Debugging: Attach a hardware debugger (e.g., Raspberry Pi Debug Probe) directly to the CM4 IO board SWD header (SWCLK, SWDIO, GND).

Hardware Freeze: Use OpenOCD/GDB to halt the ARM Cortex-A72 cores at the hardware level during an exception, bypassing OS-level PMIC reset triggers and allowing full register inspection



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
