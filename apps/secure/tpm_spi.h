#ifndef TPM_SPI_H
#define TPM_SPI_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Initialize the TPM SPI interface.
 *
 * SCB0 is initialized in main.c because it is shared
 * with other SPI devices on the Trainer Kit.
 */
bool tpm_spi_init(void);


/*
 * Read the SLB9670 DID/VID register.
 *
 * Returns:
 *     true  = SPI transaction completed and TPM response
 *             was received.
 *
 *     false = communication failure.
 *
 * DID/VID format:
 *
 *     [31:16] = Device ID
 *     [15:0]  = Vendor ID
 */
bool tpm_read_did_vid(uint32_t *did_vid);


/*
 * Read the SLB9670 Revision ID.
 */
bool tpm_read_rid(uint8_t *rid);


#endif /* TPM_SPI_H */