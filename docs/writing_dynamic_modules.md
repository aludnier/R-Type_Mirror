# Writing dynamically loaded modules

This guide explains how to add an optional subsystem implementation—such as
animation, audio, physics, or rendering—to the modular engine described in
[`docs/architecture/01_engine_core_modules.puml`](architecture/01_engine_core_modules.puml).

The important distinction is:

- the **Engine Core** is a static library and owns discovery, loading,
  initialization, replacement, and unloading;
- the **module contract** is the stable interface used by the Engine Core;
- the **module** is a shared library that implements that contract.

The game must depend on ECS contracts, never on a concrete shared library.
Every module uses the same basic module contract to communicate with the Engine Core;
the module itself contains only its ECS components and systems.

## 1. Basic module API

Every dynamically loaded module must export the common entry point below:

```cpp
extern "C" const ModuleDescriptor *RTypeLibEntryPoint();
```

`ModuleDescriptor` is the common contract between the Engine Core and every
module. When the Engine Core and modules use the same controlled C++ toolchain,
the descriptor may contain C++ types. It should identify the module and contain
at least:

- a stable module name and API major/minor version;
- the module's required dependencies;
- registration of its ECS components and systems;
- the module's registration callable.

A shared API header should define the descriptor and the interfaces passed to
the registration callback. For example:

```cpp

namespace RType {
struct ModuleApiVersion {
    std::uint32_t major;
    std::uint32_t minor;
};

struct ModuleDependency {
    std::string name;
    ModuleApiVersion compatible_api;
};

using RegisterModuleFn = std::function<ModuleStatus(Registry &registry)>;

struct ModuleDescriptor {
    std::string name;
    ModuleApiVersion apiVersion;
    std::vector<ModuleDependency> dependencies;
    RegisterModuleFn registerModule;
};
}
extern "C" const ModuleDescriptor *RTypeLibEntryPoint();
```

`ModuleStatus` must be a shared status type defined by the API header. The
descriptor is a read-only static object owned by the module; the Engine Core
must not delete it. The descriptor and all of its C++ members must remain valid
for as long as the library is loaded. The Engine Core owns
the `RType::Registry` instance and passes them to `registerModule` only after
all declared dependencies have been registered. The callback must return a
failure status if registration cannot be completed; the core then removes any
partially registered ECS types or systems before reporting the error.

The exact callable type belongs in a shared API header. Registration callbacks
return explicit status codes and receive only the ECS registry and system
scheduler interfaces defined by the engine. They must not throw exceptions
across the shared-library boundary. The Engine Core validates the descriptor,
registers the module's components and systems, and removes them before
unloading the library. A reference returned by the entry point must not be
retained after the library is closed.

## 2. Define the module contract

Put the public contract in a small, implementation-independent header. It
should contain only the ECS component data, system declarations, and
registration operations that the Engine Core needs. Keep the ECS registry and
scheduler behind carefully documented interfaces rather than exposing their
implementation details.

A contract should define:

1. a module/API name and version;
2. the ECS components and systems provided by the module;
3. component ownership, lifetime, and serialization rules;
4. system ordering, access, and thread-safety expectations;
5. the compatibility policy for future versions.

For a new subsystem, create a module (for example, animation or physics) that
defines the ECS data and systems belonging to that subsystem. It must not
contain unrelated services, resources, or runtime policy.

## 3. Keep the ABI stable

The boundary between the Engine Core and a shared library uses a controlled C++
ABI. The Engine Core and every module must use compatible compiler, standard
library, architecture, runtime-library, language-standard, and debug/release
settings. This permits C++ classes and STL containers in the descriptor, but
they must never cross an uncontrolled or third-party plugin boundary.

A conceptual entry point is the common descriptor function:

```cpp
#if defined(_WIN32)
    #define MODULE_EXPORT __declspec(dllexport)
    #define MODULE_CALL __cdecl
#else
    #define MODULE_EXPORT __attribute__((visibility("default")))
    #define MODULE_CALL
#endif

extern "C" MODULE_EXPORT const ModuleDescriptor &MODULE_CALL
RTypeLibEntryPoint();
```

The module should return a reference to a function-local static or namespace-
scope `const ModuleDescriptor`. The Engine Core borrows this object and must
not destroy it. Returning a newly allocated descriptor would introduce an
allocator and ownership problem across the shared-library boundary.

The Engine Core must validate the descriptor before calling the registration
function:

- the entry point exists;
- the API major version is supported;
- the module name and dependencies are valid;
- the registration callback is non-null;
- the registration callable is valid.

The descriptor, its C++ members, and its callback must remain valid until the
shared library is closed. The reference becomes dangling as soon as the
library is unloaded.

## 4. Implement the module

Create a separate module directory containing at least:

```text
modules/<name>/
├── CMakeLists.txt
├── include/<name>/<name>_api.h   # only if the API is module-specific
└── src/
    ├── <name>_module.cpp
    └── ...
```

The implementation should:

1. define its ECS component types and systems;
2. register them through the public ECS interfaces;
3. perform work only through the ECS API and registered systems;
4. return explicit errors instead of throwing across the ABI boundary;
5. tolerate being unloaded and replaced without leaving systems from the old
   library registered or active.

