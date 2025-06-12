// SPDX-FileCopyrightText: © 2025 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <regex>

#if defined(__linux__)
#include <dirent.h>
#endif

#if defined(_WIN32)
#include "extra/dirent/include/dirent.h"
#endif

#include "lib_app/FileUtils.h"

extern "C" {
#include "lib_rtos/utils.h"
}

static const char CurrentDirectory = '.';
static const char PathSeparator = '/';
static const char WinPathSeparator = '\\';

/****************************************************************************/
void FormatFolderPath(std::string& folderPath)
{
  if(folderPath.empty())
  {
    folderPath += CurrentDirectory;
  }

  if(folderPath[folderPath.size() - 1] != PathSeparator && folderPath[folderPath.size() - 1] != WinPathSeparator)
  {
    folderPath += PathSeparator;
  }
}

/****************************************************************************/
std::string CombinePath(const std::string& folder, const std::string& filename)
{
  std::string formattedFolderPath = folder;
  FormatFolderPath(formattedFolderPath);
  return formattedFolderPath + filename;
}

/****************************************************************************/
std::string CreateFileNameWithID(const std::string& path, const std::string& motif, const std::string& extension, int32_t iFrameID)
{
  std::ostringstream filename;
  filename << motif << "_" << iFrameID << extension;
  return CombinePath(path, filename.str());
}

/****************************************************************************/
bool FolderExists(std::string folderPath)
{
  (void)folderPath;

  if(folderPath.compare("") != 0)
  {
#if defined(_WIN32)

    WIN32_FIND_DATA FindFolderData;
    HANDLE hFind;
    folderPath.resize(folderPath.find_last_not_of("\\/") + 1);

    hFind = FindFirstFile(folderPath.c_str(), &FindFolderData);

    if(hFind == INVALID_HANDLE_VALUE)
      return false;

    FindClose(hFind);
#endif

#if defined(__linux__)

    DIR* dir = opendir(folderPath.c_str());

    if(dir == NULL)
      return false;
    closedir(dir);
#endif
  }

  return true;
}

/****************************************************************************/
bool FileExists(std::string folderPath, std::regex const& file_regex)
{
  (void)file_regex;

  if(folderPath.compare("") == 0)
  {
    folderPath = ".";
  }

#if defined(_WIN32)

  WIN32_FIND_DATA FindFolderData;
  HANDLE hFind;
  FILE* fileStream;

  if(folderPath.compare(".") != 0)
    folderPath.resize(folderPath.find_last_not_of("\\/") + 1);

  hFind = FindFirstFile((folderPath + "\\*").c_str(), &FindFolderData);

  if(hFind == INVALID_HANDLE_VALUE)
    return false;

  do
  {
    std::string file(FindFolderData.cFileName);

    if(std::regex_match(file, file_regex))
    {
      std::string file_path = folderPath + "\\" + file;

      if(fopen_s(&fileStream, file_path.c_str(), "r") == 0)
      {
        FCLOSE(fileStream);
        FindClose(hFind);
        return true;
      }
    }
  }
  while(FindNextFile(hFind, &FindFolderData));

  FindClose(hFind);

#endif

#if defined(__linux__)

  if(folderPath.back() != '/')
    folderPath = folderPath + "/";

  struct dirent* entry;
  DIR* dir = opendir(folderPath.c_str());

  while((entry = readdir(dir)) != NULL)
  {
    std::string file(entry->d_name);

    if(std::regex_match(file, file_regex))
    {
      std::string file_path = folderPath + file;

      auto pFile = fopen(file_path.c_str(), "r");

      if(pFile)
      {
        FCLOSE(pFile);
        closedir(dir);
        return true;
      }
    }
  }

#endif

  return false;
}

/****************************************************************************/
bool GetFileSize(std::ifstream& fileStream, size_t& zSize)
{
  if(!fileStream.is_open())
    return false;

  auto initialPositionToRestore = fileStream.tellg();

  fileStream.seekg(0, std::ios::end);

  if(fileStream.fail())
    return false;

  zSize = fileStream.tellg();

  fileStream.seekg(initialPositionToRestore);

  return !fileStream.fail();
}

/****************************************************************************/
bool GetFileSize(std::string const& filename, size_t& zSize)
{
  std::ifstream fileStream(filename, std::ios::in);
  return GetFileSize(fileStream, zSize);
}

/****************************************************************************/
static int32_t FromHex1(char a)
{
  int32_t A = FROM_HEX_ERROR;

  if((a >= 'a') && (a <= 'f'))
    A = (a - 'a') + 10;
  else if((a >= 'A') && (a <= 'F'))
    A = (a - 'A') + 10;
  else if((a >= '0') && (a <= '9'))
    A = (a - '0');

  return A;
}

/****************************************************************************/
int32_t FromHex2(char a, char b)
{
  int32_t A = FromHex1(a);
  int32_t B = FromHex1(b);

  if(A == FROM_HEX_ERROR || B == FROM_HEX_ERROR)
    return FROM_HEX_ERROR;

  return (A << 4) + B;
}

/****************************************************************************/
int32_t FromHex4(char a, char b, char c, char d)
{
  int32_t AB = FromHex2(a, b);
  int32_t CD = FromHex2(c, d);

  if(AB == FROM_HEX_ERROR || CD == FROM_HEX_ERROR)
    return FROM_HEX_ERROR;

  return (AB << 8) + CD;
}
