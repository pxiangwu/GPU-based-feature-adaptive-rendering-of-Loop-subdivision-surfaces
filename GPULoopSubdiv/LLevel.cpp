// Written by P.Wu, email: pxiangwu@gmail.com.

#include "LLevel.h"
#include <cstring>
#include <sstream>
#include <fstream>

#define PI 3.1415926

bool ControlMesh::s_bShadersAreCreated = false;

ID3D11InputLayout*		ControlMesh::s_pVertexLayout = NULL;
ID3D11VertexShader*		ControlMesh::s_pVertexShader = NULL;
ID3D11PixelShader*		ControlMesh::s_pPixelShaderGreen = NULL;
ID3D11PixelShader*		ControlMesh::s_pPixelShaderRed = NULL;
ID3D11PixelShader*		ControlMesh::s_pPixelShaderBlue = NULL;
ID3D11HullShader*		ControlMesh::s_pHullShader = NULL;
ID3D11DomainShader*		ControlMesh::s_pDomainShader = NULL;

ID3D11ComputeShader*		ControlMesh::s_pEdgePointCS = NULL;
ID3D11ComputeShader*		ControlMesh::s_pVertexPointCS = NULL;

ID3D11VertexShader*		ControlMesh::s_pVertexShaderLines = NULL;
ID3D11VertexShader*		ControlMesh::s_pVertexShaderLimit = NULL;

static const UINT Map2Patch[12] = {7,3,6,4,8,11,2,0,1,10,9,5};

bool isRegular(Face* f)
{
	int n;
	Edge e = f->rep;
	do{
		n = e.Right()->getN();
		if (n != 6)		return false;
		e = e.ONext();
	}while(e != f->rep);
	return true;
}

void RotateFaceNext(Face* f)
{
	f->rep = f->rep.ONext();
	f->m_Rotations++;
}

void RotateFacePrev( Face* f )
{
	assert(f->m_Rotations > 0);
	f->rep = f->rep.OPrev();
	f->m_Rotations--;
}

Face* neighbor(Face* l, Face* r, Edge e)
{
	if( ((l->rep) != e) || ((l->rep).ONext() != e.ONext()) || ((l->rep).ONext().ONext() != e.ONext().ONext()))
		return l;
	else
		return r;
}

void GetOneRing12Indices(Face* f, UINT* indices, UINT ringSize)
{
	Face* l, *r;
	Face* nb;
	Edge temp, te, representation;
	BaseVertex* previous;
	unsigned int idx;

	unsigned int currPoint = 0;
	Edge e = f->rep;
	for (UINT i = 0; i < 3; i++, e = e.ONext())
	{
		idx = e.Right()->Idx;
		indices[Map2Patch[currPoint++ % ringSize]] = idx;
	}

	for (UINT i = 0; i < 3; i++, e = e.ONext())
	{
		te = e;
		representation = f->rep;
		previous = e.Left();
		for (UINT j = 0; j < 3; j++)
		{
			l = (Face*)te.e->vert[1];  r = (Face*)te.e->vert[3]; 
			nb = neighbor(l, r, representation); //adjacent face
			temp = nb->rep;
			representation = temp;
			for(int h=0; h<3;h++)
			{
				if( temp.e->vert[0] == e.Right() && temp.e->vert[2] != previous )
				{
					idx = (temp.e->vert[2])->Idx;
					indices[Map2Patch[currPoint++ % ringSize]] = idx;
					previous = temp.e->vert[2];
					break;
				}
				else if (temp.e->vert[2] == e.Right() && temp.e->vert[0] != previous )
				{
					idx = (temp.e->vert[0])->Idx;
					indices[Map2Patch[currPoint++ % ringSize]] = idx;
					previous = temp.e->vert[0];
					break;
				}
				else temp = temp.ONext();
			}//for
			te = temp;
		}
	}
}

void RemovesSpacesAndTabs(std::string &str) {
	if (str.length() == 0)	return;
	while(*(--str.end()) == ' ' || *(--str.end()) == '\t')	str.erase(--str.end());
	while(*(str.begin()) == ' ' || *(str.begin()) == '\t')	str.erase(str.begin());
}

void GetAttribute(ObjectDescriptor &desc, std::string &line, std::string *baseFilename = NULL, UINT* numObjects = NULL) {
	if (line.length() == 0)	return;
	if (line[0] == '#')		return;
	

	std::string attr_name = line.substr(0, line.find('='));
	std::string attr_value = line.substr(line.find('\"')+1, line.rfind('\"') - line.find('\"') - 1);
	RemovesSpacesAndTabs(attr_name);
	RemovesSpacesAndTabs(attr_value);

	std::stringstream ss(attr_value);

	if (attr_name == "filename") {
		ss >> desc.m_Filename;
	} else if (attr_name == "isolateExtraordinary") {
		ss >> desc.m_IsolateExtraordinary;
	} else if (attr_name == "subdivisionsInAdvance") {
		ss >> desc.m_SubdivisionsInAdvance;
	} else if (attr_name == "baseVertexTranslation") {
		ss >> desc.m_BaseVertexTranslation.x >> desc.m_BaseVertexTranslation.y >> desc.m_BaseVertexTranslation.z;
	} else if (attr_name == "color") {
		ss >> desc.m_ObjectColor.x >> desc.m_ObjectColor.y >> desc.m_ObjectColor.z >> desc.m_ObjectColor.w;
	} else if (attr_name == "colorTexture") {
		ss >> desc.m_ColorTexture;
	} else if (attr_name == "displacementMap") {
		ss >> desc.m_DisplacementMap;
	} else if (attr_name == "displacementScalar") {
		ss >> desc.m_DisplacementScalar;
	} else if (attr_name == "normalMap") {
		ss >> desc.m_NormalMap;
	} else if (attr_name == "keyframeBaseFilename") {
		ss >> desc.m_KeyFrameBasename;
	} else if (baseFilename && attr_name == "baseFilename") {
		ss >> *baseFilename;
	} else if (numObjects && attr_name == "numObjects") {
		ss >> *numObjects;
	} else if (attr_name == "useDirectCreaseEvaluation") {
		UINT v;
		ss >> v;
		if (v == 0)	desc.m_UseDirectCreaseEvaluation = false;
		else		desc.m_UseDirectCreaseEvaluation = true;
	} else if (attr_name == "useNormalizeVertexCoordinates") {
		UINT v;
		ss >> v;
		if (v == 0)	desc.m_NormalizeVertexCoordinates = false;
		else		desc.m_NormalizeVertexCoordinates = true;
	} else {
		throw std::ifstream::failure(std::string("Parsing error in scene descriptor (unknown attribute): ").append(line));
	}
}

ObjectDescriptor CreateSceneFromFile( ID3D11Device* pd3dDevice, const WCHAR* filename)
{
	std::ifstream file(filename);

	if (!file.is_open()) {
		throw std::ifstream::failure(std::string("Could not open object descriptor file!"));
	}

	ObjectDescriptor desc;

	while(!file.eof()) {
		std::string line;
		
		getline(file, line);
		if (line.length() == 0)	continue;
		RemovesSpacesAndTabs(line);
		if (line[0] == '#')	continue;	//comment

		else if (line == "BeginObject") {
			while (!file.eof()) {
				getline(file, line);
				if (line.length() == 0)	continue;
				RemovesSpacesAndTabs(line);
				if (line[0] == '#')	continue;	//comment

				else if (line == "EndObject") {	//end of object
					if (desc.m_Filename.length() == 0) throw std::ifstream::failure(std::string("\"filename\" MUST be specified in scene descriptor!"));
					break;
				} else {
					GetAttribute(desc, line);
				}
			}
		} else if (line == "BeginSequenceObjects") {
			UINT numObjects = 0;
			std::string baseFilename;

			while (!file.eof()) {
				getline(file, line);
				if (line.length() == 0)	continue;
				RemovesSpacesAndTabs(line);
				if (line[0] == '#')	continue;	//comment

				else if (line == "EndSequenceObjects") {	//end of object
					if (baseFilename.length() == 0 || numObjects == 0) throw std::ifstream::failure(std::string("\"basefilename\" and \"numObject\" MUST be specified in scene descriptor!"));
					for (UINT i = 0; i < numObjects; i++) {
						std::stringstream ss;
						ss << baseFilename;
						ss << i << ".subdiv";
						desc.m_Filename = ss.str();
					}
					break;
				} else {
					GetAttribute(desc, line, &baseFilename, &numObjects);
				}

			}

		}
	}
	file.close();
	return desc;
}

LLevel::LLevel(void)
{
	m_IndexBufferFullPatch = NULL;
	m_NumIndicesFullPatch = 0;

	m_IndexBufferEndPatch = NULL;
	m_NumIndicesEndPatch = 0;

	m_IndexBufferIrregularPatch = NULL;
	m_NumIndicesIrregularPatch = 0;

	m_IndexBufferPartialPatch = NULL;
	m_NumIndicesPartialPatch = 0;

	m_E0_IT = NULL;
	m_V0_ITa = NULL;
	m_V0_IT = NULL;

	m_BUF_E0_IT = NULL;
	m_BUF_V0_ITa = NULL;
	m_BUF_V0_IT = NULL;

	m_SRV_E0_IT = NULL;
	m_SRV_V0_IT = NULL;
	m_SRV_V0_ITa = NULL;
}

LLevel::~LLevel(void)
{}

