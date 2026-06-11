/**
 * @file transmit.c
 * @brief Transmission logic for the traceroute utility.
 * @details Handles socket option configurations for dynamic TTL tracking 
 *          and handles packet transmissions over standard UDP wrappers.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#include <stdint.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "messages.h"
#include "main.h"

/**
 * @brief Compiles and transmits a target network probe packet with custom socket TTL limits.
 * @param net_ctx Pointer to the configuration containing active sockets.
 * @param ttl The explicit time-to-live hop value to set on the transmission sequence.
 * @param udp_message Pointer to the data payload object being routed.
 * @param stats Pointer to the session statistics destination structure.
 * @param probe_index Sequential index token tracking the specific probe burst.
 * @return 0 on successful socket write, -1 on configuration or runtime failures.
 */
int send_next_message(const network_context_t *net_ctx, uint8_t ttl, const udp_message_t *udp_message, stats_t *stats, uint32_t probe_index)
{
    int ttl_val;

    if ((net_ctx == NULL) || (udp_message == NULL) || (stats == NULL))
    {
        return -1;
    }

    ttl_val = (int)ttl;
    if (setsockopt(net_ctx->sock_udp, IPPROTO_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) < 0)
    {
        fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
        return -1;
    }

    if (gettimeofday(&(stats->probes[probe_index].send_time), NULL) < 0)
    {
        fprintf(stderr, "gettimeofday failed: %s\n", strerror(errno));
        return -1;
    }

    if (sendto(net_ctx->sock_udp, (const void *)&(udp_message->data), sizeof(udp_message->data), 0, (const struct sockaddr *)&(net_ctx->dst), net_ctx->dstlen) < 0)
    {
        fprintf(stderr, "sendto failed: %s\n", strerror(errno));
        stats->probes[probe_index].received = PROBE_PASSIVE; /* Mark as failed o bypass tracking hang */
        return -1;
    }
    else
    {
        stats->probes[probe_index].received = PROBE_NOT_RECEIVED; /* Mark as not received */
        stats->transmit_cnt++;
    }

    return 0;
}
