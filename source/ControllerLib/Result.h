#pragma once
#include <cstdint>

//Function error code Result type
typedef uint32_t Result;

//Checks whether a Result code indicates success
#ifndef R_SUCCEEDED
#define R_SUCCEEDED(Result) ((Result) == 0)
#endif
//Checks whether a Result code indicates failure
#ifndef R_FAILED
#define R_FAILED(Result) ((Result) != 0)
#endif