/**
 * spi_driver.c — Generic Bare-Metal SPI Driver Implementation
 * ===========================================================
 * Direct register manipulation — no HAL, no vendor libraries.
 * All register names and bit definitions are in spi_driver.h.
 *
 * INTERVIEW WALK-THROUGH NOTES:
 * ─────────────────────────────
 * SPI is a synchronous, full-duplex serial protocol with four wires:
 *   MOSI  — Master Out, Slave In   (master drives data to slave)
 *   MISO  — Master In,  Slave Out  (slave drives data to master)
 *   SCK   — Serial Clock           (always driven by master)
 *   CS/SS — Chip Select            (active low; one per slave)
 *
 * Modes (CPOL + CPHA combo):
 *   Mode 0: CPOL=0 CPHA=0 — idle low,  sample on rising  edge  (most common)
 *   Mode 1: CPOL=0 CPHA=1 — idle low,  sample on falling edge
 *   Mode 2: CPOL=1 CPHA=0 — idle high, sample on falling edge
 *   Mode 3: CPOL=1 CPHA=1 — idle high, sample on rising  edge
 *
 * Transfer flow (master, 8-bit):
 *   1. Wait for TXE=1 (TX buffer empty — safe to load new byte)
 *   2. Write byte to DR (hardware starts shifting immediately)
 *   3. Wait for RXNE=1 (RX buffer has the byte shifted in from slave)
 *   4. Read DR (clears RXNE)
 *   5. Wait for BSY=0 before de-asserting CS (ensures last bit shifted out)
 */

#include "spi_driver.h"

/* --------------------------------------------------------------------------
 * SPI_Init
 * Build CR1 from the config struct, write it with SPE=0, then set SPE last.
 * Order matters: the peripheral must be disabled while configuring.
 * -------------------------------------------------------------------------- */
void SPI_Init(const SPI_Config *cfg)
{
    /* Step 1: Disable SPI before changing any settings.
     *         Writing to CR1 while SPE=1 causes undefined behaviour. */
    SPI_CR1 &= ~SPI_CR1_SPE;

    /* Step 2: Build the new CR1 value from scratch. */
    uint32_t cr1 = 0U;

    /* Master / Slave selection */
    if (cfg->mode == SPI_MODE_MASTER) {
        cr1 |= SPI_CR1_MSTR;   /* Set MSTR=1: this peripheral drives SCK */
    }

    /* Clock polarity — defines SCK idle state */
    if (cfg->cpol == SPI_CPOL_HIGH) {
        cr1 |= SPI_CR1_CPOL;   /* CPOL=1: SCK idles high */
    }

    /* Clock phase — defines which edge latches data */
    if (cfg->cpha == SPI_CPHA_2EDGE) {
        cr1 |= SPI_CR1_CPHA;   /* CPHA=1: capture on second clock edge */
    }

    /* Baud rate: BR[2:0] divide fPCLK by 2^(BR+1).
     *   baud_div enum already encodes the bit-shifted value (see header). */
    cr1 |= (cfg->baud_div & SPI_CR1_BR_MASK);

    /* Data frame format: 8-bit (default) or 16-bit */
    if (cfg->data_size == SPI_DATA_16BIT) {
        cr1 |= SPI_CR1_DFF;    /* DFF=1: 16-bit frames */
    }

    /* Bit order: MSB first is standard; LSB first is rare (some sensors) */
    if (cfg->bit_order == SPI_LSB_FIRST) {
        cr1 |= SPI_CR1_LSBFIRST;
    }

    /* Step 3: Write config — SPE still 0 */
    SPI_CR1 = cr1;

    /* Step 4: Enable the peripheral — starts the clock, pins go active */
    SPI_CR1 |= SPI_CR1_SPE;
}

/* --------------------------------------------------------------------------
 * SPI_Transceive  (core primitive — TX and RX happen in the same clock cycle)
 *
 * Every SPI transfer is inherently full-duplex:
 *   - Master shifts out txData on MOSI, one bit per SCK edge
 *   - Slave shifts out its byte on MISO simultaneously
 *   - After 8 clocks, both sides have exchanged a complete byte
 *
 * All other functions (Transmit, Receive) are built on top of this.
 * -------------------------------------------------------------------------- */
uint8_t SPI_Transceive(uint8_t txData)
{
    /* Wait until the TX buffer is empty.
     * TXE=1 means the shift register has picked up the previous byte
     * and DR is free to accept the next one. */
    while (!(SPI_SR & SPI_SR_TXE)) {
        /* Spin. In production, add a timeout counter here. */
    }

    /* Load the transmit data register.
     * Hardware immediately starts clocking the byte out on MOSI. */
    SPI_DR = (uint32_t)txData;

    /* Wait for the receive buffer to fill.
     * RXNE=1 means 8 bits have been clocked in from MISO and are
     * sitting in DR, ready to read. */
    while (!(SPI_SR & SPI_SR_RXNE)) {
        /* Spin. */
    }

    /* Read and return the received byte.
     * Reading DR also clears the RXNE flag. */
    return (uint8_t)SPI_DR;
}

/* --------------------------------------------------------------------------
 * SPI_Transmit  (write-only — discard the received byte)
 *
 * Use when you only need to send data to the slave
 * (e.g. writing a command or config register).
 * -------------------------------------------------------------------------- */
void SPI_Transmit(uint8_t data)
{
    /* Reuse Transceive — the received byte is simply discarded */
    (void)SPI_Transceive(data);
}

/* --------------------------------------------------------------------------
 * SPI_Receive  (read-only — send a dummy byte to generate the clock)
 *
 * SPI has no separate "read" operation. To receive a byte, the master must
 * still generate 8 clock cycles. We send 0xFF (all ones) as a neutral dummy
 * — chosen so it doesn't accidentally trigger any command on the slave.
 * -------------------------------------------------------------------------- */
uint8_t SPI_Receive(void)
{
    return SPI_Transceive(0xFFU);   /* 0xFF = dummy TX, returns real RX */
}

/* --------------------------------------------------------------------------
 * SPI_Disable
 *
 * Proper shutdown sequence:
 *   1. Wait for the last byte to finish (BSY=0)
 *   2. Clear SPE
 *
 * Never de-assert CS before BSY=0 — you will cut off the last bit.
 * -------------------------------------------------------------------------- */
void SPI_Disable(void)
{
    /* Wait for BSY to clear — shift register is idle */
    while (SPI_SR & SPI_SR_BSY) {
        /* Spin. */
    }

    /* Clear SPE — peripheral disabled, pins return to GPIO control */
    SPI_CR1 &= ~SPI_CR1_SPE;
}
