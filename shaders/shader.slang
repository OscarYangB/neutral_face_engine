struct VertexInput {
    float3 position;
    float3 color;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

ConstantBuffer<UniformBuffer> uniform_buffer;

struct VertexOutput {
    float4 position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOutput vert_main(VertexInput input) {
    VertexOutput output;
    output.position = mul(uniform_buffer.projection, mul(uniform_buffer.view, mul(uniform_buffer.model, float4(input.position, 1.0))));
    output.color = input.color;
    return output;
};

[shader("fragment")]
float4 frag_main(VertexOutput vertex) : SV_Target {
    float3 color = vertex.color;
    return float4(color, 1.0);
}
