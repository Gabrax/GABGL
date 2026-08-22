cbuffer PhysicsDebugConstants : register(b0)
{
  float4x4 ViewProjection;
  float4x4 Model;
  float4 Color;
};
struct VSInput { float3 position : POSITION; };
float4 VSMain(VSInput input) : SV_POSITION
{
  return mul(ViewProjection, mul(Model, float4(input.position, 1.0f)));
}
float4 PSMain() : SV_TARGET { return Color; }