# **************************************************************************** #
#                                                                              #
#                                                         ::::::::             #
#    Makefile                                           :+:    :+:             #
#                                                      +:+                     #
#    By: kziari <kziari@student.42.fr>                +#+                      #
#                                                    +#+                       #
#    Created: 2025/05/05 10:26:11 by mstencel      #+#    #+#                  #
#    Updated: 2025/07/22 11:02:01 by mstencel      ########   odam.nl          #
#                                                                              #
# **************************************************************************** #

RESET = \033[0m
GREEN = \033[38;5;82m
RED = \033[31;1m
ITALIC = \033[3m

NAME = webserv

SRC_FILES = src/main.cpp \
			src/Cgi.cpp \
			src/ConfParser.cpp \
			src/epoll.cpp \
			src/Location.cpp \
			src/Request.cpp \
			src/Response.cpp \
			src/Server42.cpp \
			src/SingleServer.cpp

OBJ_DIR = obj
OBJ_FILES = $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

HEADER_DIR = incl

CPP = c++
CPP_FLAGS = -Werror -Wextra -Wall -I $(HEADER_DIR) -std=c++11 #delete c++11 on Linux - redundant

RM = rm -rf

all: $(NAME)

$(NAME): $(OBJ_FILES)
	@$(CPP) $(CPP_FLAGS) $(OBJ_FILES) -o $(NAME)
	@echo "$(GREEN)compilation success: '$(ITALIC)$(NAME)$(GREEN)' created$(RESET)"

$(OBJ_DIR)/%.o: %.cpp
	@if [ ! -d "$(@D)" ]; then mkdir -p $(@D); fi
	@$(CPP) $(CPP_FLAGS) -c -I $(HEADER_DIR) $< -o $@

clean:
	@$(RM) $(OBJ_FILES)
	@$(RM) $(OBJ_DIR)
	@echo "obj files & directory $(RED)removed$(RESET)"

fclean: clean
	@$(RM) $(NAME)
	@echo "$(NAME) $(RED)removed$(RESET)"

re: fclean all

.PHONY: all clean fclean re
