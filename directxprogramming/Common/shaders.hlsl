cbuffer cbPerObject : register(b0)
{
    float4x4 model;
};

cbuffer cbPass : register(b1)
{
    float4x4 View;
    float4x4 InvView;
    float4x4 Proj;
    float4x4 InvProj;
    float4x4 ViewProj;
    float4x4 InvViewProj;
    float3 EyePosW;
    float cbPerObjectPad1;
    float2 RenderTargetSize;
    float2 InvRenderTargetSize;
    float NearZ;
    float FarZ;
    float TotalTime;
    float DeltaTime;
};

struct VertexIn
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VertexOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4x4 T(float3 v)
{
    return float4x4(
    1, 0, 0, v.x,
    0, 1, 0, v.y,
    0, 0, 1, v.z,
    0, 0, 0, 1
    );
    }

float4x4 S(float3 v)
{
    return float4x4(
    v.x, 0, 0, 0,
    0, v.y, 0, 0,
    0, 0, v.z, 0,
    0, 0, 0, 1
    );
}

float4x4 Rz(float theta)
{
    return float4x4(
    cos(theta), -sin(theta), 0, 0,
    sin(theta), cos(theta), 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
    );
}

float4x4 Ry(float theta)
{
    return float4x4(
    cos(theta), 0, sin(theta), 0,
    0, 1, 0, 0,
    -sin(theta), 0, cos(theta), 0,
    0, 0, 0, 1
    );
}

float4x4 Rx(float theta)
{
    return float4x4(
    1, 0, 0, 0,
    0, cos(theta), -sin(theta), 0,
    0, sin(theta), cos(theta), 0,
    0, 0, 0, 1
    );

}

// this is probably not right
float4x4 R(float tx, float ty, float tz)
{
    return mul(Rz(tz), mul(Ry(ty), Rx(tx)));
}


VertexOut VSMain(VertexIn vin)
{
    VertexOut vout;
	// Transform to homogeneous clip space.
    float4x4 model2 = mul(Ry(TotalTime), model);
    float4 posW = mul(float4(vin.position, 1.0f), model2);
    vout.position = mul(posW, ViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.color = vin.color;
    
    return vout;
}

float4 PSMain(VertexOut pin) : SV_TARGET
{
    return pin.color;
}