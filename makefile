# Name of the final executable
NAME = ft_traceroute

# Compiler and basic flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror
CINC = -I./main/inc

# Build output directory for object files
OBJDIR = build

# All source files from your tree
SRCS = main/src/main.c \
       main/src/args_handler.c \
       main/src/flag_handler_help.c \
       main/src/flags_handler.c \
       main/src/init.c \
       main/src/transmit.c \
       main/src/receive.c \
       main/src/utils.c 
# Object files go into the build directory, preserving source subpaths
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

# Default rule to build the program
all: $(NAME)

# Rule to link the object files into the final executable
$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

# Rule to compile source files into object files in build directory
$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CINC) -c $< -o $@

# Rule to clear out the compiled object files
clean:
	rm -rf $(OBJDIR)

# Rule to clear out object files AND the compiled executable
fclean: clean
	rm -f $(NAME)

# Rule to rebuild everything from scratch
re: fclean all

# Prevents make from getting confused if a file named "clean" or "all" exists
.PHONY: all clean fclean re