/**
 * @file main.c
 * @brief Main implementation for ft_traceroute.
 * @details Initializes network context, manages the Time-To-Live (TTL) increment loop,
 *          dispatches UDP probes, and processes routing statistics.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "messages.h"
#include "utils.h"
#include "init.h"

/**
 * @brief Formats and outputs the routing statistics for a single TTL hop.
 * @details Evaluates the array of sent probes. If a response was received, it 
 *          calculates and prints the RTT. It dynamically handles asymmetric routing by 
 *          printing the IP address only when it differs from the preceding probe's source.
 * @param[in] stats Pointer to the session statistics structure.
 * @param[in] ttl The current Time-To-Live boundary being evaluated.
 */
static void print_stats(const stats_t *stats, uint8_t ttl)
{
    char *last_dest_addr = NULL;
    float rtt;

    /* Front allignment for single-digit hops */
    if ((ttl - MIN_TTL + 1) < 10)
    {
        printf(" ");
    }
    printf("%d  ", ttl - MIN_TTL + 1);

    for (uint8_t probe = 0; probe < PROBE_NUM; probe++)
    {
        if (stats->probes[probe].received == 1)
        {
            /* Check if we need to print the IP */
            if ((last_dest_addr == NULL) || 
                (ft_strcmp(stats->probes[probe].dest_addr_str, last_dest_addr) != 0))
            {
                printf("%s  ", stats->probes[probe].dest_addr_str);
                last_dest_addr = stats->probes[probe].dest_addr_str;
            }
            
            rtt = elapsed_ms(&(stats->probes[probe].send_time), &(stats->probes[probe].recv_time));
            
            if (rtt >= 0.0f)
            {
                printf("%.2f ms ", rtt);
            }
            else
            {
                fprintf(stderr, "Error calculating RTT for probe %d at TTL %d\n", probe, ttl);
            }
        }
        else
        {
            printf("* ");
        }
    }
    printf("\n");
}

/**
 * @brief Main execution loop for the traceroute utility.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @return 0 on successful execution, 1 on initialization or fatal network failure.
 */
int main(int argc, char *argv[])
{
    udp_message_t udp_message = {0};
    stats_t stats = {0};
    char *target = NULL;
    uint16_t port = TRACEROUTE_PORT_BASE; 
    network_context_t net_ctx = {0};

    /* SET UP */
    if (init(argc, argv, &net_ctx, &target, &udp_message) != 0)
    {
        if (target != NULL)
        {
            free(target);
        }
        return 1;
    }

    for (uint8_t ttl = MIN_TTL; ttl <= MAX_TTL; ttl++)
    {
        for (uint8_t i = 0; i < PROBE_NUM; i++)
        {
            if (stats.probes[i].dest_addr_str != NULL)
            {
                free(stats.probes[i].dest_addr_str);
            }
        }
        
        /* RESET */
        ft_memset(stats.probes, 0, sizeof(stats.probes));
        stats.receive_cnt = 0; 
        stats.transmit_cnt = 0; 
        stats.dest_base_port = port; 

        /* TRANSMIT */
        for (uint8_t probe = 0; probe < PROBE_NUM; probe++)
        {
            net_ctx.dst.sin_port = htons(port);
            port++; 
            
            if (send_next_message(&net_ctx, ttl, &udp_message, &stats, probe) < 0)
            {
                /* Warning: A probe transmission failed. */
                fprintf(stderr, "Warning: Failed to transmit probe %d at TTL %d\n", probe, ttl);
            }
        }

        /* RECEIVE */
        if (receive_responses(&net_ctx, &stats) < 0)
        {
            fprintf(stderr, "Error: ICMP receive crashed.\n");
            cleanup_resources(&net_ctx, target, &stats);
            return 1;
        }

        /* PRINT */
        print_stats(&stats, ttl);
        

        if (stats.target_reached != 0)
        {
            break; 
        }
    }

    /* FREE */
    cleanup_resources(&net_ctx, target, &stats);
    return 0;
}
