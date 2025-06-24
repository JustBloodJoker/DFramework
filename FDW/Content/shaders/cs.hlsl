
struct Particle
{
    float3 position;
    float velocity;
    float4 color;
};
RWStructuredBuffer<Particle> particles : register(u0);


[numthreads(1024, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    Particle p = particles[DTid.x];

    float angle = p.velocity * 0.01f;
    float3 newPosition = float3(
        cos(angle) * p.position.x - sin(angle) * p.position.y,
        sin(angle) * p.position.x + cos(angle) * p.position.y,
        p.position.z
    );

    particles[DTid.x].position = newPosition;
}