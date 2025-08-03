#version 330 core

in vec4 v_color;

out vec4 fragColor;

void main() {
    vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
    color = v_color.bgra;
    fragColor = color;
}
