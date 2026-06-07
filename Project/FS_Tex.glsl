#version 330

in vec3 v;
in vec3 n;
in vec3 l;
in vec3 h;
in vec2 frag_uv;

uniform sampler2D color_map;
uniform float emissive;

float ka = 0.3;
float kd = 0.8;
float ks = 0.6;
float shininess = 32.0;

vec4 light_color    = vec4(1.0, 1.0, 1.0, 1.0);
vec4 specular_color = vec4(1.0, 1.0, 1.0, 1.0);

void main()
{
    vec4 texColor = texture(color_map, frag_uv);

    vec4 ambient  = ka * light_color;
    float diff    = max(dot(l, n), 0.0);
    vec4 diffuse  = kd * diff * light_color;
    float spec    = max(pow(dot(h, n), shininess), 0.0);
    vec4 specular = ks * spec * specular_color;

    vec4 litColor = texColor * (ambient + diffuse) + specular;
    gl_FragColor = mix(litColor, texColor, emissive);
}
