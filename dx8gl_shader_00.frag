#version 330 core

in vec4 v_color;
in vec2 v_texcoord0;
in vec3 v_normal;
in vec3 v_worldPos;

uniform vec4 u_materialAmbient;
uniform vec4 u_materialDiffuse;
uniform vec4 u_materialSpecular;
uniform vec4 u_materialEmissive;
uniform float u_materialPower;
uniform vec4 u_ambientLight;
uniform vec3 u_lightPos0;
uniform vec4 u_lightDiffuse0;
uniform vec3 u_lightPos1;
uniform vec4 u_lightDiffuse1;
uniform sampler2D u_texture0;

out vec4 fragColor;

void main() {
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    color = v_color.bgra;
    vec3 normal = normalize(v_normal);
    vec3 lightColor = u_ambientLight.rgb * u_materialAmbient.rgb;
    {
        vec3 lightDir = normalize(u_lightPos0 - v_worldPos);
        float diff = max(dot(normal, lightDir), 0.0);
        lightColor += diff * u_lightDiffuse0.rgb * u_materialDiffuse.rgb;
    }
    {
        vec3 lightDir = normalize(u_lightPos1 - v_worldPos);
        float diff = max(dot(normal, lightDir), 0.0);
        lightColor += diff * u_lightDiffuse1.rgb * u_materialDiffuse.rgb;
    }
    color.rgb *= lightColor;
    vec2 modifiedTexCoord[1];
    modifiedTexCoord[0] = v_texcoord0;
    color *= texture(u_texture0, modifiedTexCoord[0]);
    fragColor = color;
}