void LLevel::Destroy()
{
	SAFE_RELEASE(m_IndexBufferFullPatch);
	SAFE_RELEASE(m_IndexBufferEndPatch);
	SAFE_RELEASE(m_IndexBufferIrregularPatch);
	SAFE_RELEASE(m_IndexBufferPartialPatch);

	SAFE_RELEASE(m_BUF_E0_IT);
	SAFE_RELEASE(m_BUF_V0_ITa);
	SAFE_RELEASE(m_BUF_V0_IT);

	SAFE_RELEASE(m_SRV_E0_IT);
	SAFE_RELEASE(m_SRV_V0_IT);
	SAFE_RELEASE(m_SRV_V0_ITa);

	SAFE_DELETE_ARRAY (m_V0_ITa);
	SAFE_DELETE_ARRAY (m_V0_IT);
	SAFE_DELETE_ARRAY (m_E0_IT);

	for (unsigned int i = 0; i < this->elist.size(); i++) {
		SAFE_DELETE(elist[i]);
	}
	for (unsigned int i = 0; i < this->flist.size(); i++) {
		SAFE_DELETE(flist[i]);
	}
	for (unsigned int i = 0; i < this->vlist.size(); i++) {
		SAFE_DELETE(vlist[i]);
	}
	m_SubdivFaces.clear();
	m_SubdivEdges.clear();
	m_SubdivVertices.clear();

	m_FullPatch.clear();
	m_EndPatch.clear();
	//m_irregularPatch.clear();
	m_PartialPatch.clear();

	m_PartialCase0.Destroy();
	m_PartialCase1.Destroy();
	m_PartialCase2.Destroy();
}

#define BUF_SIZE 256
static void skipLine(char * buf, int size, FILE * fp)//not very robust method! Some bug exists.
{
	do {
		buf[size-1] = '$';
		fgets(buf, size, fp);
	} while (buf[size-1] != '$');
}

LLevel* CreateMeshFromFile( const std::string &filename)
{
	if (filename.length() > 3 && filename.find(".obj", filename.length()-4) != string::npos) {
		return LLevel::LoadObjFromFile(filename);
	}else {
		throw std::ifstream::failure(std::string("Unknown file extension of file: ").append(filename));
	}
}

LLevel* LLevel::LoadObjFromFile( const std::string &filename )
{
	FILE* fp = fopen(filename.c_str(), "r");
	if (fp == NULL)	throw std::ifstream::failure(std::string("Could not find file: ").append(filename));

	LLevel* mesh = new LLevel();

	char buf[BUF_SIZE];
	float val[4];
	int idx[256][3];
	int match;
	bool vtx4Comp = false;	//	is a vertex made up of four components?
	bool tex3Comp = false;
	bool hasTC = false;
	bool hasNormals = false;

	mesh->m_bIsTriMesh = true;

	Vertex<float4>* vertex[1024];
	std::vector<Vertex<float4>*> vertices;

	std::vector<float> _positions;
	std::vector<float> _normals;
	std::vector<float> _texCoords;
	UINT type = 0;
	while ( fscanf( fp, "%s", buf) != EOF ) {

		switch (buf[0]) {
			case '#':
				//comment line, eat the remainder
				skipLine( buf, BUF_SIZE, fp);
				break;

			case 'v':
				switch (buf[1]) {

			case '\0':
				//vertex, 3 or 4 components
				val[3] = 1.0f;  //default w coordinate
				match = fscanf( fp, "%f %f %f %f", &val[0], &val[1], &val[2], &val[3]);

				vertices.push_back( mesh->addVertex(float4(val[0], val[1], val[2], val[3])));

				_positions.push_back( val[0]);
				_positions.push_back( val[1]);
				_positions.push_back( val[2]);
				_positions.push_back( val[3]);
				vtx4Comp |= ( match == 4);
				assert( match > 2 && match < 5);
				break;

			case 'n':
				//normal, 3 components
				match = fscanf( fp, "%f %f %f", &val[0], &val[1], &val[2]);
				_normals.push_back( val[0]);
				_normals.push_back( val[1]);
				_normals.push_back( val[2]);
				assert( match == 3);
				break;

			case 't':
				//texcoord, 2 or 3 components
				val[2] = 0.0f;  //default r coordinate
				match = fscanf( fp, "%f %f %f %f", &val[0], &val[1], &val[2]);
				_texCoords.push_back( val[0]);
				_texCoords.push_back( val[1]);
				_texCoords.push_back( val[2]);
				tex3Comp |= ( match == 3);
				assert( match > 1 && match < 4);
				break;
				}
				break;

			case 'f':
				//face
				fscanf( fp, "%s", buf);
				{
					unsigned int n = 2;

					//determine the type, and read the initial vertex, all entries in a face must have the same format
					if ( sscanf( buf, "%d//%d", &idx[0][0], &idx[0][1]) == 2) {
						type = 4;
						//This face has vertex and normal indices

						//remap them to the right spot
						idx[0][0] = (idx[0][0] > 0) ? (idx[0][0] - 1) : ((int)_positions.size() - idx[0][0]);
						idx[0][1] = (idx[0][1] > 0) ? (idx[0][1] - 1) : ((int)_normals.size() - idx[0][1]);

						//grab the second vertex to prime
						fscanf( fp, "%d//%d", &idx[1][0], &idx[1][1]);

						//remap them to the right spot
						idx[1][0] = (idx[1][0] > 0) ? (idx[1][0] - 1) : ((int)_positions.size() - idx[1][0]);
						idx[1][1] = (idx[1][1] > 0) ? (idx[1][1] - 1) : ((int)_normals.size() - idx[1][1]);

						//create the fan
						while ( fscanf( fp, "%d//%d", &idx[n][0], &idx[n][1]) == 2) {
							//remap them to the right spot
							idx[n][0] = (idx[n][0] > 0) ? (idx[n][0] - 1) : ((int)_positions.size() - idx[n][0]);
							idx[n][1] = (idx[n][1] > 0) ? (idx[n][1] - 1) : ((int)_normals.size() - idx[n][1]);
							n++;
						}
						hasNormals = true;
					}
					else if ( sscanf( buf, "%d/%d/%d", &idx[0][0], &idx[0][1], &idx[0][2]) == 3) {
						type = 3;
						//This face has vertex, texture coordinate, and normal indices

						//remap them to the right spot
						idx[0][0] = (idx[0][0] > 0) ? (idx[0][0] - 1) : ((int)_positions.size() - idx[0][0]);
						idx[0][1] = (idx[0][1] > 0) ? (idx[0][1] - 1) : ((int)_texCoords.size() - idx[0][1]);
						idx[0][2] = (idx[0][2] > 0) ? (idx[0][2] - 1) : ((int)_normals.size() - idx[0][2]);

						//grab the second vertex to prime
						fscanf( fp, "%d/%d/%d", &idx[1][0], &idx[1][1], &idx[1][2]);

						//remap them to the right spot
						idx[1][0] = (idx[1][0] > 0) ? (idx[1][0] - 1) : ((int)_positions.size() - idx[1][0]);
						idx[1][1] = (idx[1][1] > 0) ? (idx[1][1] - 1) : ((int)_texCoords.size() - idx[1][1]);
						idx[1][2] = (idx[1][2] > 0) ? (idx[1][2] - 1) : ((int)_normals.size() - idx[1][2]);

						//create the fan
						while ( fscanf( fp, "%d/%d/%d", &idx[n][0], &idx[n][1], &idx[n][2]) == 3) {
							//remap them to the right spot
							idx[n][0] = (idx[n][0] > 0) ? (idx[n][0] - 1) : ((int)_positions.size() - idx[n][0]);
							idx[n][1] = (idx[n][1] > 0) ? (idx[n][1] - 1) : ((int)_texCoords.size() - idx[n][1]);
							idx[n][2] = (idx[n][2] > 0) ? (idx[n][2] - 1) : ((int)_normals.size() - idx[n][2]);
							n++;
						}

						hasTC = true;
						hasNormals = true;
					}
					else if ( sscanf( buf, "%d/%d", &idx[0][0], &idx[0][1]) == 2) {
						type = 2;
						//This face has vertex and texture coordinate indices

						//remap them to the right spot
						idx[0][0] = (idx[0][0] > 0) ? (idx[0][0] - 1) : ((int)_positions.size() - idx[0][0]);
						idx[0][1] = (idx[0][1] > 0) ? (idx[0][1] - 1) : ((int)_texCoords.size() - idx[0][1]);

						//grab the second vertex to prime
						fscanf( fp, "%d/%d", &idx[1][0], &idx[1][1]);

						//remap them to the right spot
						idx[1][0] = (idx[1][0] > 0) ? (idx[1][0] - 1) : ((int)_positions.size() - idx[1][0]);
						idx[1][1] = (idx[1][1] > 0) ? (idx[1][1] - 1) : ((int)_texCoords.size() - idx[1][1]);

						//create the fan
						while ( fscanf( fp, "%d/%d", &idx[n][0], &idx[n][1]) == 2) {
							//remap them to the right spot
							idx[n][0] = (idx[n][0] > 0) ? (idx[n][0] - 1) : ((int)_positions.size() - idx[n][0]);
							idx[n][1] = (idx[n][1] > 0) ? (idx[n][1] - 1) : ((int)_texCoords.size() - idx[n][1]);
							n++;
						}
						hasTC = true;
					}
					else if ( sscanf( buf, "%d", &idx[0][0]) == 1) {
						type = 1;
						//This face has only vertex indices

						//remap them to the right spot
						idx[0][0] = (idx[0][0] > 0) ? (idx[0][0] - 1) : ((int)_positions.size() - idx[0][0]);

						//grab the second vertex to prime
						fscanf( fp, "%d", &idx[1][0]);

						//remap them to the right spot
						idx[1][0] = (idx[1][0] > 0) ? (idx[1][0] - 1) : ((int)_positions.size() - idx[1][0]);

						//create the fan
						while ( fscanf( fp, "%d", &idx[n][0]) == 1) {
							//remap them to the right spot
							idx[n][0] = (idx[n][0] > 0) ? (idx[n][0] - 1) : ((int)_positions.size() - idx[n][0]);
							n++;
						}
					}
					else {
						//bad format
						assert(0);
						skipLine( buf, BUF_SIZE, fp);
					}

					if (n > 2){
						for(UINT i = 0; i < n; i++){
							vertex[i] = vertices[idx[i][0]];
							//	if (type == 2) {	//has vertex and tex coords
							//		vertex[i]->texCoord = float2(_texCoords[idx[i][1]*3+0], _texCoords[idx[i][1]*3+1]);
							//	} else if (type == 3) { // has vertex, normals and tex coords
							//		vertex[i]->texCoord = float2(_texCoords[idx[i][2]*3+0], _texCoords[idx[i][2]*3+1]);
							//	}
						}
						try {
							Face* f = mesh->addFace(vertex, n);

							for (UINT i = 0; i < n; i++) {
								float2 tex(0.0f,0.0f);
								if (type == 2) {
									tex = float2(_texCoords[idx[i][1]*3+0], _texCoords[idx[i][1]*3+1]);
								} else if (type == 3) {
									tex = float2(_texCoords[idx[i][1]*3+0], _texCoords[idx[i][1]*3+1]);	//monsterfrog animation needs it that way
								}
								f->m_TexCoords.push_back(tex);
							}
							if (n != 4) mesh->m_bIsTriMesh = false;
						} 
						catch (MeshError) {
							OutputDebugString(L"\nWARNING: Mesh is non-manifold -> ignoring Face");
						}
					}
					else{
						if (n < -3) mesh->m_bIsTriMesh = false;// n<-3 !
					}
				}
				break;

			case 's':
			case 'g':
			case 'u':
				//all presently ignored
			default:
				skipLine( buf, BUF_SIZE, fp);

		};
	}

	fclose(fp);

	return mesh;
}

