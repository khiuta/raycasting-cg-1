#version 330 core
layout (location = 0) in vec3 position;

out vec3 aColor;

uniform mat4 u_MVP;

void main()
{
    gl_Position = u_MVP * vec4(position, 1.0);
    aColor = vec3(sin(position.x/10), cos(position.y/1), sin(position.z));
}