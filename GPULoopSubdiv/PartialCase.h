// Written by P.Wu, email: pxiangwu@gmail.com.

#pragma once

#include "DXUT.h"
#include "DXUTcamera.h"
#include "SDKmisc.h"
#include "DefaultInclude.h"
#include "mesh.h"

#include <vector>
#include <set>
#include <cassert>
#include <list>

using namespace qemesh;

class PartialCase 
{
public:
	PartialCase(void);
	~PartialCase(void);

	//! Virtual method that crease all shaders; child classes take care for specific patchh setups
	virtual HRESULT CreateShaders(ID3D11Device* pd3dDevice)=0;

	void Destroy();

	HRESULT CreateIndexBufferPatches( ID3D11Device* pd3dDevice, std::set<Face*> &targetFaces);

	void DrawIndexedPatches(ID3D11DeviceContext* pd3dImmediateContext, UINT offset);

protected:
	HRESULT CreateShadersGeneral(ID3D11Device* pd3dDevice, std::vector<D3D_SHADER_MACRO*>& macros);
	virtual void DestroyShaders() = 0;

	UINT					m_NumShaders;

	//! Shaders for partial cases
	ID3D11HullShader**		m_pHSPartials;
	ID3D11DomainShader**	m_pDSPartials;

	//! Index buffers and counts 
	UINT					m_NumIndicesPatches;
	ID3D11Buffer*			m_IndexBuffersPatches;
};

//DIFFERENT IMPLEMENTATION OF TRANSITION PATCHH CASES BELOW

class PartialCase0 :
	public PartialCase
{
public:
	PartialCase0(void);
	~PartialCase0(void);

	virtual HRESULT CreateShaders(ID3D11Device* pd3dDevice);
	virtual void DestroyShaders();
private:
	static bool s_bShadersAreCreated;
	static ID3D11HullShader**		stg_pHSPartials;
	static ID3D11DomainShader**	stg_pDSPartials;
};

class PartialCase1 :
	public PartialCase
{
public:
	PartialCase1(void);
	~PartialCase1(void);

	virtual HRESULT CreateShaders(ID3D11Device* pd3dDevice);
	virtual void DestroyShaders();
private:
	static bool s_bShadersAreCreated;
	static ID3D11HullShader**		stg_pHSPartials;
	static ID3D11DomainShader**	stg_pDSPartials;
};

class PartialCase2 :
	public PartialCase
{
public:
	PartialCase2(void);
	~PartialCase2(void);

	virtual HRESULT CreateShaders(ID3D11Device* pd3dDevice);
	virtual void DestroyShaders();
private:
	static bool s_bShadersAreCreated;
	static ID3D11HullShader**		stg_pHSPartials;
	static ID3D11DomainShader**	stg_pDSPartials;
};