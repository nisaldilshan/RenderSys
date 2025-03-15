struct Material {
    vec4 color;
    float hardness;
    float kd;
    float ks;
    float workflow;
    float metallic;
    float roughness;
    int colorTextureSet;
    int PhysicalDescriptorTextureSet;
    int normalTextureSet;
    float _pad;
};