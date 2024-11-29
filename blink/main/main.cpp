#include "esp_system.h"
#include "driver/ledc.h"
#include "soc/gpio_reg.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <cstdint>
#include <algorithm>

class led_base {
    public:
        virtual void toggle() = 0;
        virtual ~led_base() {};
        bool state_is_on() const { return is_on; }
    protected:
        bool is_on;
        led_base() : is_on(false) {}
    private:
        led_base(const led_base&) = delete;
        led_base& operator=(const led_base&) = delete;
};

class led_port : public led_base {
    public:
        typedef std::uint8_t port_type;
        typedef std::uint8_t bval_type;
        
        led_port(port_type port, bval_type bval) : port(port), bval(bval) {
            esp_rom_gpio_pad_select_gpio(port);
            gpio_set_direction(static_cast<gpio_num_t>(port), GPIO_MODE_OUTPUT);
        }
        
        virtual ~led_port() {}
        
        virtual void toggle() override {
            if (is_on) {
                gpio_set_level(static_cast<gpio_num_t>(port), 0);
            } else {
                gpio_set_level(static_cast<gpio_num_t>(port), 1);
            }
            is_on = !is_on;
        }
    
    private:
        const port_type port;
        const bval_type bval;
};

class pwm final {
    public:
        explicit pwm(const std::uint8_t gpio_pin) : gpio_pin(gpio_pin), channel(LEDC_CHANNEL_0), duty_cycle(0U) {
            // Configure timer
            ledc_timer_config_t timer_conf = {
                .speed_mode = LEDC_HIGH_SPEED_MODE,
                .duty_resolution = LEDC_TIMER_8_BIT,  // 8-bit resolution for duty cycle
                .timer_num = LEDC_TIMER_0,
                .freq_hz = 5000,
                .clk_cfg = LEDC_AUTO_CLK
            };
            ledc_timer_config(&timer_conf);

            // Configure channel
            ledc_channel_config_t chan_conf = {
                .gpio_num = gpio_pin,
                .speed_mode = LEDC_HIGH_SPEED_MODE,
                .channel = LEDC_CHANNEL_0,  // Use channel 0
                .intr_type = LEDC_INTR_DISABLE,
                .timer_sel = LEDC_TIMER_0,
                .duty = 0,
                .hpoint = 0
            };
            ledc_channel_config(&chan_conf);
        }
        
        ~pwm() {
            ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
        }
        
        void set_duty(const std::uint8_t duty) {
            duty_cycle = std::min(duty, std::uint8_t(100U));
            // Convert percentage (0-100) to 8-bit duty cycle (0-255)
            uint32_t duty_scaled = (duty_cycle * 255) / 100;
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_scaled);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
        }
        
        std::uint8_t get_duty() const {
            return duty_cycle;
        }
        
        std::uint8_t get_channel() const {
            return channel;
        }
    
    private:
        const std::uint8_t gpio_pin;
        const std::uint8_t channel;
        std::uint8_t duty_cycle;
};

class led_pwm : public led_base {
    public:
        explicit led_pwm(pwm* p) : my_pwm(p) { }
        
        virtual ~led_pwm() {
            delete my_pwm;
        }
        
        virtual void toggle() override {
            const std::uint8_t duty = (my_pwm->get_duty() > 0U) ? 0U : 100U;
            my_pwm->set_duty(duty);
            is_on = (my_pwm->get_duty() > 0U);
        }
        
        void dimming(const std::uint8_t duty) {
            my_pwm->set_duty(duty);
            is_on = (duty != 0U);
        }
    
    private:
        pwm* my_pwm;
};

extern "C" void app_main(void) {
    led_pwm pwm_led(new pwm(23));  // PWM LED on GPIO 23
    
    printf("Starting PWM dimming...\n");
    
    // Demonstrate PWM LED dimming
    while (true) {
        for (std::uint8_t i = 0U; i <= 100U; i++) {
            pwm_led.dimming(i);
            printf("Setting duty: %u\n", i);
            vTaskDelay(20 / portTICK_PERIOD_MS);  // Smoother transition
        }
        printf("Fading out...\n");
        for (std::uint8_t i = 100U; i > 0U; i--) {
            pwm_led.dimming(i);
            printf("Setting duty: %u\n", i);
            vTaskDelay(20 / portTICK_PERIOD_MS);
        }
    }
    
    printf("PWM demo complete\n");
}