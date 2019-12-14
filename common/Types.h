#pragma once

#include <vector>

using RawDataType = std::vector<char>;

enum class ExecutionPolicy { Sync = 0, Async };

using ClassHandle = int;
using RequestId = int;
