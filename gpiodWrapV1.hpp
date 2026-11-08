/**
 * @file gpiodWrapV1.hpp
 * @brief Lightweight C++ wrapper for libgpiod GPIO access.
 *
 * Simplifies GPIO input/output handling on Linux systems using libgpiod.
 *
 * Requires:
 *  - libgpiod (version 1.6.3 recommended)
 *
 * @author Kay Donau
 * @gpiodWrapV1
 * @version 0.1.2
 * @date 08.11.2026
 *
 * @github https://github.com/raspino-org/gpiodWrapV1
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


#ifdef NO_SIGNAL_HANDLING
    constexpr bool EnableSignalHandling = false;
#else
    constexpr bool EnableSignalHandling = true;
#endif


#pragma once

#include <gpiod.h>

#include <mutex>
#include <array>
#include <memory>
#include <atomic>
#include <string>
#include <csignal>
#include <cstring>
#include <optional>
#include <filesystem>


class gpiodWrapV1 {

public:

    enum class PinValue : int8_t {
        ERROR   = -1,
        LOW     = 0,
        HIGH    = 1
    };

    enum class PinLevel : uint8_t {
        IS_INACTIVE = 0,
        IS_ACTIVE = 1
    };

    enum class PinMode : uint8_t {
        AS_IS = 0,
        INPUT,
        PULLUP,
        PULLDOWN,
        BIAS_DISABLE,

        OUTPUT,
        PUSH_PULL,
        OUTPUT_PULLUP,
        OPEN_DRAIN,
        OPEN_SOURCE
    };

    enum class PinDirection : uint8_t {
        AS_IS = 0,
        INPUT,
        OUTPUT
    };

    enum class PinBias : uint8_t {
        AS_IS = 0,
        DISABLE,
        PULLUP,
        PULLDOWN
    };

    enum class PinDrive : uint8_t {
        PUSH_PULL = 0,
        OPEN_DRAIN,
        OPEN_SOURCE
    };

    enum class PinPolarity : uint8_t {
        ACTIVE_LOW = 0,
        ACTIVE_HIGH
    };

    enum class PinEdge : uint8_t {
        EDGE_NONE = 0,
        AS_IS,
        RISING,
        FALLING,
        BOTH
    };

    enum class PinEdgeLevel : uint8_t {
       NO_EVENTS = 0,
       IS_RISING,
       IS_FALLING
    };

    enum class EventClock : uint8_t {
        MONOTONIC = 0,
        REALTIME,
        HTE
    };

    enum class TimeUnit : uint8_t {
        MILLIS = 0,
        MICROS,
        NANOS
    };



    struct alignas(32) PinSettings {
        const char *consumer = "gpiodWrap";
        uint64_t event_wait_timeout{50000000ULL};
        uint32_t event_buffer_size{32};
        uint16_t debounce{0};

        PinMode      mode     = PinMode::AS_IS;
        PinDirection dir      = PinDirection::INPUT;
        PinBias      bias     = PinBias::DISABLE;
        PinDrive     drive    = PinDrive::PUSH_PULL;
        PinPolarity  polarity = PinPolarity::ACTIVE_HIGH;
        PinEdge      edge     = PinEdge::EDGE_NONE;
        TimeUnit     unit     = TimeUnit::MILLIS;
        EventClock   clock    = EventClock::MONOTONIC;
    };


    struct PinFlags {
        enum State    : uint16_t { START = 0, STOP = 1, PAUSE = 2 };
        enum RunMode  : uint16_t { LOOP = 0, RUN  = 1 };
        enum Unit     : uint16_t { MILLIS = 0, MICROS = 1 };
        enum Invert   : uint16_t { DIRECT = 0, INVERT = 1 };

        enum SpinMode    : uint16_t { OFF = 0, YIELD = 1, NOP = 2, BUSY = 3};
        enum SpinLatency : uint16_t { AUTO = 0, AGGRESSIVE = 1, REALTIME = 2 };
        enum RTPriority  : uint16_t { NONE = 0, LOW = 1, MEDIUM = 2,HIGH = 3};

        State         state       : 2;
        RunMode       run_mode    : 1;
        Unit          unit        : 1;
        Invert        invert      : 1;
        SpinMode      spin_mode   : 2;
        SpinLatency   latency     : 2;
        RTPriority    priority    : 2;
        uint16_t      idle_offset : 5;

        PinFlags() :
            state(START),
            run_mode(LOOP),
            unit(MILLIS),
            invert(DIRECT),
            spin_mode(OFF),
            latency(AUTO),
            priority(NONE),
            idle_offset(OFFSET_NONE)
            {}

        // Recommendations for the IDLE latency offset in microsecond mode
        // Verified values Pi1, Pi3, Pi5
        static constexpr uint8_t OFFSET_NONE = 0;

        static constexpr uint8_t OFFSET_CM4 = 10;
        static constexpr uint8_t OFFSET_CM3 = 15;
        static constexpr uint8_t OFFSET_CM2 = 20;
        static constexpr uint8_t OFFSET_CM1 = 25;

        static constexpr uint8_t OFFSET_PI5 = 6;
        static constexpr uint8_t OFFSET_PI4 = 12;
        static constexpr uint8_t OFFSET_PI3 = 14;
        static constexpr uint8_t OFFSET_PI2 = 19;
        static constexpr uint8_t OFFSET_PI1 = 24;

        static constexpr uint8_t OFFSET_PI500 = 7;
        static constexpr uint8_t OFFSET_PI400 = 11;

        static constexpr uint8_t OFFSET_ZERO2 = 16;
        static constexpr uint8_t OFFSET_ZERO1 = 26;
    };


    static constexpr std::size_t MaxEntries{30};



    explicit gpiodWrapV1(int chipNum = -1) {

       if constexpr (EnableSignalHandling) watchdog();
       openChip(chipNum);

    }

    ~gpiodWrapV1() {

        if constexpr (EnableSignalHandling) {
            watchdog_running.store(false, std::memory_order_relaxed);
            if (watchdog_thread.joinable()) watchdog_thread.join();
        }

        closeChip();

    }



public:

    /**
    * @brief Open gpio chip by number.
    * @param Integer chip number, mostly 0.
    * @param Without parameters, the corresponding chip is opened.
    *
    * @return 0 on success, -1 on failure.
    */
    int openChip(int num = -1) {

        if (chip) closeChip();

        std::string path = (num == -1) ? findChip() : "/dev/gpiochip" + std::to_string(num);
        chip = gpiod_chip_open(path.c_str());

        chip_closing.store(false, std::memory_order_relaxed);

        return 0;
    }



    /**
    * @brief Close gpio Chip, brings the line to a safe state and releases all resources.
    * @note  The built-in signal handler is recommended.
    */
    void closeChip() {

        bool expected = false;

        if (!chip_closing.compare_exchange_strong(expected, true)) return;

        for (uint8_t line = 0; line < MaxEntries; ++line) {

            {
                std::scoped_lock lock(static_line_data[line].mtx);
                auto& data = static_line_data[line];
                if (data.request != nullptr) {
                    safeStateLine(data.request);
                    gpiod_line_release(data.request);
                    data.request = nullptr;
                }
            }
        }

        if (chip) {
            gpiod_chip_close(chip);
            chip = nullptr;
        }

        chip_closing.store(false, std::memory_order_relaxed);
    }



    /**
    * @brief  Configures and requests a line.
    * @param  GPIO line number (offset).
    * @param  PinMode combines the logical direction (input/output) with the 
    *         physical hardware configurations (pull resistors and output drivers) 
    *         of the libgpiod interface into distinct combinations.
    * @param  Debounce sets the kernel debounce time.
    * @param  PinPolarity can be used to adjust the polarity ACTIVE_HIGH/ACTIVE_LOW.
    * @param  Consumer line entity.
    *
    * @return 0 on success, -1 on error.
    */
    int configurePin(unsigned int pin,
                      PinMode mode,
                      uint16_t debounce = 0,
                      PinPolarity polarity = PinPolarity::ACTIVE_HIGH,
                      const char *consumer = "gpiodWrap")
    {

        if (!validRequest(pin)) return -1;

        PinSettings settings;
        settings.consumer    = consumer;
        settings.debounce    = debounce;
        settings.polarity    = polarity;
        settings.edge        = PinEdge::EDGE_NONE;
        settings.mode        = mode;

        int8_t res = lineRequest(pin, settings);
        if (res != 0) return -1;

        return 0;
    }



    /**
    * @brief  Configures and requests a event line.
    * @param  GPIO line number (offset).
    * @param  PinEdge determines the edge of the event.
    * @param  PinBias sets the internal pull resistors.
    * @param  Debounce sets the kernel debounce time.
    * @param  PinPolarity can be used to adjust the polarity ACTIVE_HIGH/ACTIVE_LOW.
    * @param  Consumer line entity.
    *
    * @return 0 on success, -1 on error.
    */
    int configurePin(unsigned int pin,
                      PinEdge edge,
                      PinBias bias = PinBias::DISABLE,
                      uint16_t debounce = 0,
                      PinPolarity polarity = PinPolarity::ACTIVE_HIGH,
                      const char *consumer = "gpiodWrap")
    {

        if (!validRequest(pin)) return -1;

        PinSettings settings;
        settings.consumer    = consumer;
        settings.debounce    = debounce;
        settings.polarity    = polarity;
        settings.dir         = PinDirection::INPUT;
        settings.edge        = edge;
        settings.bias        = bias;
        settings.mode        = PinMode::AS_IS;

        const int8_t res = lineRequest(pin, settings);
        if (res != 0) return -1;

        return 0;
    }



    /**
     * @brief  Configures a GPIO pin and requests it from the kernel.
     * @param  GPIO line number (offset).
     * @param  PinSettings struc.
     *
     * @return 0 on success, -1 on failure.
     */
    int configurePin(unsigned int pin, const PinSettings& settings) {

        if (!validRequest(pin)) return -1;
        return lineRequest(pin, settings);
    }



    /**
    * @brief  Sets the logic level of an output line.
    * @param  GPIO line number (offset).
    * @param  PinValue HIGH/LOW.
    * @return 0 on success, -1 on failure.
    */
    int setPin(unsigned int pin, PinValue value) {

        if (!validRequest(pin)) return -1;

        struct gpiod_line* line = static_line_data[pin].request;
        if (!line) return -1;

        int valid = gpiod_line_set_value(line, static_cast<int>(value));
        if (valid < 0) return valid;

        return 0;
    }



    /**
    * @brief Reads the logic level of a line.
    * @param GPIO line number (offset).
    *
    * @return PinValue HIGH/LOW/ERROR.
    */
    PinValue getPin(unsigned int pin) {

        if (!validRequest(pin)) return PinValue::ERROR;

        struct gpiod_line* line = static_line_data[pin].request;
        if (!line) return PinValue::ERROR;

        const int value = gpiod_line_get_value(line);
        if (value < 0) return PinValue::ERROR;

        return (value != 0) ? PinValue::HIGH : PinValue::LOW;
    }



    /**
    * @brief Reads the logic level of a line.
    * @param GPIO line number (offset).
    *
    * @return PinValue HIGH/LOW/ERROR on a changing edge.
    */
    std::optional<PinValue> getPinChange(unsigned int pin) {

        if (!validRequest(pin)) return PinValue::ERROR;

        struct gpiod_line* line = static_line_data[pin].request;
        if (!line) return PinValue::ERROR;

        const int value = gpiod_line_get_value(line);
        if (value < 0) return PinValue::ERROR;

        const bool rawValue = (value != 0);

        if (!line_change[pin].received) {
            line_change[pin].current = rawValue;
            line_change[pin].received = 1;
            return std::nullopt;
        }

        if (line_change[pin].current == rawValue) {
            return std::nullopt; 
        }

        line_change[pin].current = rawValue;

        return rawValue ? PinValue::HIGH : PinValue::LOW;
    }
   



