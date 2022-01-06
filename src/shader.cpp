#include "shader.h"
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <sstream>
#include <ranges>
#include <iostream>

#include <glad/glad.h>

#include "utils.h"

static unsigned int debugMessageId = 0;

Shader::SubShader::SubShader(int _id, const std::string& path)
 : id{_id}, filePath{path}
{}

Shader::SubShader::SubShader(SubShader&& lhs)
 : id{lhs.id}, filePath{std::move(lhs.filePath)} {
    lhs.bOwned = false;
}

Shader::SubShader& Shader::SubShader::operator=(SubShader&& lhs) {
    id = lhs.id;
    filePath = std::move(lhs.filePath);
    lhs.bOwned = false;
    return *this;
}

Shader::SubShader::~SubShader() {
    if (bOwned)
        glDeleteShader(id);
}




std::optional<std::pair<GLenum, int>> Shader::createSubShader(const std::pair<GLenum, std::string>& program, const std::string& programDefines) {
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
    const auto n = static_cast<std::size_t>(input.tellg());
    source.reserve(n + programDefines.size());
    input.seekg(0, input.beg);
    // 1. Discard first comment lines (glsl requires #version to be the first line, but this way I can still add file-comments)
    for (std::string line; input && input.peek() != '#';)
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // 1. Read first few #define lines in shader
    for (std::string line; input && input.peek() == '#';) {
        std::getline(input, line);
        source += line + '\n';
    }
    // 2. Add defines:
    source += programDefines;
    // 3. Read rest of file:
    source.insert(source.end(), std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});
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

// Compiles and appends a program to this shaders list of subshaders. Returns true if new shader was added.
bool Shader::addShader(const std::pair<GLenum, std::string>& program) {
    auto result = createSubShader(program, getDefineStr());
    if (!result)
        return false;

    auto [key, value] = *result;
    programs.insert(std::move(std::make_pair(key, std::move(SubShader{value, program.second}))));
    bValid = false;
    return true;
}

void Shader::addDefine(const std::string& value) {
    defines.insert(value);
    bValid = false;
}

void Shader::addDefine(std::string&& value) {
    defines.insert(std::move(value));
    bValid = false;
}

std::string Shader::getDefineStr() const {
    std::string output{};
    for (const auto& s : defines)
        output.append(std::format("#define {}\n", s));
    return output;
}

// Attempts to link this shaers subshaders. Returns true if successfull. 
bool Shader::link() {
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
        
        glDeleteProgram(id);
        return false;
    }

    // Print debug info:
    std::stringstream shaderIdentifier;
    std::size_t i{0};
    for (auto it{programs.begin()}; it != programs.end(); ++it, ++i)
        shaderIdentifier << it->second.filePath << (i == programs.size() - 1 ? "" : ", ");
    const auto debugMessage = std::format("Shader {{{}}} successfully compiled with id {}", shaderIdentifier.str(), id);
    glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER, ++debugMessageId, GL_DEBUG_SEVERITY_NOTIFICATION, debugMessage.size(), debugMessage.c_str());
    
    bValid = true;
    return true;
}

bool Shader::reload() {
    const auto& defines = getDefineStr();
    const auto keys = util::collect(programs | std::ranges::views::transform([](const auto& pair){ return pair.first; }));

    if (bOwned && bValid)
        glDeleteProgram(id);

    bValid = false;

    // Recreate and reinstert subshaders:
    for (const auto& key : keys) {
        auto node = std::move(programs.extract(key));
        const auto& filePath = node.mapped().filePath;

        auto result = createSubShader(std::make_pair(key, filePath), defines);
        if (!result)
            return false;
        
        const auto id = result->second;
        node.mapped() = std::move(SubShader{id, filePath});
        programs.insert(std::move(node));
    }

    // Relink shader
    return link();
}

Shader::Shader(Shader&& rhs) noexcept
 : id{rhs.id}, bValid{rhs.bValid}, programs{std::move(rhs.programs)}, defines{std::move(rhs.defines)}
{
    rhs.bOwned = false;
}

Shader& Shader::operator=(Shader&& rhs) noexcept {
    id = rhs.id;
    bValid = rhs.bValid;
    programs = std::move(rhs.programs);
    defines = std::move(rhs.defines);
    rhs.bOwned = false;
    return *this;
}

Shader::~Shader() {
    programs.clear();

    // Delete program:
    if (bOwned && bValid)
        glDeleteProgram(id);
}