HRESULT LLevel::CreateIndexBufferLines(ID3D11Device* pd3dDevice)
{	
	HRESULT hr = S_OK;

	SAFE_RELEASE(m_IndexBufferFullPatch);

	m_NumIndicesFullPatch = 2 * elist.size();
	unsigned int* indices = new unsigned int[m_NumIndicesFullPatch];

	unsigned int cnt = 0;
	for (vector<TriEdge*>::iterator qe = elist.begin(); qe != elist.end(); ++qe)
	{
		indices[cnt++] = (*qe)->vert[0]->Idx;// two end points of one edge
		indices[cnt++] = (*qe)->vert[2]->Idx;
	}
	assert(m_NumIndicesFullPatch == cnt);

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory( &bufferDesc, sizeof(D3D11_BUFFER_DESC) );
	bufferDesc.Usage           = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth       = m_NumIndicesFullPatch * sizeof(unsigned int);
	bufferDesc.BindFlags       = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags  = 0;
	bufferDesc.MiscFlags       = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );

	InitData.pSysMem = indices;
	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_IndexBufferFullPatch ));
	} else {
		m_IndexBufferFullPatch = NULL;
	}

	SAFE_DELETE_ARRAY(indices);

	return hr;
}

HRESULT LLevel::CreateIndexBufferRegularPatch(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	SAFE_RELEASE(m_IndexBufferFullPatch);
	SAFE_RELEASE(m_IndexBufferEndPatch);
	SAFE_RELEASE(m_IndexBufferPartialPatch);

	UINT patchID = 0;
	int ringSize = 12;

	// Create m_IndexBufferFullPatch ============================================================
	m_NumIndicesFullPatch = m_FullPatch.size() * 12;

	unsigned int* indices = new unsigned int[m_NumIndicesFullPatch]; ///////////////////////////

	for (set<Face*>::iterator f = m_FullPatch.begin(); f != m_FullPatch.end(); ++f)
	{
		if((*f)->n && (*f)->IsRegular() && (*f)->GetNumBoundaryVertices()==0){
			if((*f)->m_Father == NULL || !((*f)->m_Father)->m_isPartialFace)
			{
				GetOneRing12Indices(*f, &indices[patchID*12], ringSize);
				patchID++;
			}
		}
	}//for
	//assert(patchID == m_FullPatch.size());
	m_NumIndicesFullPatch = patchID*12;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory( &bufferDesc, sizeof(D3D11_BUFFER_DESC) );
	bufferDesc.Usage           = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth       = m_NumIndicesFullPatch * sizeof(unsigned int);
	bufferDesc.BindFlags       = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags  = 0;
	bufferDesc.MiscFlags       = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );

	InitData.pSysMem = indices;
	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_IndexBufferFullPatch ));
	} else {
		m_IndexBufferFullPatch = NULL;
	}
	SAFE_DELETE_ARRAY(indices);
	// ================================================================================


	// Create m_IndexBufferEndPatch ===========================================================
	patchID = 0;
	m_NumIndicesEndPatch = m_EndPatch.size() * 12;
	unsigned int* indicesEndPatch = new unsigned int[m_NumIndicesEndPatch];
	for (set<Face*>::iterator f = m_EndPatch.begin(); f != m_EndPatch.end(); ++f)
	{
		if((*f)->n && (*f)->IsRegular() && (*f)->GetNumBoundaryVertices()==0){
			if((*f)->m_Father == NULL || !((*f)->m_Father)->m_isPartialFace)
			{
				GetOneRing12Indices(*f, &indicesEndPatch[patchID*12], ringSize);
				patchID++;
			}
		}
	}//for
	m_NumIndicesEndPatch = patchID*12;

	bufferDesc.ByteWidth       = m_NumIndicesEndPatch * sizeof(unsigned int);
	InitData.pSysMem = indicesEndPatch;

	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_IndexBufferEndPatch ));
	} else {
		m_IndexBufferEndPatch = NULL;
	}
	SAFE_DELETE_ARRAY(indicesEndPatch);
	// ================================================================================
	
	// Just for test in terms of PartialPatches ========================================================
	patchID = 0;
	m_NumIndicesPartialPatch = m_PartialPatch.size() * 12;
	unsigned int* indicesPartialPatch = new unsigned int[m_NumIndicesPartialPatch];
	for (set<Face*>::iterator f = m_PartialPatch.begin(); f != m_PartialPatch.end(); ++f)
	{
		if((*f)->n && (*f)->IsRegular() && (*f)->GetNumBoundaryVertices()==0){
			if((*f)->m_Father == NULL || !((*f)->m_Father)->m_isPartialFace)
			{
				GetOneRing12Indices(*f, &indicesPartialPatch[patchID*12], ringSize);
				patchID++;
			}
		}
	}//for
	m_NumIndicesPartialPatch = patchID*12;

	bufferDesc.ByteWidth       = m_NumIndicesPartialPatch * sizeof(unsigned int);
	InitData.pSysMem = indicesPartialPatch;

	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_IndexBufferPartialPatch ));
	} else {
		m_IndexBufferPartialPatch = NULL;
	}
	SAFE_DELETE_ARRAY(indicesPartialPatch);
	// =================================================================================
	return hr;
}

HRESULT LLevel::CreateIndexBufferIrregularPatch(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	SAFE_RELEASE(m_IndexBufferIrregularPatch);

	m_NumIndicesIrregularPatch = m_EndPatch.size() * 3;
	unsigned int* indices = new unsigned int[m_NumIndicesIrregularPatch]; 
	int k = 0;
	Edge temp;
	for (set<Face*>::iterator f = m_EndPatch.begin(); f != m_EndPatch.end(); ++f)
	{
		if ((*f)->getN() == 3 && !(*f)->IsRegular() && (*f)->GetNumBoundaryVertices() == 0){
			temp = (*f)->rep;
			do{
				indices[k++] = temp.Right()->Idx;
				temp = temp.ONext();
			}while(temp != (*f)->rep);
		}
	}//for
	//assert(k == m_NumIndicesIrregularPatch);
	m_NumIndicesIrregularPatch = k;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory( &bufferDesc, sizeof(D3D11_BUFFER_DESC) );
	bufferDesc.Usage           = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth       = m_NumIndicesIrregularPatch * sizeof(unsigned int);
	bufferDesc.BindFlags       = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags  = 0;
	bufferDesc.MiscFlags       = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );

	InitData.pSysMem = indices;
	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_IndexBufferIrregularPatch ));
	} else {
		m_IndexBufferFullPatch = NULL;
	}

	SAFE_DELETE_ARRAY(indices);
	return hr;
}

void LLevel::DrawIndexedFullPatches(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level)
{
	if(!m_IndexBufferFullPatch) CreateIndexBufferRegularPatch(DXUTGetD3D11Device());
	UINT voffset = 0;
	UINT stride = sizeof(float4);

	pd3dImmediatecontext->VSSetShader(ControlMesh::s_pVertexShader, 0 ,0);
	pd3dImmediatecontext->GSSetShader(NULL, 0, 0);
	pd3dImmediatecontext->PSSetShader(ControlMesh::s_pPixelShaderGreen, 0, 0);
	pd3dImmediatecontext->HSSetShader(ControlMesh::s_pHullShader, 0, 0);
	pd3dImmediatecontext->DSSetShader(ControlMesh::s_pDomainShader, 0, 0);
	pd3dImmediatecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST);
	pd3dImmediatecontext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &voffset);
	pd3dImmediatecontext->IASetIndexBuffer(m_IndexBufferFullPatch, DXGI_FORMAT_R32_UINT, 0);
	pd3dImmediatecontext->DrawIndexed(m_NumIndicesFullPatch, 0, offset);
}

