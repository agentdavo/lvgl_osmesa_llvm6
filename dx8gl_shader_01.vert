#version 330 core

in vec3 a_position;  // XYZ - world coordinates
in vec3 a_normal;
in vec4 a_color;

uniform mat4 u_world;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_worldViewProj;
uniform mat3 u_normalMatrix;

out vec4 v_color;
out vec3 v_normal;
out vec3 v_worldPos;

void main() {
    vec4 worldPos = u_world * vec4(a_position, 1.0);
    gl_Position = u_worldViewProj * vec4(a_position, 1.0);
    v_color = a_color;
    v_normal = normalize(u_normalMatrix * a_normal);
    v_worldPos = worldPos.xyz;
}
