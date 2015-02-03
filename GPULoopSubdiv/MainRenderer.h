// Written by P.Wu, email: pxiangwu@gmail.com.

#include "DefaultInclude.h"
#include "LLevel.h"

#include <fstream>
#include <cstring>
#include <vector>
#include <sstream>
#include <cmath>

static ControlMesh cmesh;;

HRESULT OnCreateDevice( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext )
{
	HRESULT hr = S_OK;
	ControlMesh::CreateShaders(pd3dDevice);

	ObjectDescriptor obd;
	obd = CreateSceneFromFile(pd3dDevice, L"Media/icosahedron.desc");
	
	cmesh.Create(pd3dDevice, obd);
	return hr;
}

static bool b_CompiledShaderDirectoryCreated = false;
HRESULT CompileShaderFromFile( WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut, const D3D_SHADER_MACRO* pDefines /*= NULL*/, DWORD pCompilerFlags /*= 0*/)
{
	HRESULT hr = S_OK;

	std::wstring compiledShaderDirectory(L"CompiledShaders/");

	if (!b_CompiledShaderDirectoryCreated) {
		CreateDirectory(compiledShaderDirectory.c_str(), NULL);
		b_CompiledShaderDirectoryCreated = true;
	}
	std::wstring compiledFilename(compiledShaderDirectory);
	compiledFilename.append(szFileName);


	compiledFilename.push_back('.');
	std::string entryPoint(szEntryPoint);
	unsigned int oldLen = compiledFilename.length();
	compiledFilename.resize(entryPoint.length() + oldLen);
	std::copy(entryPoint.begin(), entryPoint.end(), compiledFilename.begin()+oldLen);

	if (pDefines) {
		compiledFilename.push_back('.');
		for (unsigned int i = 0; pDefines[i].Name != NULL; i++) {
			std::string name(pDefines[i].Name);
			if (name[0] == '\"')				name[0] = 'x';
			if (name[name.length()-1] == '\"')	name[name.length()-1] = 'x';
			std::string def(pDefines[i].Definition);
			if (def[0] == '\"')				def[0] = 'x';
			if (def[def.length()-1] == '\"')	def[def.length()-1] = 'x';
			oldLen = compiledFilename.length();
			compiledFilename.resize(oldLen + name.length() + def.length());
			std::copy(name.begin(), name.end(), compiledFilename.begin()+oldLen);
			std::copy(def.begin(), def.end(), compiledFilename.begin()+name.length()+oldLen);
		}
	}
	if (pCompilerFlags) {
		compiledFilename.push_back('.');
		std::wstringstream ss;
		ss << pCompilerFlags;
		std::wstring cf;
		ss >> cf;
		compiledFilename.append(cf);
	}
	compiledFilename.push_back('.');	
	compiledFilename.push_back('p');


	HANDLE hFindShader;
	HANDLE hFindCompiled;
	WIN32_FIND_DATA findData_shader;
	WIN32_FIND_DATA findData_compiled;

	//hFindShader = FindFirstFile(szFileName, &findData_shader);
	//hFindCompiled = FindFirstFile(compiledFilename.c_str(), &findData_compiled);
	
#ifndef WIN_METRO_APP
	std::wstring shaderFileNamePath(L"./");
	shaderFileNamePath.append(std::wstring(szFileName));
#else
	std::wstring shaderFileNamePath(szFileName);
#endif
	hFindShader = FindFirstFileEx(shaderFileNamePath.c_str(), FindExInfoStandard, &findData_shader, FindExSearchNameMatch, NULL, 0);
	hFindCompiled = FindFirstFileEx(compiledFilename.c_str(),  FindExInfoStandard, &findData_compiled, FindExSearchNameMatch, NULL, 0);

	bool shaderFilesHasModifications = false;
	if (findData_shader.ftLastWriteTime.dwHighDateTime > findData_compiled.ftLastWriteTime.dwHighDateTime ||
			(findData_shader.ftLastWriteTime.dwHighDateTime == findData_compiled.ftLastWriteTime.dwHighDateTime) &&
				(findData_shader.ftLastWriteTime.dwLowDateTime > findData_compiled.ftLastWriteTime.dwLowDateTime)) {
					shaderFilesHasModifications = true;
	}
	  
#ifndef WIN_METRO_APP
		if (hFindCompiled == INVALID_HANDLE_VALUE || CompareFileTime(&findData_shader.ftLastWriteTime, &findData_compiled.ftLastWriteTime) > 0) {
			//// find the file
			//WCHAR str[MAX_PATH];
			//V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, szFileName ) );
			const WCHAR* str = shaderFileNamePath.c_str();
#else 
//if (hFindCompiled == INVALID_HANDLE_VALUE || shaderFilesHasModifications) {
if (false) {  //CURRENT SHADER COMPILER IS BUGGY - TO BUILD THE SHADERS USE THE OLD STUFF....
			WCHAR *str = szFileName;	// a post-build event copies all that stuff into the right directory!
#endif
			DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
	#if defined( DEBUG ) || defined( _DEBUG )
			// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
			// Setting this flag improves the shader debugging experience, but still allows 
			// the shaders to be optimized and to run exactly the way they will run in 
			// the release configuration of this program.
			dwShaderFlags |= D3DCOMPILE_DEBUG;
	#endif 
			dwShaderFlags |= pCompilerFlags;

			ID3DBlob* pErrorBlob;
#ifndef WIN_METRO_APP
			hr = D3DX11CompileFromFile( str, pDefines, NULL, szEntryPoint, szShaderModel, 
				dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );
#else
			//TODO fix this (shader compile doesn't quite work yet - but on Win8-Apps right now we use the pre-compiled versions anyway
			//hr = D3DCompileFromFile(str, pDefines, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel, dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
#endif
			if( FAILED(hr) ) 
			{
				if( pErrorBlob != NULL )
					OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
				SAFE_RELEASE( pErrorBlob );
				return hr;
			}
			SAFE_RELEASE( pErrorBlob );
   
			std::ofstream compiledFile(compiledFilename.c_str(), ios::out | ios::binary);
			if (!compiledFile.is_open()) {
				throw EXCEPTION("Could not open shader file!");
			}
			 
			compiledFile.write((char*)(*ppBlobOut)->GetBufferPointer(), (*ppBlobOut)->GetBufferSize());
			compiledFile.close();
		} else {
			std::ifstream compiledFile(compiledFilename.c_str(), ios::in | ios::binary);

			if (!compiledFile.is_open()) {
				throw EXCEPTION("Could not open shader file!");
			}
			assert(compiledFile.is_open());
			unsigned int size_data = findData_compiled.nFileSizeLow;

			V_RETURN(D3DCreateBlob(size_data,ppBlobOut));
			compiledFile.read((char*)(*ppBlobOut)->GetBufferPointer(), size_data);
			compiledFile.close();
		}

	return hr;
}
