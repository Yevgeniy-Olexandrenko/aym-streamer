#pragma once

#include "drivers/DriverEnable.h"

#ifdef USE_M328_DRIVER
#include "drivers/parallel/m328_driver.h"
namespace PowerSG { using DefPAccess = m328_driver; }
#endif
