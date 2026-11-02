/*
** EPITECH PROJECT, 2026
** RType
** File description:
** nholam reader
*/

#ifndef NLOHAMREADER_H_
    #define NLOHAMREADER_H_

#include <string>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace RType {


class nlohamReader
{
public:
    nlohamReader() = default;
    nlohamReader(const std::filesystem::path &path);
    ~nlohamReader() = default;

    bool readFile(const std::filesystem::path &path); 

    template<typename T>
    [[ nodiscard ]] T get(const std::string &param) {
        return _data.at(param).get<T>();
    }

    template<typename T>
    [[ nodiscard ]] T get(const std::string &param, const T &fallback) {
        try {
            return _data.at(param).get<T>();
        } catch(const nlohmann::json::exception & _) {
            return fallback;
        }
    }

    [[ nodiscard ]] nlohmann::json operator[] (const std::string &param);

    [[ nodiscard ]] bool has(const std::string &param);

    template<typename T>
    [[ nodiscard ]] bool validate(){
        try {
            _data.get<T>();
        } catch(const nlohmann::json::exception&) {
            return false;
        }
        return true;
    }

    template<typename T>
    [[ nodiscard ]] T convertJson() {
        if (!validate<T>()) {
            throw std::runtime_error(std::string("[NlohmanReader] cannot convert json into ") + typeid(T).name());
        }
        return _data.get<T>();
    }

private:
    std::filesystem::path _file;
    nlohmann::json _data = nlohmann::json::object();
};

}

#endif