private:

    gpiod_chip* chip{nullptr};
    std::atomic<bool> chip_closing{false};


    struct StaticLineData {
        struct gpiod_line *request{nullptr};
        uint32_t line_fd{0}; 
        std::mutex mtx;
    };

    std::array<StaticLineData, MaxEntries> static_line_data;


    struct LineChange {
        uint8_t current  : 1;
        uint8_t received : 1;

        LineChange() :
        current(0),
        received(0)
        {}
    };

    std::array<LineChange, MaxEntries> line_change;

    std::atomic<bool> watchdog_running{false};
    std::thread watchdog_thread;


/******************************************************************************************

   findChip

******************************************************************************************/

    std::string findChip() {
        const char* targets[] = {
            "pinctrl-rp1",
            "rp1-gpio",
            "pinctrl-bcm2712",
            "pinctrl-bcm2835"
        };

        for (const auto& entry : std::filesystem::directory_iterator("/dev")) {
            auto name = entry.path().filename().string();

            if (name.rfind("gpiochip", 0) != 0) continue;

            struct gpiod_chip* chip = gpiod_chip_open(entry.path().c_str());
            if (!chip) continue;

            const char* label = gpiod_chip_label(chip);
            bool matched = false;

            if (label) {
                for (const char* t : targets) {
                    if (std::strstr(label, t)) {
                        matched = true;
                        break;
                    }
                }
            }

            if (matched) {
                gpiod_chip_close(chip);
                return "/dev/" + name;
            }

            gpiod_chip_close(chip);
        }

        return "";
    }




