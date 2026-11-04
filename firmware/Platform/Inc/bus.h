#ifndef BUS_H
#define BUS_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    BUS_OK = 0,
    BUS_ERROR_NACK,
    BUS_ERROR_TIMEOUT,
    BUS_ERROR_OTHER
} bus_status_t;

typedef struct {
    bus_status_t (*i2c_write)(uint8_t addr7, const uint8_t *data, size_t len);
    bus_status_t (*i2c_read)(uint8_t addr7, uint8_t *data, size_t len);
    bus_status_t (*spi_transfer)(const uint8_t *tx, uint8_t *rx, size_t len);
    void         (*cs_set)(int level);      /* NEW: CS# для SPI-flash */
    void         (*delay_ms)(uint32_t ms);  /* NEW: для tWR/tCE flash */
} bus_t;

extern const bus_t *bus;

#endif