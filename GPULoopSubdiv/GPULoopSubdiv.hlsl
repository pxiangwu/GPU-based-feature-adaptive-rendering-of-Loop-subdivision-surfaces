// Written by P.Wu, email: pxiangwu@gmail.com.

//#ifndef BEZIER_HS_PARTITION
#define BEZIER_HS_PARTITION "fractional_odd"
//#endif // BEZIER_HS_PARTITION

// The input patch size.  In this sample, it is 16 control points.
// This value should match the call to IASetPrimitiveTopology()
#define INPUT_PATCH_SIZE 12

// The output patch size.  In this sample, it is also 16 control points.
#define OUTPUT_PATCH_SIZE 12

#define MAX_VALENCE 25
#define M_PI 3.14159265

Buffer<float4> g_VertexBuffer : register ( t0 );
Buffer<int> g_ValenceBuffer : register ( t1 );
//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer cbPerFrame : register( b0 )
{
    matrix g_mViewProjection;
    float3 g_vCameraPosWorld;
    float  g_fTessellationFactor;
};

cbuffer cbPerFrame : register( b1 )
{
	float g_fTessFactor;
	float g_fTessFactorNext;
	uint g_uOffset;
	uint dummy;
};
//--------------------------------------------------------------------------------------
// Vertex shader section
//--------------------------------------------------------------------------------------
struct VS_CONTROL_POINT_INPUT
{
    float4 vPosition        : POSITION;
};

struct VS_CONTROL_POINT_OUTPUT
{
    float3 vPosition        : POSITION;
};


//----------------------------------------------------------------------------------------------------------------
struct VS_INPUT
{
	float4 vPosition	: POSITION;
};

struct VS_OUTPUT
{
	float4 vPosition	: SV_POSITION;
};
VS_OUTPUT VSMain( VS_INPUT Input )
{
	VS_OUTPUT Output;
	
	//Output.vPosition = Input.vPosition;
	Output.vPosition = mul( Input.vPosition, g_mViewProjection );
	
	return Output;
}

//----------------------------------------------------------------------------------------------------------------
// vertex shader

VS_CONTROL_POINT_OUTPUT BezierVS( VS_CONTROL_POINT_INPUT Input )
{
    VS_CONTROL_POINT_OUTPUT Output;

    Output.vPosition = Input.vPosition.xyz;
	 //Output.vPosition = mul( float4(Input.vPosition.xyz,1), g_mViewProjection );

    return Output;
}

//--------------------------------------------------------------------------------------
// Constant data function for the BezierHS.  This is executed once per patch.
//--------------------------------------------------------------------------------------
struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[3]             : SV_TessFactor;
    float Inside            : SV_InsideTessFactor;
};

struct HS_OUTPUT
{
    float3 vPosition           : BEZIERPOS;
};

// This constant hull shader

HS_CONSTANT_DATA_OUTPUT BezierConstantHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> ip,
                                          uint PatchID : SV_PrimitiveID )
{    
    HS_CONSTANT_DATA_OUTPUT Output;

    float TessAmount = g_fTessFactor;
	//float TessAmount =  4.0f;

    Output.Edges[0] = Output.Edges[1] = Output.Edges[2]  = TessAmount;
    Output.Inside =  TessAmount;

    return Output;
}

// The hull shader

[domain("tri")]
[partitioning(BEZIER_HS_PARTITION)]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("BezierConstantHS")]
HS_OUTPUT BezierHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> p, 
                    uint i : SV_OutputControlPointID,
                    uint PatchID : SV_PrimitiveID )
{
    HS_OUTPUT Output;
    Output.vPosition = p[i].vPosition;
    return Output;
}

//--------------------------------------------------------------------------------------
// Bezier evaluation domain shader section
//--------------------------------------------------------------------------------------
struct DS_OUTPUT
{
    float4 vPosition        : SV_POSITION;
};

//--------------------------------------------------------------------------------------
float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4( invT * invT * invT,
                   3.0f * t * invT * invT,
                   3.0f * t * t * invT,
                   t * t * t );
}

