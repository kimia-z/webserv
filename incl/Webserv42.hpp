/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv42.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kziari <kziari@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/05 10:30:28 by mstencel          #+#    #+#             */
/*   Updated: 2025/07/15 12:57:57 by kziari           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV42
# define WEBSERV42


#include "Server42.hpp"
#include "SingleServer.hpp"
#include "Cgi.hpp"
#include "ConfParser.hpp"
#include "Location.hpp"
#include "HttpException.hpp"
#include "Request.hpp"
#include "Router.hpp"

//headers for the main.cpp
#include <fstream> //for ifstream
#include <sstream> //for stringstream
#include <vector> //for vector

#endif

// TODO - check if all the setters and getters are actually needed