#version 330

in vec3 vs_normal;
in vec3 vs_position;

uniform float ambient_intensity = 0.2;
uniform vec3 diffuseColor = vec3(0.2, 0.2, 0.6);
uniform vec3 v_lightPosition;
uniform mat4 m_world;

out vec4 fragColor;

void main() {
    vec3 ambient = ambient_intensity * diffuseColor;
    vec3 worldPos = vec3(m_world * vec4(vs_position, 1));
    vec3 L = normalize(v_lightPosition - worldPos);
    vec3 diffuse = clamp(dot(L, normalize(vs_normal)), 0, 1) * diffuseColor;
    fragColor = clamp(vec4(ambient + diffuse, 1),
                      vec4(0),
                      vec4(1));
}
