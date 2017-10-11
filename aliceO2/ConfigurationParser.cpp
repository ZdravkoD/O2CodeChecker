//===--- ConfigurationParser.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------===//

#include "ConfigurationParser.h"
#include <regex>
#include <string>
#include <ctype.h>
#include <cstdio>
#include <sys/file.h>

namespace clang {
namespace tidy {
namespace aliceO2 {

ConfigurationParser::ConfigurationParser()
  : defaultValue("<provide_value>")
{
}

void ConfigurationParser::init(std::string configFilePath, std::string checkName)
{
  initInner(configFilePath, checkName, FileLockType::fltLockFile);
}

/// Reads the configuration file at configFilePath and populates a map for each configuration option
/// that adresses the checkName.
void ConfigurationParser::initInner(std::string configFilePath, std::string checkName, FileLockType fileLockType)
{
  this->configFilePath = configFilePath;
  this->checkName = checkName;

  FILE *work=fopen(configFilePath.c_str(),"r");
  if(!work)
  {
    return;
  }
  if(fileLockType == FileLockType::fltLockFile)
  {
    flock(fileno(work), LOCK_EX);
  }

  char temp[1024];
  while(fscanf(work,"%s",temp) != EOF)
  {
    std::string currentWord(temp);

    if(currentWord == "key:")
    {
      fscanf(work,"%s",temp);
      std::string key(temp);
      // skipping "value:" word with "%*s" token and reading the value itself
      fscanf(work,"%*s%s",temp);
      std::string value(temp);
      // strip quotes around value
      value = value.substr(1,value.size()-2);

      // if key has the format: checkName.SOME_KEY
      if(key.compare(0, checkName.size()+1, checkName+".") == 0)
      {
        std::string strippedKey = key.substr(checkName.size()+1, key.size());
        configKeyToValue[strippedKey] = value;
      }
    }
  }

  if(fileLockType == FileLockType::fltLockFile)
  {
    flock(fileno(work), LOCK_UN);
  }
  fclose(work);
}

/// Gets the value of the pre-populated map that corresponds to the provided key.
std::string ConfigurationParser::getValue(std::string key)
{
  if(configKeyToValue.count(key) == 0 || configKeyToValue[key] == defaultValue)
  {
    return "";
  }
  return configKeyToValue[key];
}

/// Writes the key to the provided configFilePath with corresponding empty value.
void ConfigurationParser::writeKey(std::string key, std::string reasonToFix)
{
  if(configFilePath == "" || configKeyToValue.count(key))
  {
    return;
  }

  FILE *work = fopen(configFilePath.c_str(),"a");
  flock(fileno(work), LOCK_EX);

  initInner(configFilePath, checkName, FileLockType::fltNoLockFile);

  if(configKeyToValue.count(key))
  {
    flock(fileno(work), LOCK_UN);
    fclose(work);

    return;
  }
  configKeyToValue[key]=defaultValue; // create key if it doesn't exist

  fprintf(work, "key: %s.%s\n"
                "value: '%s'\n"
                "reason: %s\n\n",
    checkName.c_str(),
    key.c_str(),
    defaultValue.c_str(),
    reasonToFix.c_str());

  flock(fileno(work), LOCK_UN);
  fclose(work);
}

} // namespace aliceO2
} // namespace tidy
} // namespace clang
