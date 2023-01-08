/*
    Interrupt based SPI writeBytes() function for ESP8266
    -----------------------------------------------------
    Use nbSPI_writeBytes(data, size) instead of SPI.writeBytes(data, size).
    nbSPI_writeBytes() will return almost immediatly and NOT block the CPU
    until all data is sent.

    Sending 80 Bytes via nbSPI_writeBytes blocks the CPU for 10µs (6.6µs + 3.5µs).
    Sending 80 Bytes @ 20MHz via SPI.writeBytes blocks the CPU for 40µs.
    Sending 80 Bytes @ 8MHz via SPI.writeBytes blocks the CPU for 88µs.
*/

#pragma once
#include <SPI.h>

// Set an IO to measure timing
#ifdef NBSPI_TDBG_IO
    #if NBSPI_TDBG_IO < 16
        #define NBSPI_TDBG_HIGH GPOS = (1<<NBSPI_TDBG_IO) // 0.1 µs
        #define NBSPI_TDBG_LOW GPOC = (1<<NBSPI_TDBG_IO) // 0.1 µs
    #else
        #error "Use IO 0-15!"
    #endif
#else
    #define NBSPI_TDBG_HIGH
    #define NBSPI_TDBG_LOW
#endif

volatile uint16_t _nbspi_size = 0;
volatile uint32_t * _nbspi_data;
volatile boolean _nbspi_isbusy = false;

void nbSPI_writeBytes(uint8_t *data, uint16_t size);
boolean nbSPI_isBusy();
void nbSPI_writeChunk();
void nbSPI_ISR();

IRAM_ATTR boolean nbSPI_isBusy() {
    if(_nbspi_isbusy) return true; // true while not all data put into buffer
    return (SPI1CMD & SPIBUSY); // true while SPI sends data
}

// This will accept data of ary length to send via SPI
IRAM_ATTR void nbSPI_writeBytes(uint8_t *data, uint16_t size) {
    NBSPI_TDBG_HIGH;
    _nbspi_isbusy = true;
    _nbspi_size = size;
    _nbspi_data = (uint32_t*) data;
    SPI0S &= ~(0x1F); // Disable and clear all interrupts on SPI0
    SPI1S &= ~(0x1F); // Disable and clear all interrupts on SPI1
    ETS_SPI_INTR_ATTACH(nbSPI_ISR, NULL); // Set Interrupt handler
    ETS_SPI_INTR_ENABLE(); // Enable SPI Interrupts
    nbSPI_writeChunk(); // Write first chunk of data
    NBSPI_TDBG_LOW;
}

// This will send up to 64 bytes via SPI
IRAM_ATTR void inline nbSPI_writeChunk() {    
    uint16_t size = _nbspi_size;
    if(size > 64) size = 64;
    _nbspi_size -= size;
    const uint32_t bits = (size * 8) - 1;
    const uint32_t mask = ~(SPIMMOSI << SPILMOSI);
    SPI1U1 = ((SPI1U1 & mask) | (bits << SPILMOSI));

    uint32_t * fifoPtr = (uint32_t*)&SPI1W0;
    uint8_t dataSize = ((size + 3) / 4); // Words to copy to SPI buffer / will actually read beyond your data if it is not 4 Byte aligned
    while(dataSize--) {
        *fifoPtr = *_nbspi_data;
        _nbspi_data++;
        fifoPtr++;
    }
    __sync_synchronize();

    if(_nbspi_size > 0) {
        // There is more data to send after this one, enable Interrupt
        SPI1S |= SPISTRIE; // Enable Transmission End interrupt on SPI1, SPI_TRANS_DONE_EN
    } else {
        // All data is sent after this one
        ETS_SPI_INTR_DISABLE(); // Disable SPI Interrupts
    }

    SPI1CMD |= SPIBUSY; // Start sending data

    if(_nbspi_size == 0) _nbspi_isbusy = false; // Clear flag after starting SPI
}

// Interrupt Handler gets called when SPI finished sending data
IRAM_ATTR void nbSPI_ISR() {
    NBSPI_TDBG_HIGH;
    if(SPIIR & (1 << SPII0)) {
        // SPI0 Interrupt
        SPI0S &= ~(0x1F); // Disable and clear all interrupts on SPI0
    }
    if(SPIIR & (1 << SPII1)) {
        // SPI1 Interrupt
        SPI1S &= ~(0x1F); // Disable and clear all interrupts on SPI1
        nbSPI_writeChunk(); // Write remaining data
    }
    NBSPI_TDBG_LOW;
}
