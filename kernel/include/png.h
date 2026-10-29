#ifndef KERNEL_PNG_H
#define KERNEL_PNG_H
#include <stdint.h>
#include <stddef.h>
int png_decode(const uint8_t *data, size_t length, uint32_t *pixels,
               size_t capacity, uint32_t *width, uint32_t *height);
#endif
