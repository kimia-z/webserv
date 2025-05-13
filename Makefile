# **************************************************************************** #
#                                                                              #
#                                                         ::::::::             #
#    Makefile                                           :+:    :+:             #
#                                                      +:+                     #
#    By: kziari <kziari@student.42.fr>                +#+                      #
#                                                    +#+                       #
#    Created: 2025/05/05 10:26:11 by mstencel      #+#    #+#                  #
#    Updated: 2025/05/13 10:44:58 by mstencel      ########   odam.nl          #
#                                                                              #
# **************************************************************************** #

RESET = \033[0m
GREEN = \033[38;5;82m
RED = \033[31;1m
ITALIC = \033[3m

NAME = webserv

SRD_DIR = src

SRC_FILES = src/main.cpp

OBJ_DIR = obj

OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

HEADER = incl/Webserv42.hpp

CPP = c++
CPP_FLAGS = -Werror -Wextra -Wall

RM = rm -rf

all: $(NAME)

$(NAME): $(OBJ_FILES) $(HEADER)
	@$(CPP) $(CPP_FLAGS) $(OBJ_FILES) -o $(NAME)
	@echo "$(GREEN)compilation success: '$(ITALIC)webserv$(GREEN)' created$(RESET)"

$(OBJ_DIR)/%.o:$(SRC_DIR)/%.cpp
	@if [ ! -d "$(@D)" ]; then mkdir $(@D); fi
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJ_FILES)
	@$(RM) $(OBJ_DIR)
	@echo "obj files & directory $(RED)removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "webserv $(RED)removed$(RESET)"

re: fclean all

.PHONY = all, clean, fclean, re
