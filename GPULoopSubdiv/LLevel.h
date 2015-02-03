// Written by P.Wu, email: pxiangwu@gmail.com.

#pragma once

#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include "DefaultInclude.h"
#include "mesh.h"
#include "PartialCase.h"

#include <set>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
//#include <cstring>
#include <cassert>
#include <list>

#define MAX_VALENCE 25

extern HRESULT CompileShaderFromFile( WCHAR* szFileName, D3D_SHADER_MACRO* pDefines, LPCSTR szEntryPoint, 
                               LPCSTR szShaderModel, ID3DBlob** ppBlobOut );

struct ObjectDescriptor {
	ObjectDescriptor() {		
		m_ObjectColor = float4(1.0f, 1.0f, 1.0f, 1.0f);
		m_BaseVertexTranslation = float3(0.0f, 0.0f, 0.0f);
		m_IsolateExtraordinary = 6;
		m_SubdivisionsInAdvance = 0;
		m_DisplacementScalar = 1.0f;
		m_UseDirectCreaseEvaluation = false;
		m_NormalizeVertexCoordinates = false;
	}

	std::string m_Filename;
	std::string m_KeyFrameBasename;
	float4 m_ObjectColor;
	float3 m_BaseVertexTranslation;
	UINT m_IsolateExtraordinary;
	UINT m_SubdivisionsInAdvance;
	float m_DisplacementScalar;
	bool m_NormalizeVertexCoordinates;
	bool m_UseDirectCreaseEvaluation;
	std::string m_ColorTexture;
	std::string m_DisplacementMap;
	std::string m_NormalMap;
};

ObjectDescriptor CreateSceneFromFile( ID3D11Device* pd3dDevice,  const WCHAR* filename);

struct CB_RUNTABLES
{
	UINT m_NumFaces; 
	UINT m_NumEdges; 
	UINT m_NumVertices; 
	UINT DEST_OFFSET_1;
	UINT DEST_OFFSET_2;
	UINT DEST_OFFSET_3;
	UINT SRC_OFFSET;
	UINT pad1;
};

struct CB_PER_LEVEL_CONSTANTS
{
	float fTessellationFactor;
	float fTessellationFactorNext;
	UINT g_offset;
	UINT dummy;
};

static UINT s_uCBPerLevelBind = 1;

using namespace qemesh;

class LLevel :
	public Mesh<float4>
{
public:
	LLevel(void);
	~LLevel(void);

	void Destroy();

	//! Loads the Mesh from an object file - returns the 0th Catmull-Clark level
	static LLevel* LoadObjFromFile(const std::string &filename);

	//! Performs one adaptive subdivision step and returns the new level
	LLevel* AdaptiveLoop();

	//! Control cage vertex buffers
	HRESULT CreateIndexBufferLines(ID3D11Device* pd3dDevice);
	HRESULT CreateIndexBufferRegularPatch(ID3D11Device* pd3dDevice);
	HRESULT CreateIndexBufferIrregularPatch(ID3D11Device* pd3dDevice);

	//! Control cage visualization
	void DrawIndexedFullPatches(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level);
	void DrawIndexedEndPatches(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level);
	void DrawIndexedPartialPatches(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level);
	void DrawIndexedEndIrregularPatches(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level);

	void DrawIndexedPartialPatchesWatertight(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level);

	//! Create Respective ParitialCases
	HRESULT CreatePartialCases(ID3D11Device* pd3dDevice, std::set<Face*> &partialPatch);
	void OrderPartialPatchesToCases(std::set<Face*> &partialPatch, std::set<Face*> &facesCase0, std::set<Face*> &facesCase1, std::set<Face*> &facesCase2);

	//Calculate the number of FullPatch, EndPatch, PartialPatch
	void CalPatches();

	//GPU Relevant
	HRESULT CreateAdaptiveLoopTables( ID3D11Device* pd3dDevice );
	void RunAdaptiveSubdivisionTablesCS(ID3D11DeviceContext* pContext, ID3D11Buffer* pcbRTCS, UINT DEST_OFFSET, ID3D11ComputeShader* pEdgePointCS, ID3D11ComputeShader* pVertexPointCS);

	std::set<Face*> m_SubdivFaces;
	std::set<TriEdge*> m_SubdivEdges;
	std::set<Vertex<float4>*> m_SubdivVertices;

	std::set<Face*> m_FullPatch; //Full Patch
	std::set<Face*> m_EndPatch;//End Full Patch
	//std::set<Face*> m_irregularPatch; //irregular Patch
	std::set<Face*> m_PartialPatch;

	PartialCase0 m_PartialCase0;
	PartialCase1 m_PartialCase1;
	PartialCase2 m_PartialCase2;

private:
	void SubdivideFace( Face *f );

	//! Vertex and index buffers for control cage rendering; these buffers are not used for the adaptive subdivision algorithm itself
	ID3D11Buffer*	m_IndexBufferFullPatch;
	UINT			m_NumIndicesFullPatch;

	ID3D11Buffer*	m_IndexBufferEndPatch;
	UINT			m_NumIndicesEndPatch;

	ID3D11Buffer*	m_IndexBufferIrregularPatch;
	UINT			m_NumIndicesIrregularPatch;

	ID3D11Buffer*	m_IndexBufferPartialPatch;
	UINT			m_NumIndicesPartialPatch;

	uint4*	m_E0_IT;	//indices into the vertex buffer;
	int*		m_V0_ITa; // valence, starting index;
	UINT*	m_V0_IT;	// indices into the vertex buffer

	ID3D11Buffer*	m_BUF_E0_IT;
	ID3D11Buffer*	m_BUF_V0_ITa;
	ID3D11Buffer*	m_BUF_V0_IT;

	ID3D11ShaderResourceView* m_SRV_E0_IT;
	ID3D11ShaderResourceView* m_SRV_V0_IT;
	ID3D11ShaderResourceView* m_SRV_V0_ITa; 

};

