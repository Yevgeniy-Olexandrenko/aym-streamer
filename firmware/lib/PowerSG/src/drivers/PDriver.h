#pragma once

#include "drivers/DriverEnable.h"

#ifdef USE_M328_PDRIVER
#include "drivers/parallel/m328_driver.h"
namespace PowerSG { using PDriver = m328_driver; }
#endif
