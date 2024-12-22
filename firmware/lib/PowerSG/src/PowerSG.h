#pragma once

#include <type_traits>
#include "drivers/Driver.h"

namespace PowerSG
{
    template<typename T, typename = typename std::enable_if<std::is_base_of<Driver, T>::value>::type>
    class Class : public T
    {

    };
}
