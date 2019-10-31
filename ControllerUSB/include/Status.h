#pragma once
#include <cstdint>

//Function error code status type
typedef uint32_t Status;

//Checks whether a status code indicates success
#define S_SUCCEEDED(status) ((status) == 0)
//Checks whether a status code indicates failure
#define S_FAILED(status) ((status) != 0)