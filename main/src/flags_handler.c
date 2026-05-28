/**
 * @file flag_handler.c
 * @brief Command-line flag parsing and dispatch logic.
 * @details Implements a scalable dispatch table for CLI flags, allowing easy addition
 * of future command-line options.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#include <inttypes.h>
#include "flag_handler.h"
#include "utils.h" 

static const char* flags[] = {"--help"};
static void (*flag_funcs[])(void) = {flag_handler_help};

/**
 * @brief Checks input against known flags and executes the corresponding handler.
 * @param flag The argument string to check.
 * @return 1 if a flag was found and handled, 0 otherwise.
 */
int flags_handler(char *flag)
{
    for (uint8_t i = 0; i < (sizeof(flags)/sizeof(flags[0])); i++)
    {
        if (ft_strcmp(flags[i], flag) == 0)
        {
            flag_funcs[i]();
            return (1);
        }
    }
    return (0);
}
