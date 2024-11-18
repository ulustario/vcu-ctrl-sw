// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <string>
#include <regex>

/****************************************************************************/
void formatFolderPath(std::string& folderPath);

/****************************************************************************/
std::string combinePath(std::string const& folder, std::string const& filename);

/****************************************************************************/
std::string createFileNameWithID(std::string const& path, std::string const& motif, std::string const& extension, int iFrameID);

/****************************************************************************/
bool checkFolder(std::string folderPath);

/****************************************************************************/
bool checkFileAvailability(std::string folderPath, std::regex const& regex);

/****************************************************************************/
#define FROM_HEX_ERROR -1
int FromHex2(char a, char b);

/****************************************************************************/
int FromHex4(char a, char b, char c, char d);
