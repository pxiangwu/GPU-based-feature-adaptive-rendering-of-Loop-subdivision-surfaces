// Written by P.Wu, email: pxiangwu@gmail.com.

#include "DefaultInclude.h"
#include "PartialCase.h"

extern HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, const D3D_SHADER_MACRO* pDefines = NULL, DWORD pCompilerFlags = 0 );

extern void GetOneRing12Indices(Face* f, UINT* indices, UINT ringSize);

static const UINT Map2Patch[12] = {7,3,6,4,8,11,2,0,1,10,9,5};

extern Face* neighbor(Face* l, Face* r, Edge e);

PartialCase::PartialCase(void)
{
	m_NumShaders = 0;
	m_pHSPartials = NULL;
	m_pDSPartials = NULL;
	m_NumIndicesPatches = 0;
	m_IndexBuffersPatches = NULL;
}

PartialCase::~PartialCase(void)
{
}

void PartialCase::Destroy()
{
	SAFE_RELEASE(m_IndexBuffersPatches);
	DestroyShaders();
}

HRESULT PartialCase::CreateIndexBufferPatches( ID3D11Device* pd3dDevice, std::set<Face*> &targetFaces)
{
	HRESULT hr = S_OK;

	UINT patchID = 0;
	int ringSize = 12;

	m_NumIndicesPatches = targetFaces.size()*12;
	UINT* indices = new UINT[m_NumIndicesPatches];
	for (set<Face*>::iterator f = targetFaces.begin(); f != targetFaces.end(); f++)
	{		
		if((*f)->n == 3 && (*f)->IsRegular() && (*f)->GetNumBoundaryVertices() == 0) // ÏÈ½øÐÐÅÐ¶Ï
		{
			if((*f)->m_Father == NULL || !((*f)->m_Father)->m_isPartialFace)
			{
				GetOneRing12Indices(*f, &indices[patchID*12], ringSize);
				patchID++;
			}
		}
	}
	m_NumIndicesPatches = patchID * 12;

	//create index buffers
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory( &bufferDesc, sizeof(D3D11_BUFFER_DESC) );
	bufferDesc.Usage           = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth       = m_NumIndicesPatches * sizeof(unsigned int);
	bufferDesc.BindFlags       = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags  = 0;
	bufferDesc.MiscFlags       = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );

	InitData.pSysMem = indices;
	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_IndexBuffersPatches ));
	} else {
		m_IndexBuffersPatches = NULL;
	}

	SAFE_DELETE_ARRAY(indices);
	return hr;
}

void PartialCase::DrawIndexedPatches(ID3D11DeviceContext* pd3dImmediateContext, UINT offset)
{
	if(m_NumIndicesPatches == 0) return;

	pd3dImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST );
	pd3dImmediateContext->IASetIndexBuffer(m_IndexBuffersPatches, DXGI_FORMAT_R32_UINT, 0);

	for(UINT i = 0; i<m_NumShaders; i++){
		pd3dImmediateContext->HSSetShader(m_pHSPartials[i], 0, 0);
		pd3dImmediateContext->DSSetShader(m_pDSPartials[i], 0, 0);
		pd3dImmediateContext->DrawIndexed(m_NumIndicesPatches, 0, offset);
	}
}

HRESULT PartialCase::CreateShadersGeneral(ID3D11Device* pd3dDevice, std::vector<D3D_SHADER_MACRO*>& macros)
{
	HRESULT hr = S_OK;

	m_pHSPartials = new ID3D11HullShader*[m_NumShaders];
	m_pDSPartials = new ID3D11DomainShader*[m_NumShaders];

	ID3DBlob* pBlobHS = NULL;
	ID3DBlob* pBlobDS = NULL;

	for(UINT i=0; i<m_NumShaders; i++)
	{
		V_RETURN( CompileShaderFromFile( L"GPULoopSubdiv.hlsl", "boxSplineHS", "hs_5_0", &pBlobHS, macros[i], D3DCOMPILE_IEEE_STRICTNESS) );
		V_RETURN( pd3dDevice->CreateHullShader( pBlobHS->GetBufferPointer(), pBlobHS->GetBufferSize(), NULL, &m_pHSPartials[i]) );

		V_RETURN( CompileShaderFromFile( L"GPULoopSubdiv.hlsl", "boxSplineDS", "ds_5_0", &pBlobDS, macros[i] ) );
		V_RETURN( pd3dDevice->CreateDomainShader( pBlobDS->GetBufferPointer(), pBlobDS->GetBufferSize(), NULL, &m_pDSPartials[i]) );
	}

	SAFE_RELEASE(pBlobHS);
	SAFE_RELEASE(pBlobDS);

	return hr;
}

//static definitions
bool PartialCase0::s_bShadersAreCreated = false;
ID3D11HullShader**		PartialCase0::stg_pHSPartials = NULL;
ID3D11DomainShader**		PartialCase0::stg_pDSPartials = NULL;

PartialCase0::PartialCase0(void)
{
}

PartialCase0::~PartialCase0(void)
{
}

