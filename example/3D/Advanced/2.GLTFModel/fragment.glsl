#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    float time;
} ubo;

layout(binding = 1) uniform sampler2D tex;
layout(binding = 2) uniform LightingUniforms {
    vec4 directions[2];
    vec4 colors[2];
    float hardness;
    float kd;
    float ks;
} lightingUbo;

layout (location = 0) in vec3 in_viewDirection;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_normal;

layout (location = 0) out vec4 out_color;

void main()
{
    vec3 N = normalize(in_normal);
    vec3 V = normalize(in_viewDirection);
    vec3 texColor = texture(tex, in_uv).rgb; 

    vec3 color = vec3(0.0);
    for (int i = 0; i < 2; i++)
    {
        vec3 lightColor = lightingUbo.colors[i].rgb;
        vec3 L = normalize(lightingUbo.directions[i].xyz);
        vec3 R = reflect(-L, N); // equivalent to 2.0 * dot(N, L) * N - L

        vec3 diffuse = max(0.0, dot(L, N)) * lightColor;
        float RoV = max(0.0, dot(R, V));
        float specular = pow(RoV, lightingUbo.hardness);

        vec3 ambient = vec3(0.05);
        color += texColor * lightingUbo.kd * diffuse + lightingUbo.ks * specular + ambient;
    }

    out_color = vec4(color, 1.0);
}