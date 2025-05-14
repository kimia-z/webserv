# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: kziari <kziari@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/05/05 10:26:11 by mstencel          #+#    #+#              #
#    Updated: 2025/05/14 14:06:55 by kziari           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

RESET = \033[0m
GREEN = \033[38;5;82m
RED = \033[31;1m
ITALIC = \033[3m

NAME = webserv

SRC_FILES = main.cpp ServerMain.cpp
OBJ_DIR = obj
OBJ_FILES = $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

HEADER = incl/Webserv42.hpp

CPP = c++
CPP_FLAGS = -Werror -Wextra -Wall

RM = rm -rf

all: $(NAME)

$(NAME): $(OBJ_FILES) $(HEADER)
	@$(CPP) $(CPP_FLAGS) $(OBJ_FILES) -o $(NAME)
	@echo "$(GREEN)compilation success: '$(ITALIC)$(NAME)$(GREEN)' created$(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@if [ ! -d "$(@D)" ]; then mkdir -p $(@D); fi
	@$(CPP) $(CPP_FLAGS) -c $< -o $@

clean:
	@$(RM) $(OBJ_FILES)
	@$(RM) $(OBJ_DIR)
	@echo "obj files & directory $(RED)removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(NAME) $(RED)removed$(RESET)"

re: fclean all

.PHONY: all clean fclean re
