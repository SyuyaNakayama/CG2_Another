#include "Basic.hlsli"

Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

float4 main(VSOutPut input) : SV_TARGET
{
	return float4(tex.Sample(smp,input.uv));
}