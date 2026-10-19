/*
** EPITECH PROJECT, 2026
** RType
** File description:
** DESCRIPTION
*/

#ifndef DLLOADER_HPP
    #define DLLOADER_HPP
    #include <dlfcn.h>
    #include <memory>
    #include <string>
    #include <utility>

namespace RType {
    template<typename T>
    class DLLoader {
    public:
        explicit DLLoader(const std::string &libFile) : _handle(nullptr)
        {
            this->loadFile(libFile);
        }

        explicit DLLoader() : _handle(nullptr)
        {
        }

        DLLoader(const DLLoader &) = delete;

        ~DLLoader()
        {
            this->close();
        }

        DLLoader &operator=(const DLLoader &) = delete;

        DLLoader(DLLoader &&other) noexcept : _handle(other._handle)
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
            void *entryPoint = dlsym(this->_handle, ENTRY_POINT_SYMBOL.data());
            if (!entryPoint)
                throw NoEntrypointException(
                    std::string("Failed to open ") + ENTRY_POINT_SYMBOL.data() +
                    ": " + dlerror());

            using EntryPointFn = T*(*)();
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
                dlclose(this->_handle);
                this->_handle = nullptr;
            }
        }

        void loadFile(const std::string &libFile)
        {
            this->close();
            this->_handle = dlopen(libFile.c_str(), RTLD_LAZY);
            if (!this->_handle)
                throw LibraryLoadException(dlerror());
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
            explicit LibraryLoadException(std::string msg) : _libFile(std::move(
                msg))
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
            explicit NoEntrypointException(std::string msg) : _msg(std::move(msg))
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
            explicit InstanceCreationException(std::string msg) : _msg(std::move(
                msg))
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
        static constexpr std::string_view ENTRY_POINT_SYMBOL =
            "RTypeLibEntryPoint";
        void *_handle;
    };
}

#endif
