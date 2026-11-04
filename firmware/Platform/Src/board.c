#include "bus.h"
#include "main.h"

/* --- Налаштування пінів для програмного I2C --- */
#define I2C_PORT GPIOB
#define I2C_SCL_PIN GPIO_PIN_8
#define I2C_SDA_PIN GPIO_PIN_9

/* CS# для SPI-flash (PB6, як у ЛР2) */
#define CS_PORT  GPIOB
#define CS_PIN   GPIO_PIN_6

#define SCL_HIGH() HAL_GPIO_WritePin(I2C_PORT, I2C_SCL_PIN, GPIO_PIN_SET)
#define SCL_LOW()  HAL_GPIO_WritePin(I2C_PORT, I2C_SCL_PIN, GPIO_PIN_RESET)
#define SDA_HIGH() HAL_GPIO_WritePin(I2C_PORT, I2C_SDA_PIN, GPIO_PIN_SET)
#define SDA_LOW()  HAL_GPIO_WritePin(I2C_PORT, I2C_SDA_PIN, GPIO_PIN_RESET)

static uint8_t SDA_READ(void) {
    GPIO_PinState state;
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

    state = HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_PIN);

    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

    return (state == GPIO_PIN_SET) ? 1 : 0;
}

static void i2c_delay(void) {
    /* На 48 МГц ~1 мкс на ітерацію; 3 ітерації ≈ 3 мкс
     * 8 біт × 2 фази × 3 мкс × 2 транзакції ≈ 96 мкс на I2C-читання */
    for (volatile int i = 0; i < 3; i++);
}

static void i2c_start(void) {
    SDA_HIGH(); SCL_HIGH(); i2c_delay();
    SDA_LOW();  i2c_delay();
    SCL_LOW();  i2c_delay();
}

static void i2c_stop(void) {
    SDA_LOW(); SCL_HIGH(); i2c_delay();
    SDA_HIGH(); i2c_delay();
}

static uint8_t i2c_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        if (byte & 0x80) SDA_HIGH(); else SDA_LOW();
        byte <<= 1;
        i2c_delay();
        SCL_HIGH(); i2c_delay();
        SCL_LOW();  i2c_delay();
    }
    SDA_HIGH(); i2c_delay();
    SCL_HIGH(); i2c_delay();
    uint8_t ack = SDA_READ();
    SCL_LOW();  i2c_delay();
    return ack;
}

static uint8_t i2c_read_byte(uint8_t ack) {
    uint8_t byte = 0;
    SDA_HIGH();
    for (int i = 0; i < 8; i++) {
        byte <<= 1;
        SCL_HIGH(); i2c_delay();
        if (SDA_READ()) byte |= 1;
        SCL_LOW();  i2c_delay();
    }
    if (ack) SDA_LOW(); else SDA_HIGH();
    i2c_delay();
    SCL_HIGH(); i2c_delay();
    SCL_LOW();  i2c_delay();
    SDA_HIGH();
    return byte;
}

static bus_status_t bitbang_i2c_write(uint8_t addr7, const uint8_t *data, size_t len) {
    i2c_start();
    if (i2c_write_byte((addr7 << 1) | 0)) { i2c_stop(); return BUS_ERROR_NACK; }
    for (size_t i = 0; i < len; i++) {
        if (i2c_write_byte(data[i])) { i2c_stop(); return BUS_ERROR_NACK; }
    }
    i2c_stop();
    return BUS_OK;
}

static bus_status_t bitbang_i2c_read(uint8_t addr7, uint8_t *data, size_t len) {
    i2c_start();
    if (i2c_write_byte((addr7 << 1) | 1)) { i2c_stop(); return BUS_ERROR_NACK; }
    for (size_t i = 0; i < len; i++) {
        data[i] = i2c_read_byte(i < len - 1);
    }
    i2c_stop();
    return BUS_OK;
}

extern SPI_HandleTypeDef hspi1;
static bus_status_t hal_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t len) {
    HAL_StatusTypeDef s = HAL_SPI_TransmitReceive(&hspi1, (uint8_t*)tx, rx, (uint16_t)len, 100);
    return (s == HAL_OK) ? BUS_OK : BUS_ERROR_OTHER;
}

static void hal_cs_set(int level) {
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void hal_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

static void bitbang_i2c_init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

    SCL_HIGH();
    SDA_HIGH();
}

static const bus_t hal_bus = {
    .i2c_write    = bitbang_i2c_write,
    .i2c_read     = bitbang_i2c_read,
    .spi_transfer = hal_spi_transfer,
    .cs_set       = hal_cs_set,
    .delay_ms     = hal_delay_ms,
};

const bus_t *bus = &hal_bus;

void Board_Init(void) {
    bitbang_i2c_init();
}