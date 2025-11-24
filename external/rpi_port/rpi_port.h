/**
 * @file rpi_port.h
 *
 */

#ifndef RPI_PORT_H
#define RPI_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

int rpi_port_init(uint32_t width, uint32_t height);

void rpi_port_deinit(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*RPI_PORT_H*/
