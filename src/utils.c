/**
 * @file utils.c
 * @brief Standard library and timing utilities.
 * @details Re-implements of essential utuilities.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "utils.h"

/**
 * @brief Calculates the elapsed time between two timeval structures in microseconds.
 * @param[in] start Pointer to the start time benchmark struct.
 * @param[in] end Pointer to the end time benchmark struct.
 * @return The precise elapsed time in microseconds, or -1L if inputs are invalid.
 */
long elapsed_us(const struct timeval *start, const struct timeval *end)
{
    long elapsed_sec;
    long elapsed_usec;
    long total_us;

    if ((start == NULL) || (end == NULL))
    {
        return -1L;
    }
    if ((end->tv_sec < start->tv_sec) || 
        ((end->tv_sec == start->tv_sec) &&(end->tv_usec < start->tv_usec)))
    {
        return -1L;
    }
    elapsed_sec = end->tv_sec - start->tv_sec;
    elapsed_usec = end->tv_usec - start->tv_usec;
    total_us = (elapsed_sec * 1000000L) + elapsed_usec;

    return total_us;
}

/**
 * @brief Calculates the elapsed time between two timeval structures in milliseconds.
 * @param[in] start Pointer to the start time benchmark struct.
 * @param[in] end Pointer to the end time benchmark struct.
 * @return The elapsed time as a floating-point value in milliseconds, or -1.0f if inputs are invalid.
 */
float elapsed_ms(const struct timeval *start, const struct timeval *end)
{
    long us;
    
    us = elapsed_us(start, end);
    if (us < 0L)
    {
        return -1.0f;
    }
    return ((float)us / 1000.0f); /* Convert total microsecond metrics to milliseconds */
}

/**
 * @brief Compares two null-terminated string blocks.
 * @param[in] s1 Pointer to the first string.
 * @param[in] s2 Pointer to the second string.
 * @return Integer evaluation: less than 0, equal to 0, or greater than 0.
 */
int ft_strcmp(const char *s1, const char *s2)
{
    size_t i = 0;

    if ((s1 == NULL) || (s2 == NULL))
    {
        return 0; 
    }

    while ((s1[i] != '\0') && (s1[i] == s2[i]))
    {
        i++;
    }
    
    return (int)((unsigned char)s1[i] - (unsigned char)s2[i]);
}

/**
 * @brief Fills a destination memory with a specific byte value.
 * @param[out] dst Pointer to the target memory block destination.
 * @param[in] c Character value to set, interpreted as an unsigned char.
 * @param[in] n Number of bytes to alter within target block.
 * @return Pointer back to the initialized base address of dst.
 */
void *ft_memset(void *dst, int c, size_t n)
{
    unsigned char *p;
    size_t i;

    if (dst == NULL)
    {
        return NULL;
    }

    p = (unsigned char *)dst;  
    for (i = 0; i < n; i++)
    {
        p[i] = (unsigned char)c;
    }
    
    return dst;
}

/**
 * @brief Copies n of bytes from source to destination.
 * @param[out] dst Pointer to destination memory to copy into.
 * @param[in] src Pointer to source memory segment to copy from.
 * @param[in] n Bytes to copy from src to dst, must not exceed the bounds of either block.
 * @return Destination pointer.
 */
void *ft_memcpy(void *dst, const void *src, size_t n)
{
    unsigned char *d;
    const unsigned char *s;
    size_t i;

    if ((dst == NULL) || (src == NULL))
    {
        return dst;
    }

    d = (unsigned char *)dst;
    s = (const unsigned char *)src;
    for (i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    
    return dst;
}

/**
 * @brief Gets the length of a null-terminated string..
 * @param[in] s Pointer to the source string.
 * @return Number of characters tracking preceding the first null character.
 */
size_t ft_strlen(const char *s)
{
    size_t len = 0;

    if (s == NULL)
    {
        return 0;
    }

    while (s[len] != '\0')
    {
        len++;
        if (len == SIZE_MAX) // Prevent overflow
        {
            return 0;
        }
    }
    
    return len;
}

/**
 * @brief Allocates an and duplicates a null-terminated string on the heap.
 * @param[in] s Pointer to the source null-terminated string.
 * @return Pointer to the newly allocated duplicate string
 *          or NULL if allocation fails or if input is NULL.
 */
char *ft_strdup(const char *s)
{
    size_t len;
    char *dup;

    if (s == NULL)
    {
        return NULL;
    }

    len = ft_strlen(s);
    dup = (char *)malloc(len + 1);
    if (dup == NULL)
    {
        return NULL;
    }

    ft_memcpy(dup, s, len + 1);
    dup[len] = '\0'; // Ensure null termination. 
    
    return dup;
}