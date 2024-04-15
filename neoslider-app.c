#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdint.h>

#define I2C_DEVICE "/dev/i2c-5"
#define NEOSLIDER_ADDR 0x30
#define POTENTIOMETER_PIN 18
#define MAX_HASHES 161

#define SEESAW_GPIO_BASE 0x01
#define SEESAW_GPIO_BULK_INPUT 0x04

#define SEESAW_ADC_BASE 0x09
#define SEESAW_ADC_CHANNEL_OFFSET 0x07

#define SEESAW_STATUS_BASE 0x00
#define SEESAW_STATUS_SWRST 0x7F

int i2c_fd;

void i2c_init() {
    i2c_fd = open(I2C_DEVICE, O_RDWR);
    if (i2c_fd < 0) {
        perror("Failed to open I2C bus");
        exit(1);
    }

    if (ioctl(i2c_fd, I2C_SLAVE, NEOSLIDER_ADDR) < 0) {
        perror("Failed to set I2C slave address");
        close(i2c_fd);
        exit(1);
    }
}

void seesaw_init(int i2c_fd) {
    uint8_t reset_cmd[] = {SEESAW_STATUS_BASE, SEESAW_STATUS_SWRST};
    if (write(i2c_fd, reset_cmd, sizeof(reset_cmd)) != sizeof(reset_cmd)) {
        perror("Failed to send reset command");
    }
    usleep(500 * 1000);  
}

uint16_t seesaw_analog_read(int i2c_fd, uint8_t channel) {
    uint8_t cmd[] = {SEESAW_ADC_BASE, SEESAW_ADC_CHANNEL_OFFSET + channel};
    if (write(i2c_fd, cmd, sizeof(cmd)) != sizeof(cmd)) {
        perror("Failed to send ADC read command");
        return 0;  
    }
    usleep(1000);  

    uint8_t buf[2] = {0};
    if (read(i2c_fd, buf, sizeof(buf)) != sizeof(buf)) {
        perror("Failed to read ADC value");
        return 0;  
    }
    return (buf[0] << 8) | buf[1];  
}

void display_hashes(int mid_point, uint16_t value) {
    printf("\033[2J\033[H");  // Clear screen and move cursor to home position

    // Determine color based on value
    if (value < 341) {
        printf("\033[32m");  // Set color to green
    } else if (value < 682) {
        printf("\033[33m");  // Set color to yellow
    } else {
        printf("\033[31m");  // Set color to red
    }

    // Calculate the left and right spread from the middle
    int left_hashes = mid_point / 2;
    int right_hashes = mid_point - left_hashes;

    // Print spaces to move to the middle starting point
    for (int i = 0; i < (MAX_HASHES / 2 - left_hashes); ++i) {
        putchar(' ');
    }

    // Print left side hashes
    for (int i = 0; i < left_hashes; ++i) {
        putchar('#');
    }

    // Print right side hashes
    for (int i = 0; i < right_hashes; ++i) {
        putchar('#');
    }

    printf("\033[0m");  // Reset color
    fflush(stdout);
}

int main() {
    // main program
    printf("\033[?25l");
    printf("\033[2J\033[H");

    i2c_init();
    seesaw_init(i2c_fd);

    uint16_t previous_value = 512, current_value;

    while (1) {
        current_value = seesaw_analog_read(i2c_fd, POTENTIOMETER_PIN);  // Read potentiometer value
        int mid_point = (int)((double)current_value / 1023 * MAX_HASHES);  // Scale ADC value to max hashes
        display_hashes(mid_point, current_value);
        usleep(50000);  // Update every 100 milliseconds
    }

    close(i2c_fd);
    return 0;
}