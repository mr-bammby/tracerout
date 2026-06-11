/**
 * @file input_handler.h
 * @brief Public interface for command-line argument parsing and validation.
 * @details Provides functions for parsing command-line arguments and validating input.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

int parse_args(int argc, char *argv[], char **target);
void print_usage(void);

#endif // INPUT_HANDLER_H