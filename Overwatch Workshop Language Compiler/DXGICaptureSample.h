#pragma once

#include "DXGIManager.h"

void take_screenshot(void* buffer_mem, RECT rec);
HRESULT run(RECT rec);

void init_screenshot();