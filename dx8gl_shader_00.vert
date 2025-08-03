#version 330 core

in vec3 a_position;
in vec4 a_color;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_worldViewProj;

out vec4 v_color;

void main() {
    vec4 worldPos = u_world * vec4(a_position, 1.0);
    gl_Position = u_worldViewProj * vec4(a_position, 1.0);
    v_color = a_color;
}
