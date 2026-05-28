/**
 * @file help_handler.c
 * @brief Documentation output for the traceroute utility.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#include <stdio.h>
#include "main.h"

#define HELP_TEXT \
    "%s v%s          System Manager's Manual          %s v%s\n"                             \
    "\n"                                                                                    \
    "NAME\n"                                                                                \
    "     ft_traceroute — print the route packets trace to a network host\n"                \
    "\n"                                                                                    \
    "SYNOPSIS\n"                                                                            \
    "     ft_traceroute [HOST | --help]\n"                                                  \
    "\n"                                                                                    \
    "DESCRIPTION\n"                                                                         \
    "     The ft_traceroute utility utilizes the IPv4 protocol time to live (TTL)\n"        \
    "     field and attempts to elicit an ICMP TIME_EXCEEDED response from each\n"          \
    "     gateway along the path to some host.\n"                                           \
    "\n"                                                                                    \
    "     The program accepts exactly one argument. You must provide either a valid\n"      \
    "     target destination or the help flag.\n"                                           \
    "\n"                                                                                    \
    "ARGUMENTS\n"                                                                           \
    "     HOST        The target IPv4 address or hostname to trace.\n"                      \
    "     --help      Display this manual page and exit.\n"

/**
 * @brief Prints the program manual to standard output.
 */
void flag_handler_help(void)
{
    printf(HELP_TEXT, NAME, VERSION, NAME, VERSION);
}
