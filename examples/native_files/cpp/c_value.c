#include "c_value.h"
int32_t c_value(void) { return _Generic(1, int: 23, default: 0); }
