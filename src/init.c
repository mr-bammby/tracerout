/**
 * @file init.c
 * @brief Initialization and resource management functions for the traceroute utility.
 * @details Handles argument parsing, socket creation (UDP and Raw ICMP), 
 * network context configuration, and cleanup of allocated system resources.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "main.h"
#include "messages.h"
#include "input_handler.h"
#include "utils.h"

/**
 * @brief Fills the UDP transmission buffer with a default character pattern.
 * @param udp_message Pointer to the UDP message structure to populate.
 */
static void setup_udp_message(udp_message_t *udp_message)
{
    ft_memcpy(&(udp_message->data), UDP_PAYLOAD_STR, UDP_TRANSMIT_PAYLOAD_SIZE);
}

/**
 * @brief Prints header to standard output.
 * @param unresolved_addr The original target string input by the user.
 * @param resolved_addr The resolved IP address string.
 */
static void print_header(char *unresolved_addr, char *resolved_addr)
{
    printf("%s to %s (%s), %d hops max, %ld byte packets\n", NAME, unresolved_addr, resolved_addr, MAX_TTL, TRANSMIT_MESSAGE_SIZE);
}

/**
 * @brief Opens and initializes the standard UDP and raw ICMP sockets.
 * @param net_ctx Pointer to the network context structure where descriptors are stored.
 * @return 0 on success, -1 on socket creation failure (logs error to stderr).
 */
static int setup_sockets(network_context_t *net_ctx)
{
    net_ctx->sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (net_ctx->sock_udp < 0)
    {
        fprintf(stderr, "udp socket: %s\n", strerror(errno));
        return -1;
    }

    net_ctx->sock_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (net_ctx->sock_icmp < 0)
    {
        fprintf(stderr, "icmp socket: %s\n", strerror(errno));
        close(net_ctx->sock_udp);
        net_ctx->sock_udp = -1; 
        return -1;
    }
    return 0;
}

/**
 * @brief Releases all allocated memory, active hop strings, and closes network sockets.
 * @param net_ctx Pointer to the active network context.
 * @param target Pointer to the target string.
 * @param stats Pointer to the session statistics tracking structure.
 */
void cleanup_resources(network_context_t *net_ctx, char *target, stats_t *stats)
{
    if (stats != NULL)
    {
        for (unsigned int i = 0; i < PROBE_NUM; i++)
        {
            if (stats->probes[i].dest_addr_str != NULL)
            {
                free(stats->probes[i].dest_addr_str);
                stats->probes[i].dest_addr_str = NULL;
            }
        }
    }

    if (net_ctx != NULL)
    {
        if (net_ctx->sock_udp >= 0)
        {
            close(net_ctx->sock_udp);
            net_ctx->sock_udp = -1;
        }
        if (net_ctx->sock_icmp >= 0)
        {
            close(net_ctx->sock_icmp);
            net_ctx->sock_icmp = -1;
        }
    }
    
    if (target != NULL)
    {
        free(target);
    }
}

/**
 * @brief Orchestrates the entire startup phase of the utility.
 * @param argc Count of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @param net_ctx Pointer to the network context structure to initialize.
 * @param target Double pointer to assign the parsed target host string.
 * @param udp_message Pointer to the message payload structure to set up.
 * @return 0 on successful initialization, 1 on any configuration failure.
 */
int init(int argc, char *argv[], network_context_t *net_ctx, char **target, udp_message_t *udp_message)
{
    net_ctx->sock_udp = -1;
    net_ctx->sock_icmp = -1;

    if (parse_args(argc, argv, target) != 0)
    {
        print_usage();
        return 1;
    }

    setup_udp_message(udp_message);

    if (setup_sockets(net_ctx) != 0)
    {
        fprintf(stderr, "Error Setting up sockets failed\n");
        if (*target != NULL)
        {
            free(*target);
            *target = NULL;
        }
        return 1;
    }

    FD_ZERO(&net_ctx->read_fds); // Initialize the file descriptor set for select() multiplexing.
    FD_SET(net_ctx->sock_icmp, &net_ctx->read_fds);  // Add the ICMP socket to the set for monitoring incoming responses.

    (void)ft_memset(&net_ctx->dst, 0, sizeof(net_ctx->dst));
    net_ctx->dst.sin_family = AF_INET;
    
    if (inet_pton(AF_INET, *target, &net_ctx->dst.sin_addr) != 1)
    {
        fprintf(stderr, "Error Function inet_pton failed for %s\n", *target);
        cleanup_resources(net_ctx, *target, NULL);
        *target = NULL; 
        return 1;
    }
    
    net_ctx->dstlen = sizeof(net_ctx->dst);
    print_header(argv[1], *target);
    return 0;
}
