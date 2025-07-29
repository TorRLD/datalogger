/*
 * Datalogger de Movimento com MPU6050 e Raspberry Pi Pico W
 * Por Heitor Lemos
 * -----------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "ff.h" // Biblioteca FatFs para o sistema de arquivos
#include "sd_card.h" // Funções de baixo nível para o cartão SD

// Bibliotecas para periféricos
#include "lib/ssd1306.h"
#include "lib/font.h"
// A biblioteca ws2812.h foi removida

// --- CONFIGURAÇÕES DOS PINOS (CORRIGIDO) ---
// I2C para MPU6050
#define I2C_MPU_PORT    i2c0
#define I2C_MPU_SDA     0
#define I2C_MPU_SCL     1
#define MPU6050_ADDR    0x68

// I2C para Display OLED
#define I2C_OLED_PORT   i2c1
#define I2C_OLED_SDA    14
#define I2C_OLED_SCL    15
#define OLED_ADDR       0x3C

// ALTERADO: Pinos para LED RGB comum
#define RED_LED_PIN     12
#define GREEN_LED_PIN   11
// ATENÇÃO: Corrigido. No seu exemplo, verde e azul estavam no mesmo pino (13). 
// Assumi pino 14 para o azul. Ajuste este valor se o seu pino for outro.
#define BLUE_LED_PIN    13 

// Botões
#define BUTTON_1_PIN    5 // BOTAO_A
#define BUTTON_2_PIN    6 // BOTAO_B

// ALTERADO: Pinos para Buzzer
#define BUZZER_A_PIN    21
#define BUZZER_B_PIN    10

// --- ESTADOS DO SISTEMA ---
typedef enum {
    STATE_INIT,
    STATE_NO_SD,
    STATE_READY,
    STATE_RECORDING,
    STATE_SAVED
} system_state_t;

// --- VARIÁVEIS GLOBAIS ---
FATFS fs;
FIL fil;
ssd1306_t disp;
volatile bool button1_pressed = false;
volatile bool button2_pressed = false;
volatile system_state_t current_state = STATE_INIT;
bool is_recording = false;
uint32_t sample_count = 0;

long accel_offset[3] = {0, 0, 0};
long gyro_offset[3] = {0, 0, 0};


// --- FUNÇÕES DE CALLBACK PARA INTERRUPÇÕES DOS BOTÕES ---
void gpio_callback(uint gpio, uint32_t events) {
    static uint32_t last_irq_time = 0;
    uint32_t current_irq_time = to_ms_since_boot(get_absolute_time());
    if (current_irq_time - last_irq_time > 250) {
        last_irq_time = current_irq_time;
        if (gpio == BUTTON_1_PIN) button1_pressed = true;
        if (gpio == BUTTON_2_PIN) button2_pressed = true;
    }
}

// --- FUNÇÕES AUXILIARES ---

// ALTERADO: Função para LED RGB comum
void set_rgb_led_color(uint8_t r, uint8_t g, uint8_t b) {
    // Para LED Common Cathode: 1 = aceso, 0 = apagado.
    // Para LED Common Anode, a lógica é invertida (0 = aceso, 1 = apagado).
    gpio_put(RED_LED_PIN, r > 0);
    gpio_put(GREEN_LED_PIN, g > 0);
    gpio_put(BLUE_LED_PIN, b > 0);
}

// ALTERADO: Função para Buzzer de 2 pinos
void play_beep(int count) {
    // Aciona o buzzer colocando um pino em HIGH e outro em LOW
    gpio_put(BUZZER_A_PIN, 1);
    gpio_put(BUZZER_B_PIN, 0);
    sleep_ms(70);
    // Desliga o buzzer
    gpio_put(BUZZER_A_PIN, 0);
    gpio_put(BUZZER_B_PIN, 0);

    if (count > 1) {
        sleep_ms(50);
        gpio_put(BUZZER_A_PIN, 1);
        gpio_put(BUZZER_B_PIN, 0);
        sleep_ms(70);
        gpio_put(BUZZER_A_PIN, 0);
        gpio_put(BUZZER_B_PIN, 0);
    }
}

void update_display(const char* status, const char* detail) {
    ssd1306_fill(&disp, 0);
    ssd1306_draw_string(&disp, "Datalogger MPU6050", 0, 0);
    ssd1306_line(&disp, 0, 10, 128, 10, true);
    ssd1306_draw_string(&disp, "Status:", 0, 20);
    ssd1306_draw_string(&disp, status, 0, 32);
    if (detail) {
        ssd1306_draw_string(&disp, detail, 0, 48);
    }
    ssd1306_send_data(&disp);
}

void mpu6050_reset() {
    uint8_t buf[] = {0x6B, 0x00};
    i2c_write_blocking(I2C_MPU_PORT, MPU6050_ADDR, buf, 2, false);
}

void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3]) {
    uint8_t buffer[6];
    uint8_t reg;
    reg = 0x3B;
    i2c_write_blocking(I2C_MPU_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_MPU_PORT, MPU6050_ADDR, buffer, 6, false);
    for (int i = 0; i < 3; i++) accel[i] = (buffer[i * 2] << 8 | buffer[i * 2 + 1]);
    reg = 0x43;
    i2c_write_blocking(I2C_MPU_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_MPU_PORT, MPU6050_ADDR, buffer, 6, false);
    for (int i = 0; i < 3; i++) gyro[i] = (buffer[i * 2] << 8 | buffer[i * 2 + 1]);
}

void calibrate_imu() {
    const int num_samples = 1000;
    int16_t accel_temp[3], gyro_temp[3];
    for (int i = 0; i < 3; i++) {
        accel_offset[i] = 0;
        gyro_offset[i] = 0;
    }
    update_display("Calibrando...", "Nao mova!");
    set_rgb_led_color(255, 165, 0); // Laranja para calibrando
    for (int i = 0; i < num_samples; i++) {
        mpu6050_read_raw(accel_temp, gyro_temp);
        for (int j = 0; j < 3; j++) {
            accel_offset[j] += accel_temp[j];
            gyro_offset[j] += gyro_temp[j];
        }
        sleep_ms(2);
    }
    for (int i = 0; i < 3; i++) {
        accel_offset[i] /= num_samples;
        gyro_offset[i] /= num_samples;
    }
    #define GRAVITY_RAW 16384
    accel_offset[2] -= GRAVITY_RAW;
    update_display("Calibrado!", "Pronto.");
    sleep_ms(1500);
}


// --- FUNÇÃO PRINCIPAL ---
int main() {
    stdio_init_all();
    
    // ALTERADO: Inicializa LED RGB comum
    gpio_init(RED_LED_PIN);
    gpio_set_dir(RED_LED_PIN, GPIO_OUT);
    gpio_init(GREEN_LED_PIN);
    gpio_set_dir(GREEN_LED_PIN, GPIO_OUT);
    gpio_init(BLUE_LED_PIN);
    gpio_set_dir(BLUE_LED_PIN, GPIO_OUT);
    set_rgb_led_color(255, 255, 0); // Amarelo: Inicializando

    // Inicializa Display OLED
    i2c_init(I2C_OLED_PORT, 400 * 1000);
    gpio_set_function(I2C_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_OLED_SDA);
    gpio_pull_up(I2C_OLED_SCL);
    ssd1306_init(&disp, 128, 64, false, OLED_ADDR, I2C_OLED_PORT);
    ssd1306_config(&disp);
    update_display("Inicializando", "Aguarde...");

    // ALTERADO: Inicializa Buzzer de 2 pinos
    gpio_init(BUZZER_A_PIN);
    gpio_set_dir(BUZZER_A_PIN, GPIO_OUT);
    gpio_init(BUZZER_B_PIN);
    gpio_set_dir(BUZZER_B_PIN, GPIO_OUT);

    // Inicializa I2C para MPU6050
    i2c_init(I2C_MPU_PORT, 400 * 1000);
    gpio_set_function(I2C_MPU_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_MPU_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_MPU_SDA);
    gpio_pull_up(I2C_MPU_SCL);
    mpu6050_reset();

    // Executa a rotina de calibração do IMU
    calibrate_imu();

    // Inicializa os pinos dos botões como entrada com pull-up
    gpio_init(BUTTON_1_PIN);
    gpio_set_dir(BUTTON_1_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_1_PIN);

    gpio_init(BUTTON_2_PIN);
    gpio_set_dir(BUTTON_2_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_2_PIN);

    // Inicializa Botões com interrupção
    gpio_set_irq_enabled_with_callback(BUTTON_1_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_2_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Tenta montar o cartão SD
    sd_init_driver();
    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        current_state = STATE_NO_SD;
    } else {
        current_state = STATE_READY;
    }

    int16_t acceleration[3], gyroscope[3];
    char file_buffer[128];
    char display_detail[20];

    while (1) {
        switch (current_state) {
            case STATE_NO_SD:
                set_rgb_led_color(128, 0, 128); // Roxo
                update_display("ERRO", "SD Nao Detectado");
                sleep_ms(250);
                set_rgb_led_color(0, 0, 0); // Apagado
                sleep_ms(250);
                if (button2_pressed) {
                    button2_pressed = false;
                    update_display("Montando SD...", "");
                    set_rgb_led_color(255, 255, 0); // Amarelo
                    if (f_mount(&fs, "", 1) == FR_OK) current_state = STATE_READY;
                }
                break;

            case STATE_READY:
                set_rgb_led_color(0, 255, 0); // Verde: Pronto
                update_display("Aguardando", "Pressione B1");
                if (button1_pressed) {
                    button1_pressed = false;
                    play_beep(1);
                    set_rgb_led_color(0, 0, 255); // Azul
                    update_display("Iniciando...", "Abrindo arquivo");
                    fr = f_open(&fil, "datalog.csv", FA_OPEN_APPEND | FA_WRITE);
                    if (fr == FR_OK) {
                        if (f_size(&fil) == 0) f_puts("numero_amostra,accel_x,accel_y,accel_z,giro_x,giro_y,giro_z\n", &fil);
                        f_sync(&fil);
                        current_state = STATE_RECORDING;
                    } else {
                        current_state = STATE_NO_SD;
                    }
                }
                break;

            case STATE_RECORDING:
                set_rgb_led_color(255, 0, 0); // Vermelho: Gravando
                mpu6050_read_raw(acceleration, gyroscope);
                sample_count++;
                for (int i = 0; i < 3; i++) {
                    acceleration[i] -= accel_offset[i];
                    gyroscope[i] -= gyro_offset[i];
                }
                sprintf(file_buffer, "%lu,%d,%d,%d,%d,%d,%d\n", 
                        sample_count, acceleration[0], acceleration[1], acceleration[2],
                        gyroscope[0], gyroscope[1], gyroscope[2]);
                set_rgb_led_color(0, 0, 255); // Pisca azul na escrita
                f_puts(file_buffer, &fil);
                f_sync(&fil);
                sprintf(display_detail, "Amostras: %lu", sample_count);
                update_display("Gravando...", display_detail);
                if (button1_pressed) {
                    button1_pressed = false;
                    play_beep(2);
                    set_rgb_led_color(0, 0, 255);
                    f_close(&fil);
                    current_state = STATE_SAVED;
                }
                sleep_ms(100);
                break;
                
            case STATE_SAVED:
                set_rgb_led_color(0, 255, 0);
                update_display("Dados Salvos!", "");
                sleep_ms(2000);
                current_state = STATE_READY;
                break;
                
            case STATE_INIT:
                 sleep_ms(100);
                 break;
        }
    }
    return 0;
}