HRESULT PartialCase0::CreateShaders(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	m_NumShaders = 2;
	if (!s_bShadersAreCreated) {
		std::vector<D3D_SHADER_MACRO*> macros;
		D3D_SHADER_MACRO partial0_macros[] = {{ "CASE00", "1" },{0}};	macros.push_back(partial0_macros);
		D3D_SHADER_MACRO partial1_macros[] = {{ "CASE01", "1" },{0}};	macros.push_back(partial1_macros);

		V_RETURN(CreateShadersGeneral(pd3dDevice, macros));
		s_bShadersAreCreated = true;

		stg_pHSPartials = m_pHSPartials;
		stg_pDSPartials = m_pDSPartials;
	}else {
		m_pHSPartials = stg_pHSPartials;
		m_pDSPartials = stg_pDSPartials;
	}

	return hr;
}

void PartialCase0::DestroyShaders()
{
	if (!s_bShadersAreCreated)		return;
	for (UINT i = 0; i < m_NumShaders; i++) {
		if(stg_pHSPartials)		SAFE_RELEASE(stg_pHSPartials[i]);
		if(stg_pDSPartials)		SAFE_RELEASE(stg_pDSPartials[i]);
	}
	SAFE_DELETE_ARRAY(stg_pHSPartials);
	SAFE_DELETE_ARRAY(stg_pDSPartials);

	s_bShadersAreCreated = false;
}


//static definitions
bool PartialCase1::s_bShadersAreCreated = false;
ID3D11HullShader**		PartialCase1::stg_pHSPartials = NULL;
ID3D11DomainShader**		PartialCase1::stg_pDSPartials = NULL;

PartialCase1::PartialCase1(void)
{
}

PartialCase1::~PartialCase1(void)
{
}

HRESULT PartialCase1::CreateShaders(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	m_NumShaders = 3;
	if (!s_bShadersAreCreated) {
		std::vector<D3D_SHADER_MACRO*> macros;
		D3D_SHADER_MACRO partial0_macros[] = {{ "CASE10", "1" },{0}};	macros.push_back(partial0_macros);
		D3D_SHADER_MACRO partial1_macros[] = {{ "CASE11", "1" },{0}};	macros.push_back(partial1_macros);
		D3D_SHADER_MACRO partial2_macros[] = {{ "CASE12", "1" },{0}};	macros.push_back(partial2_macros);

		V_RETURN(CreateShadersGeneral(pd3dDevice, macros));
		s_bShadersAreCreated = true;

		stg_pHSPartials = m_pHSPartials;
		stg_pDSPartials = m_pDSPartials;
	}else {
		m_pHSPartials = stg_pHSPartials;
		m_pDSPartials = stg_pDSPartials;
	}

	return hr;
}

void PartialCase1::DestroyShaders()
{
	if (!s_bShadersAreCreated)		return;
	for (UINT i = 0; i < m_NumShaders; i++) {
		if(stg_pHSPartials)		SAFE_RELEASE(stg_pHSPartials[i]);
		if(stg_pDSPartials)		SAFE_RELEASE(stg_pDSPartials[i]);
	}
	SAFE_DELETE_ARRAY(stg_pHSPartials);
	SAFE_DELETE_ARRAY(stg_pDSPartials);

	s_bShadersAreCreated = false;
}

//static definitions
bool PartialCase2::s_bShadersAreCreated = false;
ID3D11HullShader**		PartialCase2::stg_pHSPartials = NULL;
ID3D11DomainShader**		PartialCase2::stg_pDSPartials = NULL;

PartialCase2::PartialCase2(void)
{
}

PartialCase2::~PartialCase2(void)
{
}

HRESULT PartialCase2::CreateShaders(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	m_NumShaders = 4;
	if (!s_bShadersAreCreated) {
		std::vector<D3D_SHADER_MACRO*> macros;
		D3D_SHADER_MACRO partial0_macros[] = {{ "CASE20", "1" },{0}};	macros.push_back(partial0_macros);
		D3D_SHADER_MACRO partial1_macros[] = {{ "CASE21", "1" },{0}};	macros.push_back(partial1_macros);
		D3D_SHADER_MACRO partial2_macros[] = {{ "CASE22", "1" },{0}};	macros.push_back(partial2_macros);
		D3D_SHADER_MACRO partial3_macros[] = {{ "CASE23", "1" },{0}};	macros.push_back(partial3_macros);

		V_RETURN(CreateShadersGeneral(pd3dDevice, macros));
		s_bShadersAreCreated = true;

		stg_pHSPartials = m_pHSPartials;
		stg_pDSPartials = m_pDSPartials;
	}else {
		m_pHSPartials = stg_pHSPartials;
		m_pDSPartials = stg_pDSPartials;
	}

	return hr;
}

void PartialCase2::DestroyShaders()
{
	if (!s_bShadersAreCreated)		return;
	for (UINT i = 0; i < m_NumShaders; i++) {
		if(stg_pHSPartials)		SAFE_RELEASE(stg_pHSPartials[i]);
		if(stg_pDSPartials)		SAFE_RELEASE(stg_pDSPartials[i]);
	}
	SAFE_DELETE_ARRAY(stg_pHSPartials);
	SAFE_DELETE_ARRAY(stg_pDSPartials);

	s_bShadersAreCreated = false;
}