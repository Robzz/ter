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

vec4 uv_to_obj(vec2 uv, float z, mat4 m_inv) {
    uv = uv*2 - 1;
    vec4 v = m_inv * vec4(uv, z, 1);
    v = v / v.w;
    return v;
}

void main() {
    float z1 = texture2D(depthTex1, pov1).r,
          z2 = texture2D(depthTex2, pov2).r,
          z3 = texture2D(depthTex3, pov3).r;
    vec4 p1 = uv_to_obj(pov1, z1, m_viewpoint_inverse1),
         p2 = uv_to_obj(pov2, z2, m_viewpoint_inverse2),
         p3 = uv_to_obj(pov3, z3, m_viewpoint_inverse3);
    float d1 = distance(p1, vs_pos_object_space),
          d2 = distance(p2, vs_pos_object_space),
          d3 = distance(p3, vs_pos_object_space);
    gl_FragDepth = gl_FragCoord.z;
    if(d1 < d2 && d1 < d3) {
        gl_FragColor = vec4(gl_FragDepth, 0, 0, 1);
        //gl_FragDepth = z1;
    }
    else if(d2 < d1 && d2 < d3) {
        gl_FragColor = vec4(0, gl_FragDepth, 0, 1);
        //gl_FragDepth = z2;
    }
    else {
        gl_FragColor = vec4(0, 0, gl_FragDepth, 1);
        //gl_FragDepth = z3;
    }
}