/******************************************************************************************

   LINE REQUEST

******************************************************************************************/

    int lineRequest(uint8_t line_num, PinSettings settings) {

        unsigned int offset = line_num;

        struct gpiod_line *line = gpiod_chip_get_line(chip, offset);
        if (!line) return -1;

        struct gpiod_line_request_config config;
        std::memset(&config, 0, sizeof(config));
        config.consumer = settings.consumer;
        config.flags = 0;

        if (settings.polarity == PinPolarity::ACTIVE_LOW) {
            config.flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
        }

        PinBias active_bias = settings.bias;
        if (settings.edge == PinEdge::EDGE_NONE) {
            if (settings.mode == PinMode::PULLUP || settings.mode == PinMode::OUTPUT_PULLUP) active_bias = PinBias::PULLUP;
            if (settings.mode == PinMode::PULLDOWN) active_bias = PinBias::PULLDOWN;
            if (settings.mode == PinMode::BIAS_DISABLE) active_bias = PinBias::DISABLE;
        }

        switch (active_bias) {
            case PinBias::PULLUP:   config.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP; break;
            case PinBias::PULLDOWN: config.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN; break;
            case PinBias::DISABLE:  config.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE; break;
            default: break;
        }

        if (settings.mode == PinMode::OPEN_DRAIN) {
            config.flags |= GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN;
        }
        else if (settings.mode == PinMode::OPEN_SOURCE) {
            config.flags |= GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE;
        }


        if (settings.edge != PinEdge::EDGE_NONE) {

            int event_type;
            switch (settings.edge) {
                case PinEdge::RISING:  event_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE; break;
                case PinEdge::FALLING: event_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE; break;
                case PinEdge::BOTH:    event_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES; break;
                default: return -1;
            }

            config.request_type = event_type;

            if (gpiod_line_request(line, &config, 0) < 0) {
                return -1;
            }
        } 
        else {
            if (settings.mode == PinMode::OUTPUT || 
                settings.mode == PinMode::OUTPUT_PULLUP || 
                settings.mode == PinMode::OPEN_DRAIN || 
                settings.mode == PinMode::OPEN_SOURCE) {

                config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
 
                if (gpiod_line_request(line, &config, 0) < 0) return -1;
            } 
            else {
                config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
                if (gpiod_line_request(line, &config, 0) < 0) return -1;
            }
        }

        if (settings.debounce > 0) {

            int direction = config.request_type;
            gpiod_line_set_config(line, direction, config.flags, settings.debounce * 1000);

        }

        static_line_data[line_num].request = line; 
        return 0;
    }



