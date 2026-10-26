/*
** EPITECH PROJECT, 2026
** RType
** File description:
** DESCRIPTION
*/

#ifndef DLLOADER_HPP
#define DLLOADER_HPP
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <memory>
#include <string>
#include <utility>

namespace RType {
template<typename T>
class DLLoader {
public:
    explicit DLLoader(const std::string &libFile) :
        _handle(nullptr)
    {
        this->loadFile(libFile);
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

    void close() noexcept
    {
        if (this->_handle) {
            closeLibrary(this->_handle);
            this->_handle = nullptr;
        }
    }

    void loadFile(const std::string &libFile)
    {
        this->close();
        this->_handle = openLibrary(libFile);
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

    void *openLibrary(const std::string &libFile)
    {
#if defined(_WIN32) || defined(_WIN64)
        return LoadLibraryA(libFile.c_str());
#else
        return dlopen(libFile.c_str(), RTLD_LAZY);
#endif
    }

    void closeLibrary(void *handle)
    {
#if defined(_WIN32) || defined(_WIN64)
        FreeLibrary((HMODULE) handle);
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
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buffer,
            sizeof(buffer),
            NULL);
        return std::string(buffer);
#else
        return dlerror();
#endif
    }

    void *getLibraryFunction(std::string_view funcName)
    {
#if defined(_WIN32) || defined(_WIN64)
        return (void *) GetProcAddress(
            (HMODULE) this->_handle, funcName.data());
#else
        return dlsym(this->_handle, funcName.data());
#endif
    }
};
} // namespace RType

#endif
