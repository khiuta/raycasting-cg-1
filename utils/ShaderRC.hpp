#ifndef SHADERRC_H
#define SHADERRC_H

// O GLAD DEVE VIR ANTES DE TUDO
#include "glad.h" 
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class ShaderRC {
public:
    
    unsigned int ID;
    
    ShaderRC(const char* vertexPath, const char* fragmentPath);
    
    ~ShaderRC();

    void use() const;

    void setBool(const std::string &name, bool value) const;
    void setInt(const std::string &name, int value) const;
    void setFloat(const std::string &name, float value) const;
    void setMat4(const std::string &name, const glm::mat4 &mat) const;
    void setVec3(const std::string &name, const glm::vec3 &vec) const;

private:
   
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif