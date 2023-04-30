#pragma once

#include <string>

/*
 * Преобразовать 4 байта в формате big endian в int
 */
int BytesToInt(std::string_view bytes);

std::string CalculateSHA1(const std::string& msg);
