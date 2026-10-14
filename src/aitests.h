#pragma once
#include <functional>

// Runs entirely offline against local mock servers and temporary test settings.
void runAiSelfTests(const std::function<void(bool, const char *)> &check);