void LLevel::DrawIndexedEndPatches(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level)
{
	if(!m_IndexBufferEndPatch) CreateIndexBufferRegularPatch(DXUTGetD3D11Device());
	UINT voffset = 0;
	UINT stride = sizeof(float4);

	pd3dImmediatecontext->VSSetShader(ControlMesh::s_pVertexShader, 0 ,0);
	pd3dImmediatecontext->GSSetShader(NULL, 0, 0);
	pd3dImmediatecontext->PSSetShader(ControlMesh::s_pPixelShaderGreen, 0, 0);
	pd3dImmediatecontext->HSSetShader(ControlMesh::s_pHullShader, 0, 0);
	pd3dImmediatecontext->DSSetShader(ControlMesh::s_pDomainShader, 0, 0);
	pd3dImmediatecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST);
	pd3dImmediatecontext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &voffset);
	pd3dImmediatecontext->IASetIndexBuffer(m_IndexBufferEndPatch, DXGI_FORMAT_R32_UINT, 0);
	pd3dImmediatecontext->DrawIndexed(m_NumIndicesEndPatch, 0, offset);
}

void LLevel::DrawIndexedPartialPatches(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level)
{
	if(!m_IndexBufferPartialPatch) CreateIndexBufferRegularPatch(DXUTGetD3D11Device());
	UINT voffset = 0;
	UINT stride = sizeof(float4);

	pd3dImmediatecontext->VSSetShader(ControlMesh::s_pVertexShader, 0 ,0);
	pd3dImmediatecontext->GSSetShader(NULL, 0, 0);
	pd3dImmediatecontext->PSSetShader(ControlMesh::s_pPixelShaderGreen, 0, 0);
	pd3dImmediatecontext->HSSetShader(ControlMesh::s_pHullShader, 0, 0);
	pd3dImmediatecontext->DSSetShader(ControlMesh::s_pDomainShader, 0, 0);
	pd3dImmediatecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST);
	pd3dImmediatecontext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &voffset);
	pd3dImmediatecontext->IASetIndexBuffer(m_IndexBufferPartialPatch, DXGI_FORMAT_R32_UINT, 0);
	pd3dImmediatecontext->DrawIndexed(m_NumIndicesPartialPatch, 0, offset);
}

void LLevel::DrawIndexedEndIrregularPatches(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level)
{
	if(!m_IndexBufferIrregularPatch) CreateIndexBufferIrregularPatch(DXUTGetD3D11Device());
	UINT voffset = 0;
	UINT stride = sizeof(float4);

	pd3dImmediatecontext->VSSetShader(ControlMesh::s_pVertexShaderLimit, 0, 0);
	pd3dImmediatecontext->HSSetShader(NULL, 0, 0);
	pd3dImmediatecontext->DSSetShader(NULL, 0, 0);
	pd3dImmediatecontext->GSSetShader(NULL, 0, 0);
	pd3dImmediatecontext->PSSetShader(ControlMesh::s_pPixelShaderGreen, 0, 0);
	pd3dImmediatecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dImmediatecontext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &voffset);
	pd3dImmediatecontext->IASetIndexBuffer(m_IndexBufferIrregularPatch, DXGI_FORMAT_R32_UINT, 0);
	pd3dImmediatecontext->DrawIndexed(m_NumIndicesIrregularPatch, 0, offset);
}

void LLevel::DrawIndexedPartialPatchesWatertight(ID3D11DeviceContext* pd3dImmediatecontext, ID3D11Buffer* vertexBuffer, UINT offset, UINT level)
{
	UINT voffset = 0;
	UINT stride = sizeof(float4);

	pd3dImmediatecontext->VSSetShader(ControlMesh::s_pVertexShader, 0 ,0);
	pd3dImmediatecontext->GSSetShader(NULL, 0, 0);
	pd3dImmediatecontext->PSSetShader(ControlMesh::s_pPixelShaderRed, 0, 0);
	pd3dImmediatecontext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &voffset);

	m_PartialCase0.DrawIndexedPatches(pd3dImmediatecontext, offset);
	m_PartialCase1.DrawIndexedPatches(pd3dImmediatecontext, offset);
	m_PartialCase2.DrawIndexedPatches(pd3dImmediatecontext, offset);
}


HRESULT LLevel::CreatePartialCases(ID3D11Device* pd3dDevice, std::set<Face*> &partialPatch)
{
	HRESULT hr = S_OK;

	std::set<Face*> facesCase0;
	std::set<Face*> facesCase1;
	std::set<Face*> facesCase2;

	OrderPartialPatchesToCases(partialPatch, facesCase0, facesCase1, facesCase2);

	V_RETURN(m_PartialCase0.CreateIndexBufferPatches(pd3dDevice, facesCase0));
	V_RETURN(m_PartialCase1.CreateIndexBufferPatches(pd3dDevice, facesCase1));
	V_RETURN(m_PartialCase2.CreateIndexBufferPatches(pd3dDevice, facesCase2));

	return hr;
}

void LLevel::OrderPartialPatchesToCases(std::set<Face*> &partialPatch, std::set<Face*> &facesCase0, std::set<Face*> &facesCase1, std::set<Face*> &facesCase2)
{
	for(set<Face*>::iterator f = partialPatch.begin(); f != partialPatch.end(); ++f)
	{
		Edge e = (*f)->rep;
		assert((*f)->n == 3);

		unsigned int numTriangleHead = 0;
		do {
			if(e.e->isTriangleHead) numTriangleHead++;
			e = e.ONext();
		} while (e != (*f)->rep);

		//Case0
		if (numTriangleHead == 1){
			unsigned int numRotations = 0;
			while(!e.e->isTriangleHead){
				assert(numRotations <= 3);
				numRotations++;
				RotateFaceNext(*f);
				e = e.ONext();
			}
			facesCase0.insert(*f);
		}
		else if(numTriangleHead == 2){
			unsigned int numRotations = 0;
			while ((*f)->m_Rotations) {
				RotateFacePrev(*f);
				e = e.OPrev();
			}
			while(!e.OPrev().e->isTriangleHead || !e.e->isTriangleHead)	{
				assert(numRotations <= 3);
				numRotations++;
				RotateFaceNext(*f);
				e = e.ONext();
			}
			facesCase1.insert(*f);
		}
		else if (numTriangleHead == 3){
			facesCase2.insert(*f);
		}
	}
}

void LLevel::CalPatches()
{
	for (vector<Face*>::iterator f = flist.begin(); f != flist.end(); f++) {
		if ((*f)->m_bIsMarkedForSubdivision) {
			Edge e = (*f)->rep;

			do {
				e.e->isTriangleHead = true;
				e = e.ONext();
			} while (e != (*f)->rep);
		}
	}

	for (vector<Face*>::iterator f = flist.begin(); f != flist.end(); ++f){
		if (abs((*f)->getN()) != 3)	continue; // revision 5.23

		Edge e = (*f)->rep;
		bool isTagged = false;
		bool wasTagged = false;
		UINT numTriangleHead = 0;

		do {		
			if (((Vertex<float4>*)e.e->vert[0])->isTagged) {
				isTagged = true;
			}
			if (((Vertex<float4>*)e.e->vert[2])->isTagged) {
				isTagged = true;
			}
			if (((Vertex<float4>*)e.e->vert[0])->wasPreviousTagged ||
				((Vertex<float4>*)e.e->vert[2])->wasPreviousTagged) {
					wasTagged = true;
			}
			if (e.e->isTriangleHead) {
				numTriangleHead++;
			}
			e = e.ONext();
		} while (e != (*f)->rep);

		if (wasTagged)	m_EndPatch.insert(*f);
		if ((*f)->m_bIsMarkedForSubdivision) continue;

		if (!isTagged && wasTagged) {
			if (numTriangleHead == 0) {
				m_FullPatch.insert(*f);
			} else {
				assert((*f)->IsRegular());
				(*f)->m_isPartialFace = true;
				m_PartialPatch.insert(*f);
			}
		}
	}
}

void LLevel::SubdivideFace( Face *f )
{
	if (f == NULL || f->m_bIsMarkedForSubdivision)	return;

	m_SubdivFaces.insert(f);
	f->m_bIsMarkedForSubdivision = true;

	//for that face get all edges and adjacent faces
	Edge r = f->rep; // internal format: r=3
	do {
		m_SubdivVertices.insert((Vertex<float4>*)r.Right()); // two end points of one edge
		m_SubdivVertices.insert((Vertex<float4>*)r.Left());
		m_SubdivEdges.insert(r.e);

		Edge x = r.Right()->rep; // r=0
		do {
			Face *r = (Face*)x.Right();
			Face *l = (Face*)x.Left();

			if (r)	{
				if (!r->m_bIsMarkedForSubdivision && !r->IsRegular()) {// irregular faces
					SubdivideFace(r);	
				}
				else m_SubdivFaces.insert(r);
			}
			if (l) { 
				if (!l->m_bIsMarkedForSubdivision && !l->IsRegular())	{
					SubdivideFace(l);	
				}
				else m_SubdivFaces.insert(l);
			}

			if (x.e)	m_SubdivEdges.insert(x.e);
			x = x.ONext();
		} while (x != r.Right()->rep);

		Face *o = (Face*)r.Org();
		Face *d = (Face*)r.Dest();
		if (o && !o->IsRegular() && !o->m_bIsMarkedForSubdivision)	SubdivideFace(o);
		if (d && !d->IsRegular() && !d->m_bIsMarkedForSubdivision)	SubdivideFace(d);
		r = r.ONext();
	} while (r != f->rep);
}

