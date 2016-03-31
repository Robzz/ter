#version 330

in vec3 vs_normal;

void main() {
    gl_FragColor = vec4(normalize(vs_normal), 1);
}
