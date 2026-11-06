<div align="center">
  <img src="/resources/images/gwv1.png" alt="gpiodWrapV1 Logo" width="260">
</div>
<br>


A highly efficient, lightweight, and thread-safe C++17 wrapper library for **libgpiod V1** (recommended for v1.6.0 and higher).

This project was specifically engineered for maximum performance on legacy embedded hardware (such as the **Raspberry Pi 1 / ARMv6** running Debian Bookworm). It eliminates unnecessary runtime overhead—such as map lookups—and utilizes atomic, lock-free CPU operations for robust signal and resource management.

---
## 🚀 Features
✔️ **Header-Only Interface:** Minimalistic architecture for effortless integration into your CMake projects.  
✔️ **Combined Pin Modes:** No tedious flag management—logical direction and physical pull resistors are configured in a single, intuitive step.  
✔️ **Kernel-Level Debouncing:** Utilizes native Linux hardware/kernel-level debouncing via the libgpiod C-API.  
✔️ **Safe-by-Default:** All requested lines are automatically reverted to a safe, high-impedance INPUT state.  
✔️ **GPIO Wrapper:** Works with libgpiod >=1.6.0

---
## ✨ System Requirements
- Linux kernel with an active GPIO character device subsystem.
- `libgpiod-dev` >= 1.6.0 (fully tested with v1.6.3 on Raspberry Pi OS Bookworm).
- C++17 compatible compiler (e.g., `g++`).
- CMake >= 3.16.

---
# 🧩 Provided Functions

This wrapper provides easy functions like:

<br>
<table width="100%">
  <thead>
    <tr>
      <th align="left" width="450">Basic configuration</th>
      <th align="left" width="550">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>gpiodWrapV1 gpio</code></td>
      <td>Automatic search chip number.</td>
    </tr>
    <tr>
      <td><code>gpiodWrapV1 gpio(chipnum)</code></td>
      <td>Specific chip number.</td>
    </tr>
  </tbody>
</table>
<br><br>
<table width="100%">
  <thead>
    <tr>
      <th align="left" width="450">Basic function</th>
      <th align="left" width="550">Description</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td><code>openChip(chipnum)</code></td>
      <td>Open specifically.</td>
    </tr>
    <tr>
      <td><code>closeChip()</code></td>
      <td>Returns outputs to a safe state, releases resources.</td>
    </tr>
    <tr>
      <td><code>configurePin(pin, PinMode, Debounce, PinPolarity, Consumer)</code></td>
      <td>Configured the line.</td>
    </tr>
    <tr>
      <td><code>setPin(pin, HIGH/LOW)</code></td>
      <td>Sets output configured line to high or low level.</td>
    </tr>
    <tr>
      <td><code>getPin(pin)</code></td>
      <td>Reads the intput configured line and return Pinvalue HIGH/LOW/ERROR.</td>
    </tr>
    <tr>
      <td><code>getPinEvent(pin)</code></td>
      <td>Reads the intput configured line and return Pinvalue HIGH/LOW/ERROR at every level change.</td>
    </tr>
  </tbody>
</table>
<br>

---
## 🔧 Installation

```bash
sudo apt update
sudo apt install libgpiod-dev pkg-config cmake build-essential
```

---
## 📦 Build System & Compiling Examples

The project relies entirely on CMake. All executable examples located in the `examples/` directory are automatically deployed to a central `bin/` directory in the project root upon building.

```bash
mkdir build && cd build
cmake ..
make -j\$(nproc)
```

---
## ⚡ API Reference

### Pin Modes (`PinMode`)
- `INPUT` / `BIAS_DISABLE`: High-impedance input mode without pull-resistors (floating).
- `PULLUP`: Input mode with an activated internal pull-up resistor (ideal for active-low buttons switching to GND).
- `PULLDOWN`: Input mode with an activated internal pull-down resistor.
- `OUTPUT`: Standard push-pull output mode (actively drives HIGH and LOW).
- `OUTPUT_PULLUP`: Output mode with an active internal pull-up resistor.
- `OPEN_DRAIN`: Open-drain output mode (actively pulls to GND, becomes high-impedance for HIGH states).
- `OPEN_SOURCE`: Open-source output mode (actively drives HIGH, becomes high-impedance for LOW states).

---
## 🚨 Hardware Safety & Signal Handling (Safe-by-Default)

Uncontrolled program termination (e.g., via `Ctrl + C` / `SIGINT or SIGTERM`) can be hazardous in hardware control applications if outputs connected to motors, relays, or heating elements remain active in an undefined state.

`gpiodWrapV1` addresses this by adhering to the **Safe-by-Default** principle:

* **Automatic Fail-Safe:**

    Upon receiving `SIGINT or SIGTERM`, all active output pins are automatically reverted to a safe, high-impedance input state (`INPUT`) before the GPIO chip is released.

* **Decoupled Watchdog Thread:**

    The POSIX signal handler exclusively modifies atomic flags (100% `async-signal-safe`). Hardware cleanup is executed asynchronously in a separate watchdog thread—completely free from C library locks or deadlock risks.

---
## 🛠️ Disabling Signal Handling
If your application logic manages OS signals independently, the built-in signal interception mechanism and its watchdog thread can be disabled entirely with zero runtime overhead via CMake:

```bash
cmake -DGPIODWRAPV1_DISABLE_SIGNALS=ON ..
```
In C++ Source Code:
```
#define GPIODWRAP_NO_SIGNALS
#include "gpiodWrap.hpp"
```

---
## Quick Code Example (`switch_led`)

```cpp
#include <iostream>
#include <chrono>
#include <thread>

#include "gpiodWrapV1.hpp"

gpiodWrapV1 gpio;

constexpr int LED_PIN = 17;
constexpr int BUTTON_PIN = 27;

int main() {
    // Configure Pin 27 as an input with an active pull-up and 30ms hrtimer or hardware debouncing
    gpio.configurePin(BUTTON_PIN, PinMode::PULLUP, 30);
    
    // Configure Pin 17 as a standard push-pull output
    gpio.configurePin(LED_PIN, PinMode::OUTPUT);

    bool led_state = false;

    while(true) {
        // Fetch the pin state only when a physical level transition occurs
        auto change_event = gpio.getPinChange(BUTTON_PIN);

        if (change_event == PinValue::LOW) { // Button pressed (switched to GND)
            led_state = !led_state;
            gpio.setPin(LED_PIN, led_state ? PinValue::HIGH : PinValue::LOW);
            std::cout << "LED toggled: " << (led_state ? "ON" : "OFF") << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }
    return 0;
}
```

---
## 📄 License

Copyright (C) 2026 Raspino-Projekt <dev@raspino.org>  
This API is proudly released under the terms of the **GNU General Public License v3.0**.  
You are free to use, modify, and distribute this project under the terms of the GPLv3.

---
## 🤝 Contributions

Pull requests and improvements are welcome.  
Feel free to fork, enhance, or suggest features.

---
⭐ If this wrapper helps your project — consider starring it on GitHub!