LLevel* LLevel::AdaptiveLoop()
{
	LLevel* rvalue = new LLevel();
	
	rvalue->m_bIsTriMesh = true;

	//first check all the faces that need to be subdivided
	for (UINT i = 0; i < flist.size(); i++) {
		if (flist[i]->getN() == 3 && flist[i]->GetNumBoundaryVertices() == 3) {	// renderer cannot deal with boundary 3 faces, so subdivide them
			SubdivideFace(flist[i]);
		}
	}

	for (vector<Vertex<float4>*>::iterator v = vlist.begin(); v != vlist.end(); v++) {
		if ((*v)->vertexSubdivisions > 0) {
			(*v)->isTagged = true;
		}
	}

	for (vector<Vertex<float4>*>::iterator v = vlist.begin(); v != vlist.end(); v++) {
		
		if ((*v)->getN() != 0 && (*v)->isTagged) {	//subdivide all faces around a tagged vertx			
			Edge e = (*v)->rep;

			do {	//get the adjacent faces
				Face *f1 = (Face*)e.e->vert[1];
				Face *f2 = (Face*)e.e->vert[3];

				SubdivideFace(f1);
				SubdivideFace(f2);
				 
				e = e.ONext();								
			} while (e != (*v)->rep);
		}
	}

	for (UINT i = 0; i < vlist.size(); i++) {
		vlist[i]->newIdx = (UINT)-1;
	}
	for (UINT i = 0; i < elist.size(); i++) {
		elist[i]->newIdx = (UINT)-1;
	}
	for (UINT i = 0; i < flist.size(); i++) {
		flist[i]->newIdx = (UINT)-1;
	}

	Vertex<float4>** newVertices = new Vertex<float4>*[m_SubdivVertices.size() + m_SubdivEdges.size()]; // m_SubdivFaces.size() is not necessary

	UINT i = 0;
	for (set<Vertex<float4>*>::iterator v = m_SubdivVertices.begin(); v != m_SubdivVertices.end(); v++, i++){
		(*v)->newIdx = i;
		newVertices[i] = rvalue->addVertex();
	}

	i = 0;
	for (set<TriEdge*>::iterator qe = m_SubdivEdges.begin(); qe != m_SubdivEdges.end(); qe++, i++){
		(*qe)->newIdx = i;
		newVertices[i + m_SubdivVertices.size()] = rvalue->addVertex();
	}

	for (UINT i = 0; i < m_SubdivVertices.size() + m_SubdivEdges.size(); i++) {
		newVertices[i]->p.x = newVertices[i]->p.y = newVertices[i]->p.z = newVertices[i]->p.w = 0.0f;
	}
	//compute new edge points -- Loop
	i = 0;
	for (set<TriEdge*>::iterator qe = m_SubdivEdges.begin(); qe != m_SubdivEdges.end(); qe++, i++)
	{
		float4* p = &(newVertices[i + m_SubdivVertices.size()]->p);
		p->x = p->y = p->z = p->w = 0.0f;

		float4 p1, p2, p3, p4;
		p1 = ((Vertex<float4>*)(*qe)->vert[0])->p;
		p2 = ((Vertex<float4>*)(*qe)->vert[2])->p;
		Face* l = (Face*)((*qe)->vert[1]);
		Face* r = (Face*)((*qe)->vert[3]);
		//Òí±ß±éÀú
		//left Winged edge
		Edge temp = l->rep;
		do{
			if(temp.e->vert[0] != (*qe) ->vert[0] && temp.e->vert[0] != (*qe)->vert[2])
			{
				p3 = ((Vertex<float4>*)(temp.e->vert[0]))->p;
			}
			else if(temp.e->vert[2] != (*qe) ->vert[0] && temp.e->vert[2] != (*qe)->vert[2])
			{
				p3 = ((Vertex<float4>*)(temp.e->vert[2]))->p;
			}
			temp = temp.ONext();
		}while(temp!= l->rep);

		temp = r->rep;
		do{
			if(temp.e->vert[0] != (*qe) ->vert[0] && temp.e->vert[0] != (*qe)->vert[2])
			{
				p4 = ((Vertex<float4>*)(temp.e->vert[0]))->p;
			}
			else if(temp.e->vert[2] != (*qe) ->vert[0] && temp.e->vert[2] != (*qe)->vert[2])
			{
				p4 = ((Vertex<float4>*)(temp.e->vert[2]))->p;
			}
			temp = temp.ONext();
		}while(temp!= r->rep);

		(*p) = 0.375*(p1+p2)+0.125*(p3+p4);

		p->w = 1.0f;
		newVertices[i + m_SubdivVertices.size()]->newIdx = i;
		(*qe)->newIdx = i;
	}// End new edge points

	// compute new vertex points
	i = 0;
	for (set<Vertex<float4>*>::iterator v = m_SubdivVertices.begin(); v != m_SubdivVertices.end(); ++v, i++)
	{
		newVertices[i]->newIdx = i;
		int n = (*v)->getN();
		float alpha;
		float4 p;
		p.x = p.y = p.z = p.w = 0.0f;

		alpha = (0.625 - (3+2*cos(2*PI / n)) * (3+2*cos(2*PI / n)) / 64)/n;

		Edge e = (*v)->rep;
		do{
			if(((Vertex<float4>*)(e.e->vert[0]))->p != (*v)->p)
				p = p+((Vertex<float4>*)(e.e->vert[0]))->p;
			else if(((Vertex<float4>*)(e.e->vert[2]))->p != (*v)->p)
				p = p+((Vertex<float4>*)(e.e->vert[2]))->p;
			e = e.ONext();
		}while(e!=(*v)->rep);
		float4* q = &(newVertices[i]->p);
		(*q) = (1-n*alpha)*(*v)->p + alpha*p; 
		q->w = 1.0f;
	}

	// add new faces
	Vertex<float4>* Faces[3];
	for (set<Face*>::iterator f = m_SubdivFaces.begin(); f != m_SubdivFaces.end(); ++f)
	{
		Edge e = (*f)->rep.OPrev();
		Edge temp;
		Face* face;
		UINT n = abs((*f)->getN());
		int tag = 2;
		do{
			//Face1 & Face2 & Face3
			if(!e.Left() || e.Left()->newIdx == (UINT)-1 || e.e->newIdx == (UINT)-1 || e.ONext().e->newIdx == (UINT)-1){
				tag--;
				e = e.ONext();
			}else	{
				Faces[0] = newVertices[e.Left()->newIdx];
				Faces[1] = newVertices[e.e->newIdx + m_SubdivVertices.size()];
				temp = e.ONext();
				Faces[2] = newVertices[temp.e->newIdx + m_SubdivVertices.size()];
				face = rvalue->addFace(Faces, 3);
				face->m_Father = (*f);
				e = e.ONext();
			}
		}while(e != (*f)->rep.OPrev());
		//Face4
		if(tag>0)
		{
			Faces[0] = newVertices[e.e->newIdx + m_SubdivVertices.size()];e = e.ONext();
			Faces[2] = newVertices[e.e->newIdx + m_SubdivVertices.size()];e = e.ONext();
			Faces[1] = newVertices[e.e->newIdx + m_SubdivVertices.size()];e = e.ONext();
			face = rvalue->addFace(Faces, 3);
			face->m_Father = (*f);
		}
	}//for

	for (set<Vertex<float4>*>::iterator v = m_SubdivVertices.begin(); v != m_SubdivVertices.end(); v++) {
		if ((*v)->vertexSubdivisions > 0) {
			if ((*v)->newIdx != (unsigned int)-1) {
				rvalue->vlist[(*v)->newIdx]->vertexSubdivisions = (*v)->vertexSubdivisions - 1;
			}
		}
	}

	SAFE_DELETE_ARRAY(newVertices);
	return rvalue;
}

