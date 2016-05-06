#version 330

layout(row_major, std140) uniform Viewpoints {
    mat4 position;
    sampler2D colorTex;
    sampler2D normalTex;
    sampler2D depthTex;
} pov[3];

void main() {
    gl_FragColor = (1,1,1,1);
}
