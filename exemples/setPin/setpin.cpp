#include <iostream>
#include <chrono>
#include <thread>

#include "gpiodWrapV1.hpp"

gpiodWrapV1 gpio;

constexpr int led = 17;

int main() {
    using namespace gpiodwrapv1;

    gpio.configurePin(led, OUTPUT);

    while(true) {

        gpio.setPin(led, HIGH);
        std::this_thread::sleep_for(std::chrono::milliseconds{1000});

        gpio.setPin(led, LOW);
        std::this_thread::sleep_for(std::chrono::milliseconds{1000});

    }
}