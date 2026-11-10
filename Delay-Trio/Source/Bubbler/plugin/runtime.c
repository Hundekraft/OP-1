/* SPDX-License-Identifier: GPL-3.0-or-later
 * Freestanding runtime: Opus does not export the C library memset symbol.
 * Volatile stores prevent the compiler replacing this loop with memset itself.
 */
void *memset(void *destination, int value, __SIZE_TYPE__ count)
{
    volatile unsigned char *out = destination;
    while (count--) *out++ = (unsigned char)value;
    return destination;
}