/******************************************************************************************

    Guard Clause

******************************************************************************************/
 
   inline bool validRequest(int16_t line = -1) {

        if (!chip || gpiod_chip_num_lines(chip) < 0) return false;
        if (line == -1) return true;
        if (line < 0 || line >= MaxEntries) return false;

        return true;
    }



/******************************************************************************************

   Safe-by-Default

******************************************************************************************/

    int safeStateLine(struct gpiod_line* line) {
        if (!line) return -1;

        int ret = gpiod_line_set_direction_input(line);

        if (ret < 0)  return -1;

        return 0;
    }




/******************************************************************************************

   SIGNAL

******************************************************************************************/

    static_assert(std::atomic<uint32_t>::is_always_lock_free, "Error: std::atomic<uint32_t> is not lock-free on this platform!");

    struct sigaction oldSigInt_{};
    struct sigaction oldSigTerm_{};

    static inline std::atomic<uint32_t> shutdown_requested_ {0};
    static inline std::atomic<uint32_t> caught_signal_{0};



    static void signalHandler(int signum) {
        caught_signal_.store(static_cast<std::uint32_t>(signum), std::memory_order_relaxed);   
        shutdown_requested_.store(1, std::memory_order_relaxed);
    }




/******************************************************************************************

   WATCHDOG

******************************************************************************************/

    void watchdog() {

        if (watchdog_running.exchange(true)) return;

        if constexpr (EnableSignalHandling) {
            struct sigaction sa {};
            sa.sa_handler = &gpiodWrapV1::signalHandler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;

            ::sigaction(SIGINT, &sa, &oldSigInt_);
            ::sigaction(SIGTERM, &sa, &oldSigTerm_);
        }

        watchdog_thread = std::thread([this]() {

            while (watchdog_running.load(std::memory_order_relaxed) && !shutdown_requested_.load(std::memory_order_relaxed)) {       
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if constexpr (EnableSignalHandling) {

                if (shutdown_requested_.load(std::memory_order_relaxed)) {

                    closeChip();

                    uint32_t sig = caught_signal_.load(std::memory_order_relaxed);
                    if (sig != 0) {
                        ::sigaction(SIGINT, &oldSigInt_, nullptr);
                        ::sigaction(SIGTERM, &oldSigTerm_, nullptr);

                        struct sigaction sa_dfl{};
                        sa_dfl.sa_handler = SIG_DFL;
                        ::sigemptyset(&sa_dfl.sa_mask);
                        ::sigaction(sig, &sa_dfl, nullptr);
                        ::raise(sig);
                    }
                }
            }
        });
    }

};

