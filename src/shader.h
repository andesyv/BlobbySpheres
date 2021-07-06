#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <optional>
#include <map>
#include <glad/glad.h>
#include "constants.h"

class Shader
{
public:
    struct SubShader {
        int id;
    
    private:
        bool bOwned = true;
        
    public:
        SubShader(int _id) : id{_id} {}
        SubShader(const SubShader&) = default;
        SubShader(SubShader&& lhs) : id{lhs.id} {
            lhs.bOwned = false;
        }

        SubShader& operator=(const SubShader&) = default;
        SubShader& operator=(SubShader&& lhs) {
            id = lhs.id;
            lhs.bOwned = false;
        }

        ~SubShader() {
            if (bOwned)
                glDeleteShader(id);
        }
    };

private:
    bool bValid = false;
    int id;

    std::map<GLenum, SubShader> programs;

public:
    static std::optional<std::pair<GLenum, int>> createSubShader(const std::pair<GLenum, std::string>& program) {
        const auto& [type, relPath] = program;
        const auto path = std::string{SHADER_BASE_PATH}.append(relPath);

        // Read file:
        std::ifstream input{path, std::ifstream::in | std::ifstream::ate};
        if (!input)
        {
            std::cout << "SHADER ERROR: Path " << path << " not found." << std::endl;
            return {};
        }

        std::string source{};
        source.reserve(input.tellg());
        input.seekg(0, input.beg);
        source.insert(source.begin(), std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
        auto sourcePtr = source.c_str();
        input.close();

        // Compile shader:
        int prog = glCreateShader(type);
        glShaderSource(prog, 1, &sourcePtr, NULL);
        glCompileShader(prog);
        // check for shader compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(prog, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(prog, 512, NULL, infoLog);
            std::cout << "SHADER COMPILATION ERROR in file: \"" << path << "\"\n"
                      << infoLog << std::endl;
            return std::nullopt;
        }

        return std::make_pair(type, prog);
    }

    // Appends a program to this shaders list of subshaders. Returns true if new shader was added.
    bool addShader(const std::pair<GLenum, std::string>& program) {
        auto result = createSubShader(program);
        if (!result)
            return false;

        auto [key, value] = *result;
        programs.insert(std::move(std::make_pair(key, std::move(SubShader{value}))));
        bValid = false;
        return true;
    }

    // Attempts to link this shaers subshaders. Returns true if successfull. 
    bool link() {
        id = glCreateProgram();
        for (const auto& prog : programs)
            glAttachShader(id, prog.second.id);
            
        glLinkProgram(id);
        // check for linking errors
        int success;
        char infoLog[512];
        glGetProgramiv(id, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(id, 512, NULL, infoLog);
            std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                      << infoLog << std::endl;
            return false;
        }
        
        bValid = true;
        return true;
    }

    template <std::size_t I>
    void compileAndLink(std::array<std::pair<GLenum, std::string>, I>&& params) {
        for (auto param : params) {
            if (!addShader(param))
                return;
        }

        link();
    }

    template <std::size_t I>
    Shader(std::pair<GLenum, std::string>(&& params)[I]) {
        std::array<std::pair<GLenum, std::string>, I> arr;
        for (auto i{0}; i < I; ++i)
            arr[i] = params[i];
        compileAndLink(std::move(arr));
        // compileAndLink(std::to_array<std::pair<GLenum, std::string>, I>(params)); // C++20
    }

    // template <typename ... S>
    // Shader(S&& ... args) {
    //     compileAndLink<sizeof...(S)>(std::to_array<std::pair<GLenum, std::string>, sizeof...(S)>(static_cast<std::pair<GLenum, std::string>>(args) ...));
    // }

    int get() const { return id; }
    int operator* () const { return get(); }

    ~Shader() {
        programs.clear();

        // Delete program:
        glDeleteProgram(id);
    }
};