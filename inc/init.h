/**
 * @file init.h
 * @brief Interface for initialization functions for the traceroute application.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#ifndef __INIT_H__
#define __INIT_H__

void cleanup_resources(network_context_t *net_ctx, char *target, stats_t *stats);
int init(int argc, char *argv[], network_context_t *net_ctx, char **target, udp_message_t *udp_message);

#endif // __INIT_H__