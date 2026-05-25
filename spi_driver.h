/**
 * spi_driver.h — Generic Bare-Metal SPI Driver
 * =============================================
 * Platform-independent SPI driver using direct register manipulation.
 * Adapt the BASE ADDRESS and register offsets to match your MCU's datasheet.
 *
 * USAGE EXAMPLE (in comments so this file stays self-contained):
 * --------------------------------------------------------------
 *   #include "spi_driver.h"
 *
 *   // 1. Configure
 *   SPI_Config cfg = {
 *       .mode       = SPI_MODE_MASTER,
 *       .cpol       = SPI_CPOL_LOW,     // Clock idle low  (CPOL=0)
 *       .cpha       = SPI_CPHA_1EDGE,   // Sample on first edge (CPHA=0)
 *       .baud_div   = SPI_BAUD_DIV8,    // fPCLK / 8
 *       .data_size  = SPI_DATA_8BIT,
 *       .bit_order  = SPI_MSB_FIRST,
 *   };
 *   SPI_Init(&cfg);
 *
 *   // 2. Transfer
 *   SPI_CS_Low();                        // Assert chip-select (GPIO, your code)
 *   uint8_t rx = SPI_Transceive(0xA5);  // Send 0xA5, receive simultaneously
 *   SPI_CS_High();                       // De-assert chip-select
 *
 * PORTING GUIDE:
 * --------------
 *   1. Set SPI_BASE to your peripheral's base address (check MCU reference manual)
 *   2. Verify register offsets in SPI_REG_* match your MCU
 *   3. Verify CR1 bit positions — most Cortex-M MCUs follow this layout
 *   4. Add GPIO clock-enable and alternate-function setup for MOSI/MISO/SCK pins
 *   5. Enable SPI peripheral clock (e.g. RCC->APB2ENR |= SPI1EN)
 */

#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <stdint.h>

/* ==========================================================================
 * 1. BASE ADDRESS — change this for your MCU / SPI instance
 * ==========================================================================
 * Examples (check your reference manual):
 *   STM32F4  SPI1 : 0x40013000
 *   STM32F4  SPI2 : 0x40003800
 *   nRF52840 SPI0 : 0x40003000
 *   RP2040   SPI0 : 0x4003C000
 * ========================================================================== */
#define SPI_BASE    0x40013000UL    /* <-- replace with your MCU's SPI base */

/* ==========================================================================
 * 2. REGISTER MAP  (base + offset → volatile pointer dereference)
 *    Each register is 32-bit; unused upper bits are reserved / read-as-zero.
 * ========================================================================== */
#define SPI_CR1     (*(volatile uint32_t *)(SPI_BASE + 0x00U))  /* Control Register 1   */
#define SPI_CR2     (*(volatile uint32_t *)(SPI_BASE + 0x04U))  /* Control Register 2   */
#define SPI_SR      (*(volatile uint32_t *)(SPI_BASE + 0x08U))  /* Status Register      */
#define SPI_DR      (*(volatile uint32_t *)(SPI_BASE + 0x0CU))  /* Data Register        */

/* ==========================================================================
 * 3. CR1 BIT DEFINITIONS
 *    CR1 controls the core SPI behaviour. Set before enabling (SPE).
 * ========================================================================== */
#define SPI_CR1_CPHA        (1U << 0)   /* Clock Phase
                                           0 = capture on first  clock transition
                                           1 = capture on second clock transition */
#define SPI_CR1_CPOL        (1U << 1)   /* Clock Polarity
                                           0 = idle low  (SCK low when bus is free)
                                           1 = idle high (SCK high when bus is free) */
#define SPI_CR1_MSTR        (1U << 2)   /* Master Selection
                                           0 = slave mode
                                           1 = master mode — we drive SCK */
#define SPI_CR1_BR_MASK     (7U << 3)   /* Baud Rate Control [BR2:BR1:BR0] — bits 5:3
                                           Divides the peripheral clock (fPCLK):
                                           000 = /2   001 = /4   010 = /8
                                           011 = /16  100 = /32  101 = /64
                                           110 = /128 111 = /256 */
#define SPI_CR1_SPE         (1U << 6)   /* SPI Enable — set last, after all config */
#define SPI_CR1_LSBFIRST    (1U << 7)   /* Frame Format
                                           0 = MSB transmitted first (most common)
                                           1 = LSB transmitted first */
