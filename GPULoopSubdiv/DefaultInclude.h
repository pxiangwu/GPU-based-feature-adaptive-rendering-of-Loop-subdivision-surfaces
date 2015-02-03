// Written by P.Wu, email: pxiangwu@gmail.com.
// This code is based on the implementation from Microsoft Corporation.

#pragma once

#ifndef WIN_METRO_APP

#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"

#define float4 D3DXVECTOR4
#define float3 D3DXVECTOR3
#define float2 D3DXVECTOR2

#define matrix4x4 D3DXMATRIX
#define MatrixTranspose D3DXMatrixTranspose
#define Vec3TransformCoord D3DXVec3TransformCoord
#define Vec4Transform D3DXVec4Transform

#else


#include "DirectXBase.h"
#include <d3dcompiler.h>

#include "BasicVectors.h"

using namespace DirectX;

#define float2 vec2f
#define float3 vec3f
#define float4 vec4f

#define matrix4x4 XMMATRIX

void MyMatrixTranspose(matrix4x4 *matrixOut, const matrix4x4 *matrixIn);
void MyVec3TransformCoord(float3* vecOut, const float3* vecIn, const matrix4x4 *matrixIn);
void MyVec4Transform(float4* vecOut, const float4* vecIn, const matrix4x4 *matrixIn);

#define MatrixTranspose MyMatrixTranspose
#define Vec3TransformCoord MyVec3TransformCoord
#define Vec4Transform MyVec4Transform



extern ID3D11Device* GetD3D11Device();

#endif

typedef struct {
	unsigned int x, y, z, w;
} uint4;

typedef struct {
	UINT x,y,z;
} uint3;

#ifndef EXCEPTION
	#define EXCEPTION(s) exception(std::string(__FUNCTION__).append(": ").append(s).c_str())
#endif

#ifndef SAFE_DELETE
	#define SAFE_DELETE(p)       { if (p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#ifndef V_RETURN
	#define V_RETURN(x)    { hr = (x); if( FAILED(hr) ) { return hr; } }
#endif

#ifndef V
#define V(x)           { hr = (x); }
#endif



#ifdef WIN_METRO_APP
	#include "TmpCamera.h"
#endif

#include <vector>

struct Timings {
	Timings() : m_FrameTime(0.0), m_TableCreationTime(0.0), m_ElapsedTime(0.0), m_LastFrameStartTime(0.0), m_CurrFrameStartTime(0.0) {}
	double m_FrameTime;				//Pure rendering time
	double m_TableCreationTime;		//Time required to create subdivision tables
	double m_ElapsedTime;			//Frame-to-frame elapsed time
	double m_LastFrameStartTime;	//Time when last frame 'OnRender' started
	double m_CurrFrameStartTime;	//Time when curr frame 'OnRender' started
};


//Default Camera Layout
struct CB_VS_PER_FRAME {
	matrix4x4 m_World;                  // World matrix for object
	matrix4x4 m_View;
	matrix4x4 m_Projection;
	matrix4x4 m_WorldViewProjection;    // World * View * Projection matrix

	float3 m_vLightDir;
	float m_zfzn;
};