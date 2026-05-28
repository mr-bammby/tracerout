/**
 * @file receive.c
 * @brief Response processing logic for traceroute utilizing external timing utilities.
 * @details Implements the reception and parsing of ICMP messages, dynamic timeout adjustments based on observed RTTs, and state management for probe tracking.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "messages.h"
#include "utils.h"

#define RECEIVE_BUFFER_SIZE 1500
#define MAX_GLOBAL_TIMEOUT_US 3000000L 
#define DYNAMIC_TIMEOUT_MULTIPLIER 3L

#define ICMP_TYPE_TIME_EXCEEDED 11
#define ICMP_TYPE_DEST_UNREACHABLE 3
#define ICMP_CODE_TTL_EXPIRED 0

/**
 * @brief Extracts and validates the inner UDP destination port from an ICMP response payload.
 * @param message Pointer to the raw socket buffer containing the full IP packet.
 * @param len Total length of the received IP packet in bytes.
 * @param stat Pointer to the traceroute tracking statistics structure.
 * @return The integer probe index (0 to PROBE_NUM - 1) on success, -1 on failure.
 */
static short parse_received_message(const uint8_t *message, size_t len, stats_t *stat)
{
    size_t outer_ip_len;
    size_t available_icmp_len;
    size_t inner_ip_len;
    uint16_t orig_dst_port;
    uint32_t port_diff;
    const ip_header_t *ip_hdr;
    const icmp_message_t *icmp_msg;
    const ip_header_t *orig_ip_hdr;
    const udp_message_t *orig_udp_msg;

    if ((message == NULL) || (stat == NULL) || (len < IP_HEADER_SIZE))
    {
        return -1; 
    }

    ip_hdr = (const ip_header_t *)message;
    outer_ip_len = (size_t)ip_hdr->header_len * 4;
    if (len < (outer_ip_len + ICMP_HEADER_SIZE))
    {
        return -1; 
    }

    icmp_msg = (const icmp_message_t *)(message + outer_ip_len);

    /* Process Time Exceeded (11) OR *Any* Destination Unreachable (3) */
    if ((icmp_msg->type == ICMP_TYPE_TIME_EXCEEDED && icmp_msg->code == ICMP_CODE_TTL_EXPIRED) || (icmp_msg->type == ICMP_TYPE_DEST_UNREACHABLE))
    {
        available_icmp_len = len - (outer_ip_len + ICMP_HEADER_SIZE);

        /* Validate we hold at least a basic inner IPv4 header */
        if (available_icmp_len < IP_HEADER_SIZE)
        {
            return -1; 
        }

        orig_ip_hdr = (const ip_header_t *)(icmp_msg->data);
        inner_ip_len = (size_t)orig_ip_hdr->header_len * 4;

        /* Validate the advertised inner IP length against ACTUAL packet size */
        if (available_icmp_len < (inner_ip_len + UDP_HEADER_SIZE))
        {
            return -1; 
        }

        orig_udp_msg = (const udp_message_t *)(icmp_msg->data + inner_ip_len);
        orig_dst_port = ntohs(orig_udp_msg->dst_port);
        
        if (orig_dst_port >= stat->dest_base_port)
        {           
            port_diff = (uint32_t)(orig_dst_port - stat->dest_base_port); // Port diiffrece required to identify the probe index within the batch

            if (port_diff < PROBE_NUM)
            {
                /* Protect against memory leaks if duplicate ICMP packets arrive, assumition that the first received packet is the valid one */
                if (stat->probes[port_diff].dest_addr_str == NULL)
                {
                    stat->probes[port_diff].dest_addr_str = ft_strdup(inet_ntoa(ip_hdr->ip_src)); 
                }
                /* Update target reached flag if a Destination Unreachable message is received. */
                if (icmp_msg->type == ICMP_TYPE_DEST_UNREACHABLE)
                {
                    stat->target_reached = 1;
                }
                return (short)port_diff;
            }
        }
    }
    return -1; 
}

/**
 * @brief Calculates remaining time for select() and checks for global timeouts.
 * @param start Pointer to the absolute start time of the receive window.
 * @param max_wait_us The current maximum allowed wait time in microseconds.
 * @param timeout Pointer to the timeval struct to populate for select().
 * @return 1 if time remains, 0 if timeout reached, -1 on system clock error.
 */
