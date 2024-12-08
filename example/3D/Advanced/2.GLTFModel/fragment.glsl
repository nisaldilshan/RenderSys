#version 460

layout(binding = 0) uniform UniformBufferObject {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    mat4 modelMatrix;
    vec4 color;
    float time;
    float _pad[3];
} ubo;

layout(binding = 1) uniform texture2D tex;
layout(binding = 2) uniform sampler s;

layout(binding = 3) uniform LightingUniforms {
    vec4 directions[2];
    vec4 colors[2];
    float hardness;
    float kd;
    float ks;
    float _pad;
} lightingUbo;

layout (location = 0) in vec3 in_color;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv;
layout (location = 3) in vec3 in_viewDirection;

layout (location = 0) out vec4 out_color;

void main()
{
    vec3 N = normalize(in_normal);
    vec3 V = normalize(in_viewDirection);
    vec3 texColor = texture(sampler2D(tex, s), in_uv).rgb; 
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

    // Gamma-correction
    vec3 corrected_color = pow(color, vec3(2.2));
    out_color = vec4(corrected_color, ubo.color.a);
}