HRESULT LLevel::CreateAdaptiveLoopTables( ID3D11Device* pd3dDevice )
{
	HRESULT hr = S_OK;
	UINT v0_size = 0;

	//allocate and fill edge iteration table
	m_E0_IT = new uint4[m_SubdivEdges.size()];
	for (set<TriEdge*>::iterator qe = m_SubdivEdges.begin(); qe != m_SubdivEdges.end(); ++qe){
		unsigned int idx = (*qe)->newIdx;
		UINT p1, p2, p3, p4;
		p1 = (*qe)->vert[0]->Idx;
		p2 = (*qe)->vert[2]->Idx;
		Face* l = (Face*)((*qe)->vert[1]);
		Face* r = (Face*)((*qe)->vert[3]);
		//Òí±ß±éÀú
		//left Winged edge
		Edge temp = l->rep;
		do{
			if(temp.e->vert[0] != (*qe) ->vert[0] && temp.e->vert[0] != (*qe)->vert[2])
			{
				p3 = (temp.e->vert[0])->Idx;
			}
			else if(temp.e->vert[2] != (*qe) ->vert[0] && temp.e->vert[2] != (*qe)->vert[2])
			{
				p3 = (temp.e->vert[2])->Idx;
			}
			temp = temp.ONext();
		}while(temp!= l->rep);

		temp = r->rep;
		do{
			if(temp.e->vert[0] != (*qe) ->vert[0] && temp.e->vert[0] != (*qe)->vert[2])
			{
				p4 = (temp.e->vert[0])->Idx;
			}
			else if(temp.e->vert[2] != (*qe) ->vert[0] && temp.e->vert[2] != (*qe)->vert[2])
			{
				p4 = (temp.e->vert[2])->Idx;
			}
			temp = temp.ONext();
		}while(temp!= r->rep);

		m_E0_IT[idx].x = p1;
		m_E0_IT[idx].y = p2;
		m_E0_IT[idx].z = p3;
		m_E0_IT[idx].w = p4;
	}

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory( &bufferDesc, sizeof(D3D11_BUFFER_DESC) );
	bufferDesc.Usage				= D3D11_USAGE_IMMUTABLE;
	bufferDesc.BindFlags			= D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.StructureByteStride	= sizeof(int);
	bufferDesc.ByteWidth = 4 * m_SubdivEdges.size() * sizeof(unsigned int);

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;
	InitData.pSysMem = m_E0_IT;

	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_BUF_E0_IT ));
	}else{
		m_BUF_E0_IT = NULL;
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC DescSRV;
	ZeroMemory( &DescSRV, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
	DescSRV.Format = DXGI_FORMAT_R32_UINT;
	DescSRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	DescSRV.Buffer.ElementOffset = 0;
	DescSRV.Buffer.ElementWidth = 4 * m_SubdivEdges.size();

	if (DescSRV.Buffer.ElementWidth > 0) {
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_BUF_E0_IT, &DescSRV, &m_SRV_E0_IT ));
	} else {
		m_SRV_E0_IT = NULL;
	}

	for (set<Vertex<float4>*>::iterator v = m_SubdivVertices.begin(); v != m_SubdivVertices.end(); ++v){
		short n = (*v)->getN();
		v0_size += n;
	}
	m_V0_ITa = new int[3*m_SubdivVertices.size()];
	m_V0_IT = new UINT[v0_size];
	UINT cnt=0;
	for (set<Vertex<float4>*>::iterator v = m_SubdivVertices.begin(); v != m_SubdivVertices.end(); ++v){
		short n = (*v)->getN();
		m_V0_ITa[3*(*v)->newIdx] = cnt; // Starting index
		m_V0_ITa[3*(*v)->newIdx+1] = n; //Valence
		m_V0_ITa[3*(*v)->newIdx+2] = (*v)->Idx; //Source Index

		Edge e = (*v)->rep;
		do{
			if(((Vertex<float4>*)(e.e->vert[0]))->p != (*v)->p)
				m_V0_IT[cnt++] = (e.e->vert[0])->Idx;
			else if(((Vertex<float4>*)(e.e->vert[2]))->p != (*v)->p)
				m_V0_IT[cnt++] = (e.e->vert[2])->Idx;
			e = e.ONext();
		}while(e!=(*v)->rep);
	}

	bufferDesc.ByteWidth = 3*m_SubdivVertices.size() * sizeof(int);
	InitData.pSysMem = m_V0_ITa;

	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_BUF_V0_ITa ));
	} else {
		m_BUF_V0_ITa = NULL;
	}

	DescSRV.Format = DXGI_FORMAT_R32_SINT;
	DescSRV.Buffer.ElementWidth = 3*m_SubdivVertices.size();
	if (DescSRV.Buffer.ElementWidth > 0) {
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_BUF_V0_ITa, &DescSRV, &m_SRV_V0_ITa ));
	} else {
		m_SRV_V0_ITa = NULL;
	}


	bufferDesc.ByteWidth = v0_size * sizeof(unsigned int);
	InitData.pSysMem = m_V0_IT;

	if (bufferDesc.ByteWidth > 0) {
		V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_BUF_V0_IT ));
	} else {
		m_BUF_V0_IT = NULL;
	}

	DescSRV.Format = DXGI_FORMAT_R32_UINT;
	DescSRV.Buffer.ElementWidth = v0_size;

	if (DescSRV.Buffer.ElementWidth > 0) {
		V_RETURN( pd3dDevice->CreateShaderResourceView( m_BUF_V0_IT, &DescSRV, &m_SRV_V0_IT ));
	} else {
		m_SRV_V0_IT = NULL;
	}


	return hr;
}

