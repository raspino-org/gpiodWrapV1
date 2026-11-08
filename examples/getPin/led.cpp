/**
 * @file getPin example led.cpp
 *
 *
 * @author Kay Donau
 * @date 11.2026
 *
 * @github https://github.com/raspino-org/gpioWrapV1
 *
 * gpiodWrapV1 is a part of the Raspino-Project <https://raspino.org>
 * @copyright Copyright (c) 2025-2026 Raspino-Projekt <dev@raspino.org>
 * @license GNU General Public License v3.0 (GPL-3.0-or-later)
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://gnu.org>.
 * 
 * Raspino (TM) is a trademark of the Raspino-Project and belongs to the community.
 * https://github.com/raspino-org
 */

#include <iostream>
#include <chrono>
#include <thread>

#include "gpiodWrapV1.hpp"

gpiodWrapV1 gpio;

constexpr int led = 17;
constexpr int button = 27;

int main() {
    using namespace gpiodwrapv1;

    gpio.configurePin(button, PULLUP, 30);
    gpio.configurePin(led, OUTPUT);

    while(true) {

        auto line_event = gpio.getPin(button);

        if (line_event == ERROR) {
            std::cerr << "Line read failed!\n";
        }
        else if (line_event == HIGH) {
            gpio.setPin(led, LOW);
        }
        else if (line_event == LOW) {
            gpio.setPin(led, HIGH);
        }


        std::this_thread::sleep_for(std::chrono::milliseconds{200});

    }
}