//--------------------------------------------------------------------------------------
float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4( -3 * invT * invT,
                   3 * invT * invT - 6 * t * invT,
                   6 * t * invT - 3 * t * t,
                   3 * t * t );
}

//--------------------------------------------------------------------------------------
float3 EvaluateBezier( const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> bezpatch,
                       float4 BasisU,
                       float4 BasisV )
{
    float3 Value = float3(0,0,0);
    Value  = BasisV.x * ( bezpatch[0].vPosition * BasisU.x + bezpatch[1].vPosition * BasisU.y + bezpatch[2].vPosition * BasisU.z + bezpatch[3].vPosition * BasisU.w );
    Value += BasisV.y * ( bezpatch[4].vPosition * BasisU.x + bezpatch[5].vPosition * BasisU.y + bezpatch[6].vPosition * BasisU.z + bezpatch[7].vPosition * BasisU.w );
    Value += BasisV.z * ( bezpatch[8].vPosition * BasisU.x + bezpatch[9].vPosition * BasisU.y + bezpatch[10].vPosition * BasisU.z + bezpatch[11].vPosition * BasisU.w );
    Value += BasisV.w * ( bezpatch[12].vPosition * BasisU.x + bezpatch[13].vPosition * BasisU.y + bezpatch[14].vPosition * BasisU.z + bezpatch[15].vPosition * BasisU.w );

    return Value;
}

float3 boxSpline(  const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> bezpatch, float u, float v, float w)
{
	float3 value = float3(0, 0, 0);
	value += bezpatch[0].vPosition * ( u*u*u*u + 2*u*u*u*v);
	value += bezpatch[1].vPosition * (u*u*u*u + 2*u*u*u*w);
	value += bezpatch[2].vPosition * (u*u*u*u + 2*u*u*u*w + 6*u*u*u*v + 6*u*u*v*w + 12*u*u*v*v + 6*u*v*v*w + 6*u*v*v*v + 2*v*v*v*w + v*v*v*v);
	value += bezpatch[3].vPosition * (6*u*u*u*u + 24*u*u*u*w + 24*u*u*w*w + 8*u*w*w*w + w*w*w*w + 24*u*u*u*v + 60*u*u*v*w + 36*u*v*w*w + 6*v*w*w*w + 24*u*u*v*v + 36*u*v*v*w + 12*v*v*w*w + 8*u*v*v*v + 6*v*v*v*w + v*v*v*v);
	value += bezpatch[4].vPosition * (u*u*u*u + 6*u*u*u*w + 12*u*u*w*w + 6*u*w*w*w + w*w*w*w + 2*u*u*u*v + 6*u*u*v*w + 6*u*v*w*w + 2*v*w*w*w); 
	value += bezpatch[5].vPosition * (2*u*v*v*v + v*v*v*v);
	value += bezpatch[6].vPosition * (u*u*u*u + 6*u*u*u*w + 12*u*u*w*w + 6*u*w*w*w + w*w*w*w + 8*u*u*u*v + 36*u*u*v*w + 36*u*v*w*w + 8*v*w*w*w + 24*u*u*v*v + 60*u*v*v*w + 24*v*v*w*w + 24*u*v*v*v + 24*v*v*v*w + 6*v*v*v*v);
	value += bezpatch[7].vPosition * (u*u*u*u + 8*u*u*u*w + 24*u*u*w*w + 24*u*w*w*w + 6*w*w*w*w + 6*u*u*u*v + 36*u*u*v*w + 60*u*v*w*w + 24*v*w*w*w + 12*u*u*v*v + 36*u*v*v*w + 24*v*v*w*w + 6*u*v*v*v + 8*v*v*v*w + v*v*v*v);
	value += bezpatch[8].vPosition * (2*u*w*w*w + w*w*w*w);
	value += bezpatch[9].vPosition * (2*v*v*v*w + v*v*v*v);
	value += bezpatch[10].vPosition * (2*u*w*w*w + w*w*w*w + 6*u*v*w*w + 6*v*w*w*w + 6*u*v*v*w + 12*v*v*w*w + 2*u*v*v*v + 6*v*v*v*w + v*v*v*v);
	value += bezpatch[11].vPosition * (w*w*w*w + 2*v*w*w*w);
	value = value * 0.0833333333f;
	return value;
}
// The domain shader

