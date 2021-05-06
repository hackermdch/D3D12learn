#include "Header.hlsli"

VertexOut main(VertexIn vIn)
{
    VertexOut vOut;
    vOut.pos = mul(vIn.pos, mul(gWorld, gViewProj));
    vOut.color = vIn.color;
    return vOut;
}