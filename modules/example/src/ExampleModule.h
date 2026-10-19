/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** example_api.h
*/


#ifndef EXAMPLE_API_H
    #define EXAMPLE_API_H

#include "Module.h"

namespace RType::Example {
    struct ExampleComponent {
        std::int32_t value = 0;
    };
}

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint();

#endif // EXAMPLE_API_H