[domain("tri")]
DS_OUTPUT BezierDS( HS_CONSTANT_DATA_OUTPUT input, 
                    float3 bary : SV_DomainLocation,
                    const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> bezpatch )
{
    //float4 BasisU = BernsteinBasis( bary.x );
    //float4 BasisV = BernsteinBasis( bary.y );
    //float4 dBasisU = dBernsteinBasis( bary.x );
    //float4 dBasisV = dBernsteinBasis( bary.y );

    //float3 WorldPos = EvaluateBezier( bezpatch, BasisU, BasisV );
    //float3 Tangent = EvaluateBezier( bezpatch, dBasisU, BasisV );
    //float3 BiTangent = EvaluateBezier( bezpatch, BasisU, dBasisV );
    //float3 Norm = normalize( cross( Tangent, BiTangent ) );

	float3 WorldPos = boxSpline( bezpatch, bary.x, bary.y, bary.z);
    DS_OUTPUT Output;
    Output.vPosition = mul( float4(WorldPos,1), g_mViewProjection );
    //Output.vWorldPos = WorldPos;
    //Output.vNormal = Norm;

    return Output;    
}

//--------------------------------------------------------------------------------------
// Smooth shading pixel shader section
//--------------------------------------------------------------------------------------

// The pixel shader 

float4 SolidColorPS( DS_OUTPUT Input ) : SV_TARGET
{
    // Return a solid green color
    return float4( 0, 1, 0, 1 );
}

float4 PSLinesGreen( ) : SV_TARGET
{
	return  float4( 0, 1, 0, 1 );
}

float4 PSLinesRed( ) : SV_TARGET
{
	return float4( 1, 0, 0, 1 );
}

float4 PSLinesBlue( ) : SV_TARGET
{
	return float4( 0, 0, 1, 1 );
}


VS_OUTPUT VSMainLimit( VS_INPUT Input, uint vID : SV_VertexID )
{
	VS_OUTPUT Output;
	uint valence = (uint)g_ValenceBuffer[ vID * ( MAX_VALENCE + 1 )];
	float fn = (float)valence;
	float alpha = 0.625f - ( 0.375f + 0.25f * cos( 2 * M_PI / fn ) )* ( 0.375f + 0.25f * cos( 2 * M_PI / fn ) );
	float wn = 0.375f * fn / alpha;

	Output.vPosition.xyz = Input.vPosition.xyz * wn;
	for(uint i=1; i<=valence; i++)
	{
		uint idx = g_ValenceBuffer[ vID * ( MAX_VALENCE + 1 ) + i];
		float3 neighbor = g_VertexBuffer[idx + g_uOffset].xyz;
		Output.vPosition.xyz +=  neighbor;
	}
	Output.vPosition.xyz = Output.vPosition.xyz / ( fn + wn);
	Output.vPosition = mul( float4(Output.vPosition.xyz, 1.0f ), g_mViewProjection );
	return Output;
}





