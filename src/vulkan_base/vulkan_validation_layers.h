#pragma once
#include <vector>

inline const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
inline const bool ENABLE_VALIDATION_LAYERS = false;
#else
inline const bool ENABLE_VALIDATION_LAYERS = true;
#endif

bool check_validation_layer_support();
