# --------------------------------------------------------------------- #
# 	Variables
# --------------------------------------------------------------------- #
NAME        := MattDaemon

# Dirs
SRCDIR	 	:= /work/src
INCDIR	 	:= /work/inc
OBJDIR	 	:= /work/obj
TSTDIR	 	:= /work/tests

# Flags & cmds
CC	  		:= g++
CFLAGS	  	:= -Wall -Wextra -Werror -Wpedantic -std=c++11
INFLAGS		:= -I$(INCDIR)
MAKEFLAGS	:= --no-print-directory
RM	  		:= rm -rf

# Sources
TESTS	  	:= $(TSTDIR)/unit-FileOps.cpp
SRCS	  	:= $(SRCDIR)/main.cpp \
			   $(SRCDIR)/FileOps.cpp \
			   $(TESTS)
OBJS	  	:= $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Colors
CYA		  	:= \033[0;36m
GRE	  		:= \033[0;32m
YEL	  		:= \033[0;33m
RED	  		:= \033[0;31m
RST	  		:= \033[0m

# --------------------------------------------------------------------- #
# 	Regles
# --------------------------------------------------------------------- #
all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(CYA)[+]$(RST) $@"
	@$(CC) $(CFLAGS) $(INFLAGS) -o $@ $^
	@echo "$(GRE)[√]$(RST) $(NAME) has been built successfully"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@echo "$(CYA)[+]$(RST) Compiling $<"
	@$(CC) $(CFLAGS) $(INFLAGS) -c $< -o $@
	@echo "$(GRE)[√]$(RST) $< has been compiled successfully"

$(OBJDIR):
	@mkdir -p $(OBJDIR)
	@echo "$(YEL)[!]$(RST) Created object directory"

clean:
	@echo "$(RED)[-]$(RST) Removing object files..."
	@$(RM) $(OBJDIR)
	@echo "$(GRE)[√]$(RST) Object files have been removed"

fclean: clean
	@echo "$(RED)[-]$(RST) Removing executable file..."
	@$(RM) $(NAME)
	@echo "$(GRE)[√]$(RST) Executable file has been removed"

re: fclean all

leaks:
	@echo "$(YEL)[!]$(RST) Running leaks check on $(NAME)..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(NAME)

# --------------------------------------------------------------------- #
# 	Phony targets
# --------------------------------------------------------------------- #
.PHONY: all clean fclean re leaks