For example, an animation module can define animation state components and an
animation system. A physics module can define body and collision components
and systems that simulate them. Neither module should implement external
services such as audio playback or resource management.

### Components and systems

A module contains the definitions of the ECS components it owns and the
systems that operate on those components. For example, an animation module can
define animation state components and register an animation system; a physics
module can define its physics components and register its simulation and
collision systems.

During registration, the module must register its component types and systems
through the public ECS registry and scheduler interfaces. Systems must use the
public ECS API, declare their ordering or scheduling requirements, and only
read or write components that their contract permits. Component serialization,
replication, and ownership rules must be documented when a component crosses
the server, client, or module boundary. The Engine Core owns the registry and
system scheduler; the module owns the definitions and behavior it registers.

Modules should keep components and systems cohesive with their subsystem. A
module may consume components defined by another module only when that module
is a declared dependency.

### Module dependencies

A module may require another module. Declare each dependency by stable module
name and compatible API version in the descriptor, rather than depending on a
shared-library filename or loading it manually. The Engine Core resolves the
dependency graph, rejects missing or incompatible dependencies, and
registers modules in dependency order. It must detect cycles and report the
full dependency chain in the diagnostic.

The required module's component and system definitions are available to the
dependent module through the shared ECS interfaces after dependency ordering
has been resolved. A dependent module must not access those definitions before
the required module has been registered. During unloading, dependents are
removed before the modules they require; a required module cannot be unloaded
while any dependent remains active.

## 5. Build the shared library

Add the implementation as a shared target rather than linking it into the
static Engine Core. The exact target names and output directories should
follow the repository’s CMake conventions. A minimal target has this shape:

```cmake
add_library(example_module SHARED
    src/example_module.cpp
)

target_include_directories(example_module PRIVATE
    ${PROJECT_SOURCE_DIR}/path/to/api/include
)

target_compile_features(example_module PRIVATE cxx_std_23)
set_target_properties(example_module PROPERTIES
    PREFIX ""
)
```

Link only the libraries required by the contract and the module itself. Do
not link the executable directly to an optional module if the purpose is to
discover it at runtime. Keep debug/release settings, architecture, compiler,
runtime-library choices, and standard-library ABI compatible with the Engine
Core. On Windows, use the same CRT configuration on both sides; do not let one
side allocate an object that the other side destroys.

Use a predictable filename and install/output location. The discovery rules
must document whether the core searches one module directory, configured
paths, or platform-specific paths. Do not silently load arbitrary libraries
from an untrusted directory.

## 6. Discover and register the module

The Engine Core owns the complete lifecycle:

```text
discover → open shared library → resolve entry point → validate descriptor
         → resolve dependencies → register ECS components and systems
         → make systems available to the scheduler
```

Discovery should be deterministic. If more than one implementation provides
the same API, select it using explicit configuration or a documented priority,
not filesystem order. Report the module path, API version, and failure reason
in the core logger.

The loader needs a platform-specific implementation behind one common
interface. Use `dlopen`, `dlsym`, and `dlclose` on POSIX systems; use
`LoadLibrary`, `GetProcAddress`, and `FreeLibrary` on Windows. The exported
entry point must use the platform export macro and calling convention from the
shared API header. Do not expose `dlfcn.h` or Windows loader headers to module
implementations.

The descriptor lookup is non-owning. A `DLLoader<ModuleDescriptor>`-style
helper should return `const ModuleDescriptor &`, not `std::unique_ptr`, because
the entry point returns a reference to a module-owned static object:

```cpp
const T &get_descriptor();
```

The loader must remain alive while this reference, any descriptor member, or
any callback and system originating from the module is in use. A missing entry
point or invalid descriptor should be reported as a loader error rather than
represented by a null reference.

Registration receives only the ECS registry and system scheduler interfaces.
The core must not register a module before its dependencies are ready. A
registration failure must roll back every component and system added by that
attempt.

## 7. Use the module from the game

Game code should use the ECS components and systems registered by the Engine
Core. It should not call operating-system loading functions, include the
module’s private headers, or depend on a module filename. This preserves the
architecture’s ability to replace or hot-reload an implementation.

If a subsystem is optional, the game must handle its absence explicitly:

- disable the related feature, or use a documented fallback;
- show a useful diagnostic to the user or developer;
- do not continue with a partially registered module.

## 8. Replacement and unloading rules

Before unloading or replacing a module, the core must:

1. stop scheduling new systems from the module;
2. wait until its systems are no longer running;
3. remove dependent modules first;
4. remove the module’s systems and components from the ECS;
5. invalidate references to its definitions and storage;
6. discard every reference to the descriptor and its C++ members;
7. close the shared library only after no callback or function pointer remains
   in use.

Hot reload is safe only when the API defines how state is serialized and
restored. Otherwise, treat replacement as a full stop-and-start operation.

## Definition of done

A dynamically loaded module is ready when it has a clearly documented ECS
components and systems, a compatible shared-library target, declared
dependencies, deterministic registration, explicit rollback, and tests
proving that it can be executed and unloaded without calling code from an
unloaded library.