void LLevel::RunAdaptiveSubdivisionTablesCS(ID3D11DeviceContext* pContext, ID3D11Buffer* pcbRTCS, UINT DEST_OFFSET, ID3D11ComputeShader* pEdgePointCS, ID3D11ComputeShader* pVertexPointCS)
{
	HRESULT hr;

	UINT V0 = m_SubdivVertices.size(); // return the number of vertex at this subdivision level
	UINT E0 = m_SubdivEdges.size();

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V( pContext->Map( pcbRTCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) );
	CB_RUNTABLES* pcbrt = ( CB_RUNTABLES* )MappedResource.pData;
	pcbrt->m_NumEdges = E0;
	pcbrt->m_NumVertices = V0;

	UINT stride = 4;	// every vertex has 4 dimensions - xyzw
	pcbrt->DEST_OFFSET_1 = stride*(DEST_OFFSET + numVertices);  // Vertex Point
	pcbrt->DEST_OFFSET_2 = stride*(DEST_OFFSET + numVertices + V0); // Edge Point

	pcbrt->SRC_OFFSET = stride*DEST_OFFSET;
	pContext->Unmap( pcbRTCS, 0 );
	ID3D11Buffer* ppCB[1] = { pcbRTCS };
	pContext->CSSetConstantBuffers( 1, 1, ppCB );

	ID3D11ShaderResourceView* aSRViews[ 3 ] = { m_SRV_E0_IT, m_SRV_V0_ITa, m_SRV_V0_IT};
	pContext->CSSetShaderResources( 0, 3, aSRViews );

	pContext->CSSetShader( pEdgePointCS, NULL, 0 );
	pContext->Dispatch( (E0+63)/64, 1, 1 );

	pContext->CSSetShader( pVertexPointCS, NULL, 0 );
	pContext->Dispatch( (V0+7)/8, 1, 1 );

	ID3D11ShaderResourceView* aSRVNULL[ 3 ] = { NULL, NULL, NULL };
	pContext->CSSetShaderResources( 0, 3, aSRVNULL );
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////
ControlMesh::ControlMesh(void)
{
	m_pVertexOffsets = NULL;
	m_BaseMesh = NULL;
	m_MeshSubdiviedLevels = NULL;
	m_pVertexBuffer = NULL;
	m_pVertexBufferSRV = NULL;
	m_pVertexBufferSRV4Components = NULL;
	m_pVertexBufferUAV = NULL;
	m_pValenceBuffer = NULL;
	m_pValenceBufferSRV = NULL;

	m_pConstantBufferRunTables = NULL;
}

ControlMesh::~ControlMesh(void)
{}

bool Vcompare(BaseVertex* a, BaseVertex* b)
{
	return a->getN() < b->getN();
}

HRESULT ControlMesh::Create( ID3D11Device* pd3dDevice, const ObjectDescriptor& obd)
{
	HRESULT hr = S_OK;

	m_BaseMesh = CreateMeshFromFile(obd.m_Filename);

	if (obd.m_IsolateExtraordinary) {
		for (UINT i = 0; i < m_BaseMesh->vlist.size(); i++) {
			if (m_BaseMesh->vlist[i]->isExtraordinary()) {
				m_BaseMesh->vlist[i]->vertexSubdivisions = obd.m_IsolateExtraordinary;
			}
		}
	}

	sort ( m_BaseMesh->vlist.begin(), m_BaseMesh->vlist.end(), Vcompare );
	sort ( m_BaseMesh->flist.begin(), m_BaseMesh->flist.end(), Vcompare );
	
	// initialize the index of each vertex ----------------------------------------
	for (UINT i = 0; i < m_BaseMesh->vlist.size(); i++) {
		m_BaseMesh->vlist[i]->Idx = i;
		if (abs(m_BaseMesh->vlist[i]->getN()) >= MAX_VALENCE)	{
			m_BaseMesh->Destroy();
			SAFE_DELETE_ARRAY(m_BaseMesh);
			throw MeshError("vertex valence too high");
		}
	}
	for (UINT i = 0; i < m_BaseMesh->flist.size(); i++) {
		m_BaseMesh->flist[i]->Idx = i;
		if (abs(m_BaseMesh->flist[i]->getN()) >= MAX_VALENCE)	{
			m_BaseMesh->Destroy();
			SAFE_DELETE(m_BaseMesh);
			throw MeshError("face valence too high");
		}
	}
	//------------------------------------------------------------------------
	m_MeshNumSubdivLevels =8; // assign the number of subdivision level
	m_VertexBufferSize = 0;
	if (m_MeshNumSubdivLevels == 0)	{
		m_BaseMesh->Destroy();
		SAFE_DELETE(m_BaseMesh);
		throw MeshError("need at least one subdivision level");
	}

	m_MeshSubdiviedLevels = new LLevel*[m_MeshNumSubdivLevels];
	m_MeshSubdiviedLevels[0] = m_BaseMesh;
	m_VertexBufferSize = m_BaseMesh->vlist.size();

	m_pVertexOffsets = new UINT[m_MeshNumSubdivLevels + 1];
	// get the table
	m_pVertexOffsets[0] = 0;
	for (UINT i = 1; i < m_MeshNumSubdivLevels; i++) {
		m_MeshSubdiviedLevels[i] = m_MeshSubdiviedLevels[i-1]->AdaptiveLoop();
		m_pVertexOffsets[i] = m_MeshSubdiviedLevels[i-1]->vlist.size() + m_pVertexOffsets[i-1];
	}	
	m_pVertexOffsets[m_MeshNumSubdivLevels] =  m_pVertexOffsets[m_MeshNumSubdivLevels-1] + m_MeshSubdiviedLevels[m_MeshNumSubdivLevels - 1]->vlist.size();
	
	for (UINT i = 0; i < m_MeshNumSubdivLevels-1; i++) {
		m_MeshSubdiviedLevels[i]->CreateAdaptiveLoopTables(pd3dDevice);
	}


	m_VertexBufferSize = m_pVertexOffsets[m_MeshNumSubdivLevels];

	//Create The Vertex Buffer =============================================================
	float4* initVertexData = new float4[m_VertexBufferSize];
	/*for(UINT level = 0; level <m_MeshNumSubdivLevels; level ++)
	{
		for (vector<Vertex<float4>*>::iterator v = m_MeshSubdiviedLevels[level]->vlist.begin(); v != m_MeshSubdiviedLevels[level]->vlist.end(); v++){
			initVertexData[(*v)->Idx + m_pVertexOffsets[level] ] = (*v)->p;
		}
	}*/
	for (vector<Vertex<float4>*>::iterator v = m_BaseMesh->vlist.begin(); v != m_BaseMesh->vlist.end(); v++){
		initVertexData[(*v)->Idx] = (*v)->p;
	}


	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory( &bufferDesc, sizeof(D3D11_BUFFER_DESC) );
	bufferDesc.Usage				= D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth			= m_VertexBufferSize * sizeof(float4); 
	bufferDesc.BindFlags			= D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	bufferDesc.StructureByteStride	= sizeof(float4);
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
	InitData.pSysMem = initVertexData; //////////////////initVertexData
	V_RETURN(pd3dDevice->CreateBuffer( &bufferDesc, &InitData, &m_pVertexBuffer));

	//Create SRV
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	ZeroMemory( &SRVDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC) );
	SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.NumElements = 4 * m_VertexBufferSize;
	V_RETURN(pd3dDevice->CreateShaderResourceView( m_pVertexBuffer, &SRVDesc, &m_pVertexBufferSRV));


	SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement = 0;
	SRVDesc.Buffer.NumElements = m_VertexBufferSize;
	V_RETURN(pd3dDevice->CreateShaderResourceView( m_pVertexBuffer, &SRVDesc, &m_pVertexBufferSRV4Components));


	D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
	ZeroMemory( &DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
	DescUAV.Format = DXGI_FORMAT_R32_FLOAT; // R32 is necessary, since in such case we can use numthread[64, 4, 1]. That means, we can use 4 threads in y direction to calculate 4 coordinate components respectively
	DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	DescUAV.Buffer.FirstElement = 0;
	DescUAV.Buffer.NumElements = 4 * m_VertexBufferSize;
	V_RETURN( pd3dDevice->CreateUnorderedAccessView( m_pVertexBuffer, &DescUAV, &m_pVertexBufferUAV ));

	SAFE_DELETE_ARRAY(initVertexData);
	// ==============================================================================

	// constant buffer
	ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bufferDesc.MiscFlags = 0;

	bufferDesc.ByteWidth = sizeof( CB_RUNTABLES );
	V_RETURN( pd3dDevice->CreateBuffer( &bufferDesc, NULL, &m_pConstantBufferRunTables ) );


	//Create ValenceBuffer & corresponding SRV ===================
	CreateAllValenceBuffer(pd3dDevice);

	//Patch Cases Calculation 
	//And Create index buffers ===============================
	for(UINT level = 0; level <m_MeshNumSubdivLevels; level ++){
		m_MeshSubdiviedLevels[level]->CalPatches();
		m_MeshSubdiviedLevels[level]->CreateIndexBufferRegularPatch(pd3dDevice);
		m_MeshSubdiviedLevels[level]->CreateIndexBufferIrregularPatch(pd3dDevice);
		m_MeshSubdiviedLevels[level]->CreatePartialCases(pd3dDevice, m_MeshSubdiviedLevels[level]->m_PartialPatch); //Create Index Buffers
	}

	for(UINT level = 0; level <m_MeshNumSubdivLevels; level ++){
		V_RETURN(m_MeshSubdiviedLevels[level]->m_PartialCase0.CreateShaders(pd3dDevice));
		V_RETURN(m_MeshSubdiviedLevels[level]->m_PartialCase1.CreateShaders(pd3dDevice));
		V_RETURN(m_MeshSubdiviedLevels[level]->m_PartialCase2.CreateShaders(pd3dDevice));
	}
	
	return hr;
}

void ControlMesh::Destroy()
{
	if (m_BaseMesh)	m_BaseMesh->Destroy();
	SAFE_DELETE (m_BaseMesh);
	if (m_MeshSubdiviedLevels) {
		for (unsigned int i = 1; i < m_MeshNumSubdivLevels; i++) {
			if(m_MeshSubdiviedLevels[i]) m_MeshSubdiviedLevels[i]->Destroy();
			SAFE_DELETE(m_MeshSubdiviedLevels[i]);
		}
		SAFE_DELETE_ARRAY(m_MeshSubdiviedLevels);
	}
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pVertexBufferSRV);
	SAFE_RELEASE(m_pVertexBufferSRV4Components);
	SAFE_RELEASE(m_pVertexBufferUAV);

	SAFE_RELEASE(m_pConstantBufferRunTables);

	for (UINT i = 0; i < m_MeshNumSubdivLevels; i++) {
		if(m_pValenceBuffer)		SAFE_RELEASE(m_pValenceBuffer[i]);
		if(m_pValenceBufferSRV)		SAFE_RELEASE(m_pValenceBufferSRV[i]);
	}
	SAFE_DELETE_ARRAY(m_pValenceBuffer);
	SAFE_DELETE_ARRAY(m_pValenceBufferSRV);

	SAFE_RELEASE(s_pRasterizerStateSolid);
	SAFE_RELEASE(s_pRasterizerStateWireframe);

	SAFE_RELEASE(s_pConstantBufferPerLevel);

	SAFE_DELETE_ARRAY(m_pVertexOffsets);
}

