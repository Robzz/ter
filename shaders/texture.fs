#version 330

in vec2 vs_texCoord;

uniform sampler2D u_tex;

out vec4 out_fragColor;

void main() {
    out_fragColor = texture2D(u_tex, vs_texCoord);
}
