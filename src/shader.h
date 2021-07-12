#pragma once

#include <string>
#include <iostream>
#include <optional>
#include <map>
#include <set>
#include <array>
#include <string_view>

#include <glad/glad.h>

#include "constants.h"

class Shader
{
public:
    struct SubShader {
        int id;
        std::string filePath;
    
    private:
        bool bOwned = true;
        
    public:
        SubShader(int _id, const std::string& path);
        SubShader(const SubShader&) = default;
        SubShader(SubShader&& lhs);

        SubShader& operator=(const SubShader&) = default;
        SubShader& operator=(SubShader&& lhs);

        ~SubShader();
    };

private:
    bool bOwned = true;
    bool bValid = false;
    int id;

    std::map<GLenum, SubShader> programs;
    std::set<std::string> defines;

public:
    // Reads a program from file and compiles it to a subshader. Returns subshader if successful.
    static std::optional<std::pair<GLenum, int>> createSubShader(const std::pair<GLenum, std::string>& program, const std::string& programDefines = "");

    // Compiles and appends a program to this shaders list of subshaders. Returns true if new shader was added.
    bool addShader(const std::pair<GLenum, std::string>& program);

    void addDefine(const std::string& value);
    void addDefine(std::string&& value);

    std::string getDefineStr() const;

    // Attempts to link this shaers subshaders. Returns true if successfull. 
    bool link();

    bool reload();

    template <std::size_t I>
    void compileAndLink(std::array<std::pair<GLenum, std::string>, I>&& params) {
        for (auto param : params) {
            if (!addShader(param))
                return;
        }

        link();
    }

    template <std::size_t I, std::size_t J>
    void compileAndLink(std::array<std::pair<GLenum, std::string>, I>&& params, std::array<std::string_view, J>&& globalDefines) {
        for (auto def : globalDefines)
            addDefine(std::string{def});
        
        for (auto param : params) {
            if (!addShader(param))
                return;
        }

        link();
    }

    Shader() = delete;
    Shader(const Shader&) = delete;
    Shader(Shader&& rhs);

    template <std::size_t I>
    Shader(std::pair<GLenum, std::string>(&& params)[I]) {
        compileAndLink(std::to_array<std::pair<GLenum, std::string>, I>(params)); // C++20
    }

    template <std::size_t I, std::size_t J>
    Shader(std::pair<GLenum, std::string>(&& params)[I], std::string_view (&& globalDefines)[J]) {
        compileAndLink(
            std::to_array<std::pair<GLenum, std::string>, I>(params),
            std::to_array<std::string_view, J>(globalDefines)
        );
    }

    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&& rhs);

    // template <typename ... S>
    // Shader(S&& ... args) {
    //     compileAndLink<sizeof...(S)>(std::to_array<std::pair<GLenum, std::string>, sizeof...(S)>(static_cast<std::pair<GLenum, std::string>>(args) ...));
    // }

    bool valid() const { return bValid; }
    int get() const { return id; }
    int operator* () const { return get(); }

    ~Shader();
};