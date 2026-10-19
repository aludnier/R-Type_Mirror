# Example dynamic module

This directory is a small template for a dynamically loaded ECS module. Copy
it to `modules/<name>`, rename `example` in the public header and source, then
adapt the component and system registration in `src/example_module.cpp`.

The example provides:

- `ExampleComponent`, a component owned by this module;
- one system that increments every registered value;
- a dependency-free `ModuleDescriptor`;
- the exported `RTypeLibEntryPoint()` entry point;
- a shared-library CMake target with the repository's include and link setup.

The descriptor is static and remains valid while the shared library is loaded.
The registration callback returns an explicit status and does not throw across
the module boundary. More advanced modules should extend this contract with
their dependency, serialization, scheduling, and rollback rules.