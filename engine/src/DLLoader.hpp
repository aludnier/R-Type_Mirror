/*
** EPITECH PROJECT, 2026
** RType
** File description:
** DESCRIPTION
*/

#ifndef DLLOADER_HPP
#define DLLOADER_HPP
#if defined(_WIN32) || defined(_WIN64)
#ifndef NOMINMAX
#define NOMINMAX // Prevents windows.h from defining min/max macros that
                 // conflict with std::min/std::max
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace RType {
template<typename T>
class DLLoader {
public:
    explicit DLLoader(const std::string &libFile, bool persistent = false) :
        _handle(nullptr)
    {
        this->loadFile(libFile, persistent);
    }

    explicit DLLoader() :
        _handle(nullptr)
    {
    }

    DLLoader(const DLLoader &) = delete;

    ~DLLoader()
    {
        this->close();
    }

    DLLoader &operator=(const DLLoader &) = delete;

    DLLoader(DLLoader &&other) noexcept :
        _handle(other._handle)
    {
        other._handle = nullptr;
    }

    DLLoader &operator=(DLLoader &&other) noexcept
    {
        if (this != &other) {
            close();
            this->_handle = other._handle;
            other._handle = nullptr;
        }
        return *this;
    }

    const T &getInstance()
    {
        void *entryPoint = this->getLibraryFunction(ENTRY_POINT_SYMBOL);
        if (!entryPoint)
            throw NoEntrypointException(std::string("Failed to open ") +
                ENTRY_POINT_SYMBOL.data() + ": " + getLibraryError());

        using EntryPointFn = T *(*) ();
        T *instance = reinterpret_cast<EntryPointFn>(entryPoint)();
        if (!instance)
            throw InstanceCreationException(
                std::string("Failed to create a new instance using ") +
                ENTRY_POINT_SYMBOL.data());
        return *instance;
    }

    /**
     * @brief Looks up an exported symbol and casts it to a function pointer.
     * @tparam Fn  Function pointer type to cast to.
     * @param name Null-terminated symbol name.
     * @return The function pointer, or nullptr if the symbol is not exported.
     */
    template<typename Fn>
        requires(std::is_pointer_v<Fn> &&
            std::is_function_v<std::remove_pointer_t<Fn>>)
    std::optional<Fn> getFunction(std::string_view name) const
    {
        void *symbol = this->getLibraryFunction(name);
        if (!symbol) {
            return std::nullopt;
        }
        return reinterpret_cast<Fn>(symbol);
    }

    void close() noexcept
    {
        if (this->_handle) {
            closeLibrary(this->_handle);
            this->_handle = nullptr;
        }
    }

    void loadFile(const std::string &libFile, bool persistent = false)
    {
        this->close();
        this->_handle = openLibrary(libFile, persistent);
        if (!this->_handle)
            throw LibraryLoadException(getLibraryError());
    }

    class DLLoaderException : public std::exception {
    public:
        [[nodiscard]] const char *what() const noexcept override
        {
            return "DLLoader exception";
        }
    };

    class LibraryLoadException : public DLLoaderException {
    public:
        explicit LibraryLoadException(std::string msg) :
            _libFile(std::move(msg))
        {
        }

        [[nodiscard]] const char *what() const noexcept override
        {
            return this->_libFile.data();
        }

    private:
        std::string _libFile;
    };

    class NoEntrypointException : public DLLoaderException {
    public:
        explicit NoEntrypointException(std::string msg) :
            _msg(std::move(msg))
        {
        }

        [[nodiscard]] const char *what() const noexcept override
        {
            return this->_msg.data();
        }

    private:
        std::string _msg;
    };

    class InstanceCreationException : public DLLoaderException {
    public:
        explicit InstanceCreationException(std::string msg) :
            _msg(std::move(msg))
        {
        }

        [[nodiscard]] const char *what() const noexcept override
        {
            return this->_msg.data();
        }

    private:
        std::string _msg;
    };

private:
    static constexpr std::string_view ENTRY_POINT_SYMBOL = "RTypeLibEntryPoint";
    void *_handle;

#if defined(_WIN32) || defined(_WIN64)
    static void addDllSearchDirectory(const std::filesystem::path &directory)
    {
        if (directory.empty())
            return;

        if (!AddDllDirectory(directory.wstring().c_str()) &&
            GetLastError() != ERROR_ALREADY_EXISTS)
            throw LibraryLoadException(
                "Could not register DLL directory: " + directory.string());
    }

    static std::filesystem::path executableDirectory()
    {
        std::wstring path(MAX_PATH, L'\0');
        DWORD length = 0;
        do {
            path.resize(path.size() * 2);
            length = GetModuleFileNameW(
                nullptr, path.data(), static_cast<DWORD>(path.size()));
        } while (length == path.size());

        if (length == 0)
            return {};
        path.resize(length);
        return std::filesystem::path(path).parent_path();
    }
#endif

    void *openLibrary(const std::string &libFile, bool persistent)
    {
#if defined(_WIN32) || defined(_WIN64)
        const auto modulePath = std::filesystem::absolute(libFile);
        addDllSearchDirectory(modulePath.parent_path());
        addDllSearchDirectory(executableDirectory());

        HMODULE handle = LoadLibraryExA(
            libFile.c_str(),
            nullptr,
            LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
                LOAD_LIBRARY_SEARCH_USER_DIRS);
        if (handle && persistent) {
            HMODULE pinned = nullptr;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN |
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                reinterpret_cast<LPCSTR>(handle),
                &pinned);
        }
        return handle;
#else
        const int flags =
            persistent ? RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE : RTLD_LAZY;
        return dlopen(libFile.c_str(), flags);
#endif
    }

    void closeLibrary(void *handle)
    {
#if defined(_WIN32) || defined(_WIN64)
        FreeLibrary(static_cast<HMODULE>(handle));
#else
        dlclose(handle);
#endif
    }

    std::string getLibraryError()
    {
#if defined(_WIN32) || defined(_WIN64)
        DWORD dw = GetLastError();
        if (dw == 0)
            return "Unknown error";
        char buffer[1024];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buffer,
            sizeof(buffer),
            nullptr);
        return std::string(buffer);
#else
        return dlerror();
#endif
    }

    [[nodiscard]] void *getLibraryFunction(std::string_view funcName) const
    {
#if defined(_WIN32) || defined(_WIN64)
        return (void *) GetProcAddress(
            static_cast<HMODULE>(this->_handle), funcName.data());
#else
        return dlsym(this->_handle, funcName.data());
#endif
    }
};
} // namespace RType

#endif
