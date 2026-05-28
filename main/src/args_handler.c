/**
 * @file args_handler.c
 * @brief Parses command-line arguments and resolves target addresses.
 * @details Differentiates between raw IPv4 addresses and Fully Qualified Domain Names (FQDN).
 * @date 2026-05-28
 * @author Domen Banfi
 */

#include "input_handler.h"
#include "flag_handler.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "main.h"

#define MAX_IP_STR_LEN 16

/**
 * @brief Resolves a hostname (FQDN) to an IPv4 address.
 * @param hostname The domain name string to resolve.
 * @param target Buffer to store the resolved IPv4 address string.
 * @return 0 on successful resolution, -1 on DNS lookup failure or formatting error.
 */
int fqdn_handler(char *hostname, char *target)
{
    struct addrinfo hints;
    struct addrinfo *info;
    const char *ip;

    ft_memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       
    hints.ai_socktype = 0;           

    if (getaddrinfo(hostname, NULL, &hints, &info) != 0)
    {
        return -1;
    }
    
    ip = inet_ntoa(((struct sockaddr_in *)info->ai_addr)->sin_addr);
    if (ip == NULL)
    {
        freeaddrinfo(info);
        return -1;
    }

    /* Copy to protect against buffer overwrites */
    ft_memcpy(target, ip, ft_strlen(ip) + 1);

    freeaddrinfo(info);
    return 0;
}

/**
 * @brief Argument parsing, flag handling, and host resolution.
 * @param argc Count of command-line arguments.
 * @param argv Array of command-line arguments.
 * @param target Double pointer to assign the final resolved IP string.
 * @return 0 on success, 1 for flags (like --help), -1 on error.
 */
int parse_args(int argc, char *argv[], char **target)
{
    struct in_addr dummy_addr;

    if (argc != 2)
    {
        return -1;
    }

    if (flags_handler(argv[1]))
    {
        return 1;
    }
    
    if (inet_pton(AF_INET, argv[1], &dummy_addr) == 1)
    {
        *target = ft_strdup(argv[1]);
        if (*target == NULL)
        {
            return -1;
        }
        return 0;
    }
    else
    {
        *target = malloc(sizeof(char) * MAX_IP_STR_LEN);
        if (*target == NULL)
        {
            return -1;
        }
        
        if (fqdn_handler(argv[1], *target) == 0)
        {
            return 0;
        }
        
        free(*target);
        *target = NULL;
    }
    
    return -1;
}


/**
 * @brief Prints the usage information for the program.
 */
void print_usage(void)
{
    fprintf(stderr, "Usage: %s [HOST | --help]\n", NAME);
    fprintf(stderr, "Try '%s --help' for more information.\n", NAME);
}