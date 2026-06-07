#version 330

layout (location = 0) in vec4 vertex_position;
layout (location = 1) in vec4 vertex_normal;
layout (location = 2) in vec2 vertex_uv;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projMat;
uniform vec3 cameraPos;
uniform vec3 lightPos;

out vec3 v;
out vec3 n;
out vec3 l;
out vec3 h;
out vec2 frag_uv;

void main()
{
    gl_Position = projMat * viewMat * modelMat * vertex_position;

    vec3 worldPos = (modelMat * vertex_position).xyz;
    l = normalize(lightPos - worldPos);
    n = normalize((modelMat * vec4(vertex_normal.xyz, 0.0)).xyz);
    v = normalize(cameraPos - worldPos);
    h = normalize(v + l);

    frag_uv = vertex_uv;
}