void ControlMesh::FrameRender( ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dImmediateContext, UINT stop)
{
	assert(stop<m_MeshNumSubdivLevels);
	pd3dImmediateContext->IASetInputLayout(s_pVertexLayout);

	GPUSubdivide(pd3dImmediateContext, stop);

	float tessLevel = m_fTessFactor*2;
	float tessLevelNext = tessLevel * 0.5f;

	for (UINT level = 0; level<=stop; level++){

		D3D11_MAPPED_SUBRESOURCE MappedResource;
		pd3dImmediateContext->Map( s_pConstantBufferPerLevel, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
		CB_PER_LEVEL_CONSTANTS* pData = ( CB_PER_LEVEL_CONSTANTS* )MappedResource.pData;

		pData->fTessellationFactor = tessLevel;
		pData->fTessellationFactorNext = max(tessLevelNext, 1.0f);
		pData->g_offset = m_pVertexOffsets[level];
		pd3dImmediateContext->Unmap( s_pConstantBufferPerLevel, 0 );

		pd3dImmediateContext->VSSetConstantBuffers( s_uCBPerLevelBind, 1, &s_pConstantBufferPerLevel );
		pd3dImmediateContext->HSSetConstantBuffers( s_uCBPerLevelBind, 1, &s_pConstantBufferPerLevel );
		pd3dImmediateContext->DSSetConstantBuffers( s_uCBPerLevelBind, 1, &s_pConstantBufferPerLevel );
		pd3dImmediateContext->PSSetConstantBuffers( s_uCBPerLevelBind, 1, &s_pConstantBufferPerLevel );

		pd3dImmediateContext->VSSetShaderResources(0, 1, &m_pVertexBufferSRV4Components);
		pd3dImmediateContext->VSSetShaderResources(1, 1, &m_pValenceBufferSRV[level]);

		// Rendering begins here. =====================
		if(level == stop)
		{
			m_MeshSubdiviedLevels[stop]->DrawIndexedEndIrregularPatches(pd3dImmediateContext, m_pVertexBuffer, m_pVertexOffsets[stop], stop);

			m_MeshSubdiviedLevels[stop]->DrawIndexedEndPatches(pd3dImmediateContext, m_pVertexBuffer, m_pVertexOffsets[stop], stop);
		}
		else
		{
			//m_MeshSubdiviedLevels[level]->DrawIndexedPartialPatches(pd3dImmediateContext, m_pVertexBuffer, m_pVertexOffsets[level], level);
			m_MeshSubdiviedLevels[level]->DrawIndexedPartialPatchesWatertight(pd3dImmediateContext, m_pVertexBuffer, m_pVertexOffsets[level], level);

			m_MeshSubdiviedLevels[level]->DrawIndexedFullPatches(pd3dImmediateContext, m_pVertexBuffer, m_pVertexOffsets[level], level);
		}
		// Rendering ends here. ======================

		tessLevel = tessLevelNext;
		tessLevelNext = max(tessLevelNext*0.5f, 1.0f);
	}
}

LLevel* ControlMesh::GetLLevel(UINT i)
{
	assert(i < m_MeshNumSubdivLevels);
	return m_MeshSubdiviedLevels[i];
}

void ControlMesh::BindVertexBuffer( ID3D11DeviceContext* pd3dImmediateContext )
{
	UINT offset = 0;
	UINT stride = sizeof(float4);

	pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
}

HRESULT ControlMesh::CreateShaders( ID3D11Device* pd3dDevice )
{
	HRESULT hr = S_OK;

	if (s_bShadersAreCreated)	return hr;

	ID3DBlob* pBlobVS	= NULL;
	V_RETURN( CompileShaderFromFile( L"GPULoopSubdiv.hlsl", NULL, "BezierVS", "vs_5_0", &pBlobVS)) ;
	V_RETURN( pd3dDevice->CreateVertexShader( pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), NULL, &s_pVertexShader) );
	DXUT_SetDebugName( s_pVertexShader, "BezierVS" );

	V_RETURN( CompileShaderFromFile( L"GPULoopSubdiv.hlsl", NULL, "VSMain", "vs_5_0", &pBlobVS)) ;
	V_RETURN( pd3dDevice->CreateVertexShader( pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), NULL, &s_pVertexShaderLines) );
	DXUT_SetDebugName( s_pVertexShader, "VSMain" );

	V_RETURN( CompileShaderFromFile( L"GPULoopSubdiv.hlsl", NULL, "VSMainLimit", "vs_5_0", &pBlobVS)) ;
	V_RETURN( pd3dDevice->CreateVertexShader( pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), NULL, &s_pVertexShaderLimit) );
	DXUT_SetDebugName( s_pVertexShader, "VSMainLimit" );

	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	V_RETURN( pd3dDevice->CreateInputLayout( layout, ARRAYSIZE( layout ), pBlobVS->GetBufferPointer(), pBlobVS->GetBufferSize(), &s_pVertexLayout ) );
	DXUT_SetDebugName( s_pVertexLayout, "Primary" );

	ID3DBlob* pBlobPS	= NULL;

	V_RETURN( CompileShaderFromFile( L"GPULoopSubdiv.hlsl", NULL, "PSLinesGreen", "ps_5_0", &pBlobPS)) ;
	V_RETURN( pd3dDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), NULL, &s_pPixelShaderGreen) );
	DXUT_SetDebugName( s_pPixelShaderGreen, "PSGreen" );

	V_RETURN( CompileShaderFromFile( L"GPULoopSubdiv.hlsl", NULL, "PSLinesRed", "ps_5_0", &pBlobPS)) ;
	V_RETURN( pd3dDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), NULL, &s_pPixelShaderRed) );
	DXUT_SetDebugName( s_pPixelShaderGreen, "PSRed" );

	V_RETURN( CompileShaderFromFile( L"GPULoopSubdiv.hlsl", NULL, "PSLinesBlue", "ps_5_0", &pBlobPS)) ;
	V_RETURN( pd3dDevice->CreatePixelShader( pBlobPS->GetBufferPointer(), pBlobPS->GetBufferSize(), NULL, &s_pPixelShaderBlue) );
	DXUT_SetDebugName( s_pPixelShaderGreen, "PSBlue" );

	ID3DBlob* pBlobHS	= NULL;
	V_RETURN(CompileShaderFromFile( L"GPULoopSubdiv.hlsl", NULL, "BezierHS", "hs_5_0", &pBlobHS));
	V_RETURN( pd3dDevice->CreateHullShader( pBlobHS->GetBufferPointer(), pBlobHS->GetBufferSize(),NULL, &s_pHullShader));
	DXUT_SetDebugName( s_pHullShader, "BezierHS" );

	ID3DBlob* pBlobDS	= NULL;
	V_RETURN(CompileShaderFromFile( L"GPULoopSubdiv.hlsl", NULL, "BezierDS", "ds_5_0", &pBlobDS));
	V_RETURN( pd3dDevice->CreateDomainShader( pBlobDS->GetBufferPointer(), pBlobDS->GetBufferSize() ,NULL, &s_pDomainShader));
	DXUT_SetDebugName( s_pDomainShader, "BezierDS" );

	ID3DBlob* pBlobCalcCS = NULL;
	V_RETURN( CompileShaderFromFile( L"SubdivisionCS.hlsl", NULL, "EdgePointCS", "cs_5_0", &pBlobCalcCS));
	V_RETURN( pd3dDevice->CreateComputeShader( pBlobCalcCS->GetBufferPointer(), pBlobCalcCS->GetBufferSize(), NULL, &s_pEdgePointCS));
	DXUT_SetDebugName( s_pEdgePointCS, "EdgePointCS" );

	V_RETURN( CompileShaderFromFile( L"SubdivisionCS.hlsl", NULL, "VertexPointCS", "cs_5_0", &pBlobCalcCS));
	V_RETURN( pd3dDevice->CreateComputeShader( pBlobCalcCS->GetBufferPointer(), pBlobCalcCS->GetBufferSize(), NULL, &s_pVertexPointCS));
	DXUT_SetDebugName( s_pVertexPointCS, "VertexPointCS" );

	SAFE_RELEASE(pBlobVS);
	SAFE_RELEASE(pBlobPS);
	SAFE_RELEASE(pBlobHS);
	SAFE_RELEASE(pBlobDS);
	SAFE_RELEASE(pBlobCalcCS);

	s_bShadersAreCreated = true;

	return hr;
}
void ControlMesh::DestroyShaders()
{
	if (!s_bShadersAreCreated)	return;

	SAFE_RELEASE(s_pVertexLayout);
	SAFE_RELEASE(s_pVertexShader);
	SAFE_RELEASE(s_pPixelShaderGreen);
	SAFE_RELEASE(s_pPixelShaderRed);
	SAFE_RELEASE(s_pPixelShaderBlue);
	SAFE_RELEASE(s_pHullShader);
	SAFE_RELEASE(s_pDomainShader);

	SAFE_RELEASE(s_pEdgePointCS);
	SAFE_RELEASE(s_pVertexPointCS);

	SAFE_RELEASE(s_pVertexShaderLines);
	SAFE_RELEASE(s_pVertexShaderLimit);

	s_bShadersAreCreated = false;
}

HRESULT ControlMesh::CreateValenceBuffer(ID3D11Device* pd3dDevice, UINT level)
{
	HRESULT hr = S_OK;

	int n = 0;
	int adjacent = 0;
	int cnt = (MAX_VALENCE + 1)*GetLLevel(level)->vlist.size();
	unsigned int* indices = new unsigned int[cnt];
	memset( indices, 0, cnt*sizeof( unsigned int ));
	for(UINT i = 0; i<GetLLevel(level)->vlist.size();i++){
		n = GetLLevel(level)->vlist[i]->getN();
		assert(abs(n) < MAX_VALENCE);

		Edge e = GetLLevel(level)->vlist[i]->rep;
		unsigned int curr = 1;

		indices[i*(MAX_VALENCE + 1)] = n;

		for(UINT j = 0; j< (UINT)abs(n); j++){
			adjacent = e.Dest()->Idx;
			indices[i*(MAX_VALENCE + 1) + curr++] = adjacent;
			e = e.ONext();
		}
		assert(curr == (UINT)abs(n) + 1);
	}

	D3D11_BUFFER_DESC valenceBufferDesc;
	ZeroMemory(&valenceBufferDesc, sizeof(D3D11_BUFFER_DESC));
	valenceBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	valenceBufferDesc.ByteWidth = cnt * sizeof(UINT);
	valenceBufferDesc.StructureByteStride = sizeof(UINT);
	valenceBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(D3D11_SUBRESOURCE_DATA) );
	InitData.pSysMem = indices;
	if (valenceBufferDesc.ByteWidth > 0) {
		V_RETURN(pd3dDevice->CreateBuffer(&valenceBufferDesc, &InitData,&m_pValenceBuffer[level]));
	}
	else m_pValenceBuffer[level] = NULL;

	D3D11_SHADER_RESOURCE_VIEW_DESC valenceSRVDesc;
	ZeroMemory( &valenceSRVDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
	valenceSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	valenceSRVDesc.Buffer.FirstElement = 0;
	valenceSRVDesc.Buffer.NumElements = cnt;
	valenceSRVDesc.Format = DXGI_FORMAT_R32_SINT;

	if (valenceSRVDesc.Buffer.NumElements > 0) {
		V_RETURN(pd3dDevice->CreateShaderResourceView(m_pValenceBuffer[level], &valenceSRVDesc, &m_pValenceBufferSRV[level]));
	}
	else m_pValenceBufferSRV[level] = NULL;

	SAFE_DELETE_ARRAY(indices);

	return hr;
}

HRESULT ControlMesh::CreateAllValenceBuffer(ID3D11Device* pd3dDevice)
{
	HRESULT hr = S_OK;

	m_pValenceBuffer = new ID3D11Buffer*[m_MeshNumSubdivLevels];
	m_pValenceBufferSRV = new ID3D11ShaderResourceView*[m_MeshNumSubdivLevels];

	for(UINT level = 0; level <m_MeshNumSubdivLevels; level ++)
		CreateValenceBuffer(pd3dDevice, level);

	return hr;
}

void ControlMesh::GPUSubdivide( ID3D11DeviceContext* pd3dImmediateContext, UINT stop)
{
	ID3D11Buffer* VBnull[1] = {NULL};
	UINT voffset = 0;
	UINT vstride = 0;
	pd3dImmediateContext->IASetVertexBuffers(0, 1, VBnull, &voffset, &vstride); // release vertex buffer

	ID3D11UnorderedAccessView* aUAViews[ 1 ] = { m_pVertexBufferUAV }; // compute shader UAV£¡
	pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, aUAViews, NULL );
	UINT offset = 0;
	for (UINT i = 0; i < stop; i++) {
		m_MeshSubdiviedLevels[i]->RunAdaptiveSubdivisionTablesCS(pd3dImmediateContext, m_pConstantBufferRunTables, offset, s_pEdgePointCS, s_pVertexPointCS);
		offset += m_MeshSubdiviedLevels[i]->vlist.size();
	}

	ID3D11UnorderedAccessView* ppUAVNULL[1] = { NULL};
	pd3dImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAVNULL, NULL );
}