namespace gpiodwrapv1 {


    constexpr auto ERROR    = gpiodWrapV1::PinValue::ERROR;
    constexpr auto HIGH     = gpiodWrapV1::PinValue::HIGH;
    constexpr auto LOW      = gpiodWrapV1::PinValue::LOW;

    constexpr auto AS_IS        = gpiodWrapV1::PinMode::AS_IS;
    constexpr auto INPUT        = gpiodWrapV1::PinMode::INPUT;
    constexpr auto OUTPUT       = gpiodWrapV1::PinMode::OUTPUT;
    constexpr auto PULLUP       = gpiodWrapV1::PinMode::PULLUP;
    constexpr auto PULLDOWN     = gpiodWrapV1::PinMode::PULLDOWN;
    constexpr auto BIAS_DISABLE = gpiodWrapV1::PinMode::BIAS_DISABLE;

    constexpr auto PUSH_PULL     = gpiodWrapV1::PinMode::PUSH_PULL;
    constexpr auto OUTPUT_PULLUP = gpiodWrapV1::PinMode::OUTPUT_PULLUP;
    constexpr auto OPEN_DRAIN    = gpiodWrapV1::PinMode::OPEN_DRAIN;
    constexpr auto OPEN_SOURCE   = gpiodWrapV1::PinMode::OPEN_SOURCE;

    constexpr auto RISING   = gpiodWrapV1::PinEdge::RISING;
    constexpr auto FALLING  = gpiodWrapV1::PinEdge::FALLING;
    constexpr auto BOTH     = gpiodWrapV1::PinEdge::BOTH;

    constexpr auto ACTIVE_LOW  = gpiodWrapV1::PinPolarity::ACTIVE_LOW;
    constexpr auto ACTIVE_HIGH = gpiodWrapV1::PinPolarity::ACTIVE_HIGH;

    constexpr auto IS_INACTIVE = gpiodWrapV1::PinLevel::IS_INACTIVE;
    constexpr auto IS_ACTIVE   = gpiodWrapV1::PinLevel::IS_ACTIVE;

    constexpr auto NO_EVENTS  = gpiodWrapV1::PinEdgeLevel::NO_EVENTS;
    constexpr auto IS_RISING  = gpiodWrapV1::PinEdgeLevel::IS_RISING;
    constexpr auto IS_FALLING = gpiodWrapV1::PinEdgeLevel::IS_FALLING;

} //namespace gpiodwrapv1

