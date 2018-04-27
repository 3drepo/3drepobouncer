/**
*  Copyright (C) 2015 3D Repo Ltd
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as
*  published by the Free Software Foundation, either version 3 of the
*  License, or (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <repo/error_codes.h>
#include <stdint.h>
#include <iostream>
#include <repo/repo_controller.h>

struct repo_op_t{
	std::string  command;
	char** args;
	uint32_t nArgcs;
};

/*
* =========== HOW TO ADD A NEW COMMAND ===========
* 1. write the command under command functions
* 2. modify performOperations to recognise your command
* 3. modify helpInfo/knownValid
*/

/**
* Get info on commands (for --help menu)
* @return returns a string of command and it's info
*/
std::string helpInfo();

/**
* Check if the command is a special one (that can be used without database info)
* @return returns true if it is a special command
*/
bool isSpecialCommand(const std::string &cmd);

/**
* Check if the command is recognised
* @returns returns the minimal # of arguments needed for this command,
*          -1 if command not recognised
*/
int32_t knownValid(const std::string &cmd);

/**
* Perform the command given in in the struct repo_op_t
* @param controller the controller to the bouncer library
* @param token      token provided by the controller after authentication
* @param command    command and it's arguments to perform
* @return returns true upon success
*/
int32_t performOperation(
	repo::RepoController *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	);

/*
* ======================== Command functions ===================
*/

/**
* Generate a particular type of stash (src/gltf/repo) for a given project
* @param controller the controller to the bouncer library
* @param token      token provided by the controller after authentication
* @param command    command and it's arguments to perform
* @return returns true upon success
*/
static int32_t generateStash(
	repo::RepoController       *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	);

/**
* Get a list of users
* @param controller the controller to the bouncer library
* @param token      token provided by the controller after authentication
* @param command    command and it's arguments to perform
* @return returns true upon success
*/
static int32_t getUserList(
	repo::RepoController       *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	);

/**
* Get database stats
* @param controller the controller to the bouncer library
* @param token      token provided by the controller after authentication
* @param command    command and it's arguments to perform
* @return returns true upon success
*/
int32_t getDBStats(
	repo::RepoController       *controller,
	const repo::RepoController::RepoToken      *token,
	const repo_op_t            &command
	);