#define SPI_CR1_DFF         (1U << 11)  /* Data Frame Format
                                           0 = 8-bit data frame
                                           1 = 16-bit data frame */

/* ==========================================================================
 * 4. SR BIT DEFINITIONS  (Status Register — read-only flags)
 * ========================================================================== */
#define SPI_SR_RXNE         (1U << 0)   /* Receive buffer Not Empty
                                           1 = data waiting to be read from DR */
#define SPI_SR_TXE          (1U << 1)   /* Transmit buffer Empty
                                           1 = DR can accept new data to send */
#define SPI_SR_BSY          (1U << 7)   /* Busy flag
                                           1 = SPI is busy or TX FIFO not empty
                                           Wait for BSY=0 before de-asserting CS */

/* ==========================================================================
 * 5. BAUD RATE DIVIDER PRESETS  (plug into SPI_Config.baud_div)
 * ========================================================================== */
typedef enum {
    SPI_BAUD_DIV2   = (0U << 3),   /* fPCLK / 2   — fastest */
    SPI_BAUD_DIV4   = (1U << 3),
    SPI_BAUD_DIV8   = (2U << 3),
    SPI_BAUD_DIV16  = (3U << 3),
    SPI_BAUD_DIV32  = (4U << 3),
    SPI_BAUD_DIV64  = (5U << 3),
    SPI_BAUD_DIV128 = (6U << 3),
    SPI_BAUD_DIV256 = (7U << 3),   /* fPCLK / 256 — slowest */
} SPI_BaudDiv;

/* ==========================================================================
 * 6. CONFIGURATION ENUMS — self-documenting at the call site
 * ========================================================================== */
typedef enum { SPI_MODE_SLAVE = 0, SPI_MODE_MASTER = 1 } SPI_Mode;
typedef enum { SPI_CPOL_LOW   = 0, SPI_CPOL_HIGH   = 1 } SPI_CPOL;  /* idle level */
typedef enum { SPI_CPHA_1EDGE = 0, SPI_CPHA_2EDGE  = 1 } SPI_CPHA;  /* sample edge */
typedef enum { SPI_DATA_8BIT  = 0, SPI_DATA_16BIT  = 1 } SPI_DataSize;
typedef enum { SPI_MSB_FIRST  = 0, SPI_LSB_FIRST   = 1 } SPI_BitOrder;

/* ==========================================================================
 * 7. CONFIGURATION STRUCT
 * ========================================================================== */
typedef struct {
    SPI_Mode      mode;       /* Master or Slave                        */
    SPI_CPOL      cpol;       /* Clock polarity (idle level)            */
    SPI_CPHA      cpha;       /* Clock phase (which edge to sample)     */
    SPI_BaudDiv   baud_div;   /* Clock divider (fPCLK / N)              */
    SPI_DataSize  data_size;  /* 8-bit or 16-bit frames                 */
    SPI_BitOrder  bit_order;  /* MSB first or LSB first                 */
} SPI_Config;

/* ==========================================================================
 * 8. PUBLIC API
 * ========================================================================== */

/**
 * SPI_Init — Configure and enable the SPI peripheral.
 * Call this once at startup (after enabling peripheral clock and GPIO pins).
 */
void SPI_Init(const SPI_Config *cfg);

/**
 * SPI_Transmit — Send one byte; ignore received byte.
 * Blocks until the TX buffer is ready, then loads the data register.
 */
void SPI_Transmit(uint8_t data);

/**
 * SPI_Receive — Send a dummy byte (0xFF) to generate SCK, return received byte.
 * Used when reading from a slave that only cares about receiving a clock.
 */
uint8_t SPI_Receive(void);

/**
 * SPI_Transceive — Full-duplex: send and receive simultaneously.
 * This is the fundamental SPI operation — TX and RX happen in the same frame.
 * Returns the byte shifted in from MISO while txData was shifted out on MOSI.
 */
uint8_t SPI_Transceive(uint8_t txData);

/**
 * SPI_Disable — Disable the peripheral (clears SPE).
 * Wait for BSY=0 first to avoid cutting off an active transfer.
 */
void SPI_Disable(void);

#endif /* SPI_DRIVER_H */