static int compute_remaining_timeout(const struct timeval *start, long max_wait_us, struct timeval *timeout)
{
    struct timeval current_time;
    long passed_us;
    long remaining_us;

    if (gettimeofday(&current_time, NULL) < 0)
    {
        fprintf(stderr, "gettimeofday failed: %s\n", strerror(errno));
        return -1;
    }

    passed_us = elapsed_us(start, &current_time);
    if ((passed_us >= max_wait_us) || (passed_us < 0L))
    {
        return 0; 
    }
    remaining_us = max_wait_us - passed_us;
    timeout->tv_sec = (time_t)(remaining_us / 1000000L);
    timeout->tv_usec = (suseconds_t)(remaining_us % 1000000L);
    return 1;
}

/**
 * @brief Reads data from the ICMP socket and passes it to the parser.
 * @param sock_icmp The raw ICMP file descriptor.
 * @param stats Pointer to the session statistics structure.
 * @return The parsed probe index, or -1 on network or parsing failure.
 */
static short read_and_parse_icmp(int sock_icmp, stats_t *stats)
{
    uint8_t buf[1500];
    struct sockaddr_in resp;
    socklen_t resp_len = sizeof(resp);
    int received_len;

    received_len = recvfrom(sock_icmp, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&resp, &resp_len);
    if (received_len < 0)
    {
        fprintf(stderr, "recvfrom failed: %s\n", strerror(errno));
        return -1;
    }
    return parse_received_message(buf, (size_t)received_len, stats);
}

/**
 * @brief Updates tracking metrics for a valid response and adjusts the dynamic timeout.
 * @param probe_index The index of the successfully parsed probe.
 * @param stats Pointer to the session statistics structure.
 * @param max_wait_us Pointer to the dynamic timeout ceiling (mutates on first hit).
 * @param first_resp Pointer to the flag tracking if a response has been seen yet.
 */
static void update_tracking_state(short probe_index, stats_t *stats, long *max_wait_us, int *first_resp)
{
    long probe_rtt;

    if ((probe_index >= 0) && (stats->probes[probe_index].received == PROBE_NOT_RECEIVED))
    {
        gettimeofday(&(stats->probes[probe_index].recv_time), NULL);
        stats->probes[probe_index].received = PROBE_RECEIVED; // Port matching through index and state update to received
        stats->receive_cnt++;

        /* Apply the (N-1)x RTT optimization on the very first hit of this hop batch */
        if (*first_resp == 0)
        {
            probe_rtt = elapsed_us(&(stats->probes[probe_index].send_time), &(stats->probes[probe_index].recv_time));
            
            if (probe_rtt > 0L)
            {
                *max_wait_us = probe_rtt * (DYNAMIC_TIMEOUT_MULTIPLIER - 1L);
                *first_resp = 1;
            }
        }
    }
}

/**
 * @brief Listens for incoming ICMP messages with a dynamic timeout.
 * @param net_ctx Pointer to the active network file descriptors and context data.
 * @param stats Pointer to tracking metrics block for the current session.
 * @return Total number of responses processed, -1 on socket runtime failures.
 */
int receive_responses(const network_context_t *net_ctx, stats_t *stats)
{
    struct timeval start_time;
    long max_wait_us = MAX_GLOBAL_TIMEOUT_US;
    int first_response_received = 0;
    
    if ((net_ctx == NULL) || (stats == NULL))
    {
        return -1;
    }
    if (gettimeofday(&start_time, NULL) < 0)
    {
        fprintf(stderr, "gettimeofday failed: %s\n", strerror(errno));
        return -1;
    }

    while (stats->receive_cnt < stats->transmit_cnt)
    {
        struct timeval timeout;
        fd_set ready_fds = net_ctx->read_fds; 
        int time_status;
        int activity;

        /* Calculate remaining time */
        time_status = compute_remaining_timeout(&start_time, max_wait_us, &timeout);
        if (time_status < 0) 
        {
            return -1;
        }
        if (time_status == 0) 
        {
            break; /* Tmeout boundary passed */
        }

        /* Wait for ICMP responses with the computed timeout */
        activity = select(net_ctx->sock_icmp + 1, &ready_fds, NULL, NULL, &timeout);
        if (activity < 0) 
        {
            fprintf(stderr, "select() failed: %s\n", strerror(errno));
            return -1;
        }
        if (activity == 0) 
        {
            break; /* select() timed out  */
        }

        /* Read and parse ICMP responses */
        if (FD_ISSET(net_ctx->sock_icmp, &ready_fds))
        {
            short probe_index = read_and_parse_icmp(net_ctx->sock_icmp, stats);
            update_tracking_state(probe_index, stats, &max_wait_us, &first_response_received);
        }
    }
    return stats->receive_cnt;
}