static LLevel* CreateMeshFromFile( const std::string &filename);

#define MAX_SUBDIVISONS 10
#define MAX_VALENCE 25
class ControlMesh
{
public:
	ControlMesh(void);
	~ControlMesh(void);
	//! Creates the control mesh based on an object descriptor; the subdivision table creation time will be returned
	HRESULT Create(ID3D11Device* pd3dDevice, const ObjectDescriptor& obd);

	//! Cleans up all allocated control mesh resources
	void Destroy();

	//! Renders all edges of the control mesh as lines
	void FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, UINT level);

	LLevel* GetLLevel(UINT i);

	//! Binds the vertex buffer of the control mesh
	void BindVertexBuffer(ID3D11DeviceContext* pd3dImmediateContext);
	//ID3D11ShaderResourceView* GetVertexBufferSRV();
	//ID3D11ShaderResourceView* GetVertexBufferSRV4Components();

	static HRESULT CreateShaders(ID3D11Device* pd3dDevice);
	static void DestroyShaders();

	//Create ValenceBuffer
	HRESULT CreateValenceBuffer(ID3D11Device* pd3dDevice,  UINT level);
	HRESULT CreateAllValenceBuffer(ID3D11Device* pd3dDevice);

	void GPUSubdivide( ID3D11DeviceContext* pd3dImmediateContext, UINT stop);

	//! Shaders of the control mesh; shaders are created only once even if there are multiple ControlMesh instances
	static ID3D11InputLayout*		s_pVertexLayout;
	static ID3D11VertexShader*		s_pVertexShader;
	static ID3D11PixelShader*		s_pPixelShaderGreen;
	static ID3D11PixelShader*		s_pPixelShaderRed;
	static ID3D11PixelShader*		s_pPixelShaderBlue;

	static ID3D11HullShader*		s_pHullShader;
	static ID3D11DomainShader*		s_pDomainShader;

	static ID3D11VertexShader*		s_pVertexShaderLines;
	static ID3D11VertexShader*		s_pVertexShaderLimit;

	static ID3D11RasterizerState* s_pRasterizerStateSolid;
	static ID3D11RasterizerState* s_pRasterizerStateWireframe;

	static ID3D11ComputeShader*		s_pEdgePointCS;
	static ID3D11ComputeShader*		s_pVertexPointCS;

	static ID3D11Buffer* s_pConstantBufferPerLevel;

	static float	m_fTessFactor;
private:
	UINT m_MeshNumSubdivLevels;
	UINT m_VertexBufferSize;

	UINT* m_pVertexOffsets;

	static bool s_bShadersAreCreated;
	LLevel*		m_BaseMesh;
	LLevel**	m_MeshSubdiviedLevels;

	//! Vertex buffer and corresponding views
	ID3D11Buffer*					m_pVertexBuffer;
	ID3D11ShaderResourceView*		m_pVertexBufferSRV;
	ID3D11ShaderResourceView*		m_pVertexBufferSRV4Components;
	ID3D11UnorderedAccessView*		m_pVertexBufferUAV;

	ID3D11Buffer**					m_pValenceBuffer;
	ID3D11ShaderResourceView**			m_pValenceBufferSRV;

	ID3D11Buffer*		m_pConstantBufferRunTables;
};