HS_CONSTANT_DATA_OUTPUT boxSplineConstantHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> ip,
                                          uint PatchID : SV_PrimitiveID )
{
	HS_CONSTANT_DATA_OUTPUT Output;

	float TessAmount = g_fTessFactor;
	float TessNext = g_fTessFactorNext;

#ifdef	CASE00
	float side = sqrt(0.75) * TessAmount;
	Output.Edges[0] = TessAmount;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = side;
	Output.Inside = TessAmount;
#endif
#ifdef CASE01
	float side = sqrt(0.75) * TessAmount;
	Output.Edges[0] = side;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = TessAmount;
	Output.Inside = TessAmount;
#endif

#ifdef CASE10
	Output.Edges[0] = TessAmount/2.0;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = TessAmount/2.0;
	Output.Inside = 0.33 * (Output.Edges[0] + Output.Edges[1] + Output.Edges[2]);
#endif
#ifdef CASE11
	float side = sqrt(0.75) * TessAmount;
	Output.Edges[0] = TessAmount/2.0;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = side;
	Output.Inside = 0.33 * (Output.Edges[0] + Output.Edges[1] + Output.Edges[2]);
#endif
#ifdef CASE12
	float side = sqrt(0.75) * TessAmount;
	Output.Edges[0] = side;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = TessAmount;
	Output.Inside = TessAmount;
#endif

#ifdef CASE20
	Output.Edges[0] = TessAmount/2.0;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = TessAmount/2.0;
	Output.Inside = 0.33 * (Output.Edges[0] + Output.Edges[1] + Output.Edges[2]);
#endif
#ifdef CASE21
	Output.Edges[0] = TessAmount/2.0;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = TessAmount/2.0;
	Output.Inside = 0.33 * (Output.Edges[0] + Output.Edges[1] + Output.Edges[2]);
#endif
#ifdef CASE22
	Output.Edges[0] = TessAmount/2.0;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = TessAmount/2.0;
	Output.Inside = 0.33 * (Output.Edges[0] + Output.Edges[1] + Output.Edges[2]);
#endif
#ifdef CASE23
	Output.Edges[0] = TessAmount/2.0;
	Output.Edges[1] = TessAmount/2.0;
	Output.Edges[2] = TessAmount/2.0;
	Output.Inside = 0.33 * (Output.Edges[0] + Output.Edges[1] + Output.Edges[2]);
#endif

	return Output;
}


[domain("tri")]
[partitioning(BEZIER_HS_PARTITION)]
[outputtopology("triangle_cw")]
[outputcontrolpoints(OUTPUT_PATCH_SIZE)]
[patchconstantfunc("boxSplineConstantHS")]
HS_OUTPUT boxSplineHS( InputPatch<VS_CONTROL_POINT_OUTPUT, INPUT_PATCH_SIZE> p, 
                    uint i : SV_OutputControlPointID,
                    uint PatchID : SV_PrimitiveID )
{
    HS_OUTPUT Output;
    Output.vPosition = p[i].vPosition;
    return Output;
}

[domain("tri")]
DS_OUTPUT boxSplineDS( HS_CONSTANT_DATA_OUTPUT input, 
                    float3 bary : SV_DomainLocation,
                    const OutputPatch<HS_OUTPUT, OUTPUT_PATCH_SIZE> CP )
{
	float u; u = 0.0;
	float v; v = 0.0;
	float w; w = 0.0;
#ifdef CASE00
	u = bary.x/2.0;
	w = bary.x/2.0 + bary.z;
	v = 1 - u - w;
#endif
#ifdef CASE01
	u = bary.x + bary.z/2.0;
	w = bary.z/2.0;
	v = 1 - u - w;
#endif

#ifdef CASE10
	u = bary.x/2.0;
	w = 0.5 + bary.z/2.0;
	v = 1 - u - w;
#endif
#ifdef CASE11
	u = bary.x/2.0;
	w = bary.x/2.0 + bary.z/2.0;
	v = 1 - u - w;
#endif
#ifdef CASE12
	u = bary.x + bary.z/2.0;
	w = bary.z/2.0;
	v = 1 - u - w;
#endif

#ifdef CASE20
	u = bary.x/2.0;
	w = 0.5 + bary.z/2.0;
	v = 1 - u - w;
#endif
#ifdef CASE21
	u = bary.x/2.0 + bary.z/2.0;
	w = 0.5 - bary.x/2.0;
	v = 1 - u - w;
#endif
#ifdef CASE22
	u = bary.x/2.0;
	w = bary.z/2.0;
	v = 1 - u - w;
#endif
#ifdef CASE23
	u = bary.x/2.0 + 0.5;
	w = bary.z/2.0;
	v = 1 - u - w;
#endif

	float3 WorldPos = boxSpline( CP, u, v, w);
	DS_OUTPUT Output;
    Output.vPosition = mul( float4(WorldPos,1), g_mViewProjection );

	return Output;    
}
