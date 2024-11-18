// SPDX-FileCopyrightText: © 2024 Allegro DVT <github-ip@allegrodvt.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <stdexcept>
#include <string>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>

extern "C"
{
#include "lib_rtos/types.h"
}

template<typename T>
std::string type_name(void)
{
  std::unordered_map<std::type_index, std::string> type_names;

  type_names[std::type_index(typeid(int8_t))] += "int8_t,";
  type_names[std::type_index(typeid(uint8_t))] += "uint8_t,";

  type_names[std::type_index(typeid(int16_t))] += "int16_t,";
  type_names[std::type_index(typeid(uint16_t))] += "uint16_t,";

  type_names[std::type_index(typeid(int32_t))] += "int32_t,";
  type_names[std::type_index(typeid(uint32_t))] += "uint32_t,";

  type_names[std::type_index(typeid(int64_t))] += "int64_t";
  type_names[std::type_index(typeid(uint64_t))] += "uint64_t";

  type_names[std::type_index(typeid(float))] += "float";
  type_names[std::type_index(typeid(double))] += "double";

  type_names[std::type_index(typeid(AL_VADDR))] += " (AL_VADDR)";
  type_names[std::type_index(typeid(AL_PADDR))] += " (AL_PADDR)";

  try
  {
    /* if it throws, it means that the types_name is missing above */
    return type_names.at(std::type_index(typeid(T)));
  }
  catch(std::exception const& error)
  {
    throw std::runtime_error("Missing typename for: " + std::string(typeid(T).name()));
  }
}
