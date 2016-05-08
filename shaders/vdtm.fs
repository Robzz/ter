#version 330

in vec4 vs_pos_object_space;
in vec3 vs_normal;

in PovTexCoords {
    vec2 pov1;
    vec2 pov2;
    vec2 pov3;
};

uniform mat4 m_viewpoint_inverse1, m_viewpoint_inverse2, m_viewpoint_inverse3; 
uniform sampler2D depthTex1, depthTex2, depthTex3, colorTex1, colorTex2, colorTex3;

vec4 uv_to_obj(sampler2D depthTex, vec2 uv, mat4 m_inv) {
    float z = texture2D(depthTex, uv).r;
    vec4 v = m_inv * (vec4(uv, z, 1)*2 -1);
    v = v / v.w;
    return v;
}

void main() {
    vec4 p1 = uv_to_obj(depthTex1, pov1, m_viewpoint_inverse1),
         p2 = uv_to_obj(depthTex2, pov2, m_viewpoint_inverse2),
         p3 = uv_to_obj(depthTex3, pov3, m_viewpoint_inverse3);
    float d1 = distance(p1, vs_pos_object_space),
          d2 = distance(p2, vs_pos_object_space),
          d3 = distance(p3, vs_pos_object_space);
    if(d1 < d2 && d1 < d3) {
        gl_FragColor = texture2D(colorTex1, pov1);
        gl_FragDepth = p1.z;
    }
    else if(d2 < d1 && d2 < d3) {
        gl_FragColor = texture2D(colorTex2, pov2);
        gl_FragDepth = p2.z;
    }
    else {
        gl_FragColor = texture2D(colorTex3, pov3);
        gl_FragDepth = p3.z;
    }
}
