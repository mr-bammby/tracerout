/**
 * @file utils.h
 * @brief Interface for standard library and timing utilities.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

/* Standard library replacements */
int ft_strcmp(const char *s1, const char *s2);
char *ft_strdup(const char *s);
void *ft_memset(void *b, int c, size_t len);
void *ft_memcpy(void *dst, const void *src, size_t n);
size_t ft_strlen(const char *s);

/* Time measurement functions */
float elapsed_ms(const struct timeval *start, const struct timeval *end);
long elapsed_us(const struct timeval *start, const struct timeval *end);


#endif // __UTILS_H__