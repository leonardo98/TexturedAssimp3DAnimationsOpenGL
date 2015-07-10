#include <string>
#include <map>
#include <vector>

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/postprocess.h"
#include "assimp/LogStream.hpp"

#include "MathFunc.h"

#define NUM_BONES_PER_VEREX 4

struct VertexBoneData
{        
    uint IDs[NUM_BONES_PER_VEREX];
    float Weights[NUM_BONES_PER_VEREX];

    VertexBoneData()
    {
        Reset();
    };
        
    void Reset()
    {
		memset(IDs, 0, sizeof(IDs));
		memset(Weights, 0, sizeof(Weights));
    }
        
    
	void AddBoneData(uint BoneID, float Weight)
	{
		for (uint i = 0 ; i < NUM_BONES_PER_VEREX; i++) {
			if (Weights[i] == 0.0) {
				IDs[i] = BoneID;
				Weights[i] = Weight;
				return;
			} 
		}

		// should never get here - more bones than we have space for
		assert(false);
	} 
}; 

struct BoneInfo
{
	aiMatrix4x4 BoneOffset;
	aiMatrix4x4 FinalTransformation;
};

#define INVALID_MATERIAL 0xFFFFFFFF;

struct MeshEntry {
	MeshEntry()
	{
		NumIndices = 0;
		BaseVertex = 0;
		BaseIndex = 0;
		MaterialIndex = INVALID_MATERIAL;
	}

	unsigned int NumIndices;
	unsigned int BaseVertex;
	unsigned int BaseIndex;
	unsigned int MaterialIndex;
};

class AnimationController
{
private:

	aiVector3D _rotation;

	std::string m_ModelPath;

	long long m_startTime;

	// Create an instance of the Importer class
	std::vector<Assimp::Importer *> _importers;
	
	// вся сцена из файла храниться тут
	std::vector<const aiScene*> _scenes;
	uint _curScene;

	// todo: узнать зачем это
	aiMatrix4x4 m_GlobalInverseTransform;

	// соотношенеи имя кости и ее порядкового номера в m_BoneInfo
	std::map<std::string, uint> m_BoneMapping; 

	// images / texture
	std::map<std::string, GLuint*> textureIdMap;	// map image filenames to textureIds
	GLuint*		textureIds;							// pointer to texture Array

	// Используется на этапе инициализации
	// todo: избавится от этого , переделать на временную переменную чтобы не захламлять код
	std::vector<MeshEntry> m_Entries;

	// исходное и финальное(для кадра) при анимации положение кости
	std::vector<BoneInfo> m_BoneInfo;

	uint m_NumBones;

	// todo: узнать зачем это
	std::vector<aiMatrix4x4> Transforms;

	// вся информация о сетке модели
	std::vector<aiVector3D> m_Vericies;
	std::vector<VertexBoneData> m_Mass;
	std::vector<aiVector3D> m_Normales;
	std::vector<aiVector2D> m_TextureUVCoords;
	std::vector<uint> m_Indexes;

	// вершины и нормали меняются при анмиации 
	// изменения между Update и Render храним тут
	std::vector<aiVector3D> m_VericiesOut;
	std::vector<aiVector3D> m_NormalesOut;

	void apply_material(const aiMaterial *mtl)
	{
		float c[4];

		GLenum fill_mode;
		int ret1, ret2;
		aiColor4D diffuse;
		aiColor4D specular;
		aiColor4D ambient;
		aiColor4D emission;
		float shininess, strength;
		int two_sided;
		int wireframe;
		unsigned int max;	// changed: to unsigned

		int texIndex = 0;
		aiString texPath;	//contains filename of texture

		if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath))
		{
			//bind texture
			unsigned int texId = *textureIdMap[texPath.data];
			glBindTexture(GL_TEXTURE_2D, texId);
		}

		set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
		if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
			color4_to_float4(&diffuse, c);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
			color4_to_float4(&specular, c);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

		set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
		if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
			color4_to_float4(&ambient, c);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

		set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
		if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
			color4_to_float4(&emission, c);
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

		max = 1;
		ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
		max = 1;
		ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
		if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
		else {
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
			set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
		}

		max = 1;
		if(AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
			fill_mode = wireframe ? GL_LINE : GL_FILL;
		else
			fill_mode = GL_FILL;
		glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

		max = 1;
		if((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
			glEnable(GL_CULL_FACE);
		else
			glDisable(GL_CULL_FACE);
	}

public:

	void SetRotation(const aiVector3D &r) { _rotation = r; }
	aiVector3D GetRotation() { return _rotation; }

	~AnimationController()
	{
		Release();
	}

	void Release()
	{
		// *** cleanup ***

		textureIdMap.clear(); //no need to delete pointers in it manually here. (Pointers point to textureIds deleted in next step)

		if (textureIds)
		{
			delete[] textureIds;
			textureIds = NULL;
		}

		for (uint i = 0; i < _importers.size(); ++i)
		{
			delete _importers[i];
		}
		_importers.clear();
	}

	AnimationController(const char *modelpath) 
		: m_NumBones(0)
		, _curScene(NULL)
		, m_ModelPath(modelpath)
		, _rotation(0.f)
	{}

	void ReadNodeHeirarchy(const aiScene * scene, float AnimationTime, const aiNode* pNode, const aiMatrix4x4& ParentTransform, int stopAnimLevel)
	{ 
		float time(AnimationTime);

		std::string NodeName(pNode->mName.data);

		const aiAnimation* pAnimation = scene->mAnimations[0];

		aiMatrix4x4 NodeTransformation(pNode->mTransformation);

		const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

		if (pNodeAnim) {
			// Interpolate scaling and generate scaling transformation matrix
			aiVector3D Scaling;
			CalcInterpolatedScaling(Scaling, time, pNodeAnim);
			aiMatrix4x4 ScalingM;
			aiMatrix4x4::Scaling(Scaling, ScalingM);

			// Interpolate rotation and generate rotation transformation matrix
			aiQuaternion RotationQ;
			CalcInterpolatedRotation(RotationQ, time, pNodeAnim); 
			aiMatrix4x4 RotationM;
			InitM4FromM3(RotationM, RotationQ.GetMatrix());

			// Interpolate translation and generate translation transformation matrix
			aiVector3D Translation;
			{
				float time(stopAnimLevel <= 0 ? AnimationTime : 0.f);
				CalcInterpolatedPosition(Translation, time, pNodeAnim);
			}
			aiMatrix4x4 TranslationM;
			aiMatrix4x4::Translation(Translation, TranslationM);

			// Combine the above transformations
			NodeTransformation = TranslationM * RotationM * ScalingM;
		}
		stopAnimLevel--;

		aiMatrix4x4 GlobalTransformation = ParentTransform * NodeTransformation;

		if (m_BoneMapping.find(NodeName) != m_BoneMapping.end()) {
			uint BoneIndex = m_BoneMapping[NodeName];
			m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * 
				m_BoneInfo[BoneIndex].BoneOffset;
		}

		for (uint i = 0 ; i < pNode->mNumChildren ; i++) {
			ReadNodeHeirarchy(scene, AnimationTime, pNode->mChildren[i], GlobalTransformation, stopAnimLevel);
		}
	} 

	void BoneTransform(float TimeInSeconds, std::vector<aiMatrix4x4>& Transforms)
	{
		aiMatrix4x4 Identity;
		InitIdentityM4(Identity);

		float TicksPerSecond = _scenes[_curScene]->mAnimations[0]->mTicksPerSecond != 0 ? 
			_scenes[_curScene]->mAnimations[0]->mTicksPerSecond : 25.0f;
		float TimeInTicks = TimeInSeconds * TicksPerSecond;
		float AnimationTime = fmod(TimeInTicks, _scenes[_curScene]->mAnimations[0]->mDuration);

		ReadNodeHeirarchy(_scenes[_curScene], AnimationTime, _scenes[_curScene]->mRootNode, Identity, 2);

		Transforms.resize(m_NumBones);

		for (uint i = 0 ; i < m_NumBones ; i++) {
			Transforms[i] = m_BoneInfo[i].FinalTransformation;
		}
	}

	void LoadBones(uint MeshIndex, const aiMesh* pMesh)
	{
		for (uint i = 0 ; i < pMesh->mNumBones ; i++) { 
			uint BoneIndex = 0; 
			std::string BoneName(pMesh->mBones[i]->mName.data);

			if (m_BoneMapping.find(BoneName) == m_BoneMapping.end()) {
				BoneIndex = m_NumBones;
				m_NumBones++; 
				BoneInfo bi; 
				m_BoneInfo.push_back(bi);
			}
			else {
				BoneIndex = m_BoneMapping[BoneName];
			}

			m_BoneMapping[BoneName] = BoneIndex;
			m_BoneInfo[BoneIndex].BoneOffset = pMesh->mBones[i]->mOffsetMatrix;

			for (uint j = 0 ; j < pMesh->mBones[i]->mNumWeights ; j++) {
				uint VertexID = m_Entries[MeshIndex].BaseVertex + pMesh->mBones[i]->mWeights[j].mVertexId;
				float Weight = pMesh->mBones[i]->mWeights[j].mWeight; 
				m_Mass[VertexID].AddBoneData(BoneIndex, Weight);
			}
		} 

		Transforms.resize(m_NumBones);
	} 

	bool InitFromScene(const aiScene* pScene)
	{ 
		m_startTime = -1;

		m_Entries.resize(pScene->mNumMeshes);

		uint NumVertices = 0;
		uint NumIndices = 0;

		// Count the number of vertices and indices
		for (uint i = 0 ; i < m_Entries.size() ; i++) {
			m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;        
			m_Entries[i].NumIndices    = pScene->mMeshes[i]->mNumFaces * 3;
			m_Entries[i].BaseVertex    = NumVertices;
			m_Entries[i].BaseIndex     = NumIndices;
        
			NumVertices += pScene->mMeshes[i]->mNumVertices;
			NumIndices  += m_Entries[i].NumIndices;
		}

		m_Vericies.resize(NumVertices);
		m_Normales.resize(NumVertices);
		m_TextureUVCoords.resize(NumVertices);
		m_Mass.resize(NumVertices);
		m_Indexes.reserve(NumIndices);

		for (uint i = 0; i < pScene->mNumMeshes; ++i)
		{
			LoadBones(i, pScene->mMeshes[i]);
			if (pScene->mMeshes[i]->mTextureCoords[0])
			{
				for (uint j = 0; j < pScene->mMeshes[i]->mNumVertices; ++j)
				{
					m_Vericies[m_Entries[i].BaseVertex + j] = pScene->mMeshes[i]->mVertices[j];
					m_Normales[m_Entries[i].BaseVertex + j] = pScene->mMeshes[i]->mNormals[j];
					const aiVector3D &texCoord = pScene->mMeshes[i]->mTextureCoords[0][j];
					aiVector2D &dst = m_TextureUVCoords[m_Entries[i].BaseVertex + j];
					dst.x = texCoord.x;
					dst.y = texCoord.y;
				}
			}
			// Populate the index buffer
			for (uint j = 0 ; j < pScene->mMeshes[i]->mNumFaces ; j++) {
				const aiFace& Face = pScene->mMeshes[i]->mFaces[j];
				assert(Face.mNumIndices == 3);
				m_Indexes.push_back(m_Entries[i].BaseVertex + Face.mIndices[0]);
				m_Indexes.push_back(m_Entries[i].BaseVertex + Face.mIndices[1]);
				m_Indexes.push_back(m_Entries[i].BaseVertex + Face.mIndices[2]);
			}
		}
		//     ...
		//glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(bones[0]) * bones.size(), &bones[0], GL_STATIC_DRAW);
		//glEnableVertexAttribArray(BONE_ID_LOCATION);
		//glVertexAttribIPointer(BONE_ID_LOCATION, 4, GL_INT, sizeof(VertexBoneData), (const GLvoid*)0);
		//glEnableVertexAttribArray(BONE_WEIGHT_LOCATION); 
		//glVertexAttribPointer(BONE_WEIGHT_LOCATION, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)16);
		//    ...
		return true;
	}

	bool Import3DFromFile( const std::string& pFile = "")
	{
		if (pFile.length())
			m_ModelPath = pFile;

		// Check if file exists
		std::ifstream fin(m_ModelPath.c_str());
		if(!fin.fail())
		{
			fin.close();
		}
		else
		{
			MessageBoxA(NULL, ("Couldn't open file: " + m_ModelPath).c_str() , "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		_curScene = _importers.size();
		_importers.push_back(new Assimp::Importer);
		_scenes.push_back(_importers[_curScene]->ReadFile( m_ModelPath, aiProcessPreset_TargetRealtime_Quality ));
		//_curScene = importer.ReadFile( m_ModelPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals );

		bool ret = false;
		// If the import failed, report it
		if (_scenes[_curScene]) { 
			m_GlobalInverseTransform = _scenes[_curScene]->mRootNode->mTransformation;
			m_GlobalInverseTransform.Inverse();
			ret = InitFromScene(_scenes[_curScene]);
			// Now we can access the file's contents.
			logInfo("Import of _curScene " + m_ModelPath + " succeeded.");
		}
		 else {
			 logInfo( _importers[_curScene]->GetErrorString());
		 }

		// We're done. Everything will be cleaned up by the importer destructor
		return ret;
	}

	void recursive_render (const struct aiScene *sc, const struct aiNode* nd, float scale)
	{
		unsigned int i;
		unsigned int n=0, t;
		aiMatrix4x4 m = nd->mTransformation;

		aiMatrix4x4 m2;
		aiMatrix4x4::Scaling(aiVector3D(scale, scale, scale), m2);
		m = m * m2;

		// update transform
		m.Transpose();
		glPushMatrix();
		glMultMatrixf((float*)&m);

		// draw all meshes assigned to this node
		for (; n < nd->mNumMeshes; ++n)
		{
			const struct aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

			apply_material(sc->mMaterials[mesh->mMaterialIndex]);


			if(mesh->mNormals == NULL)
			{
				glDisable(GL_LIGHTING);
			}
			else
			{
				glEnable(GL_LIGHTING);
			}

			if(mesh->mColors[0] != NULL)
			{
				glEnable(GL_COLOR_MATERIAL);
			}
			else
			{
				glDisable(GL_COLOR_MATERIAL);
			}


			glBegin(GL_TRIANGLES);

			for (t = 0; t < mesh->mNumFaces; ++t) {
				const struct aiFace* face = &mesh->mFaces[t];
				GLenum face_mode;

				switch(face->mNumIndices)
				{
					case 1: face_mode = GL_POINTS; break;
					case 2: face_mode = GL_LINES; break;
					case 3: face_mode = GL_TRIANGLES; break;
					default: face_mode = GL_POLYGON; break;
				}

				glBegin(face_mode);

				for(i = 0; i < face->mNumIndices; i++)		// go through all vertices in face
				{
					int vertexIndex = face->mIndices[i];	// get group index for current index
					if(mesh->mColors[0] != NULL)
						Color4f(&mesh->mColors[0][vertexIndex]);
					if(mesh->mNormals != NULL)

						if(mesh->HasTextureCoords(0))		//HasTextureCoords(texture_coordinates_set)
						{
							glTexCoord2f(m_TextureUVCoords[m_Entries[n].BaseVertex + vertexIndex].x, 1 - m_TextureUVCoords[m_Entries[n].BaseVertex + vertexIndex].y); //mTextureCoords[channel][vertex]
						}

						//glNormal3fv(&m_Normales[m_Entries[n].BaseVertex + vertexIndex].x);
						glNormal3fv(&m_NormalesOut[m_Entries[n].BaseVertex + vertexIndex].x);
						glVertex3fv(&m_VericiesOut[m_Entries[n].BaseVertex + vertexIndex].x);
				}
				glEnd();
			}
		}

		// draw all children
		for (n = 0; n < nd->mNumChildren; ++n)
		{
			recursive_render(sc, nd->mChildren[n], 1);
		}

		glPopMatrix();
	}

	int DrawGLScene()				//Here's where we do all the drawing
	{
		if (m_startTime == -1)
		{
			m_startTime = GetCurrentTimeMillis();
		}
		float RunningTime = (float)((double)GetCurrentTimeMillis() - (double)m_startTime) / 1000.0f;
		BoneTransform(RunningTime, Transforms);
		m_VericiesOut.resize(m_Vericies.size());
		m_NormalesOut.resize(m_Vericies.size());
		aiMatrix4x4 m;
		for (uint i = 0; i < m_Vericies.size(); ++i)
		{
			m.a1 = m.a2 = m.a3 = m.a4 = 0.f;
			m.b1 = m.b2 = m.b3 = m.b4 = 0.f;
			m.c1 = m.c2 = m.c3 = m.c4 = 0.f;
			m.d1 = m.d2 = m.d3 = m.d4 = 0.f;
			for (int b = 0; b < NUM_BONES_PER_VEREX; ++b)
			{
				if (m_Mass[i].Weights[b] > 0) MulM4(m, Transforms[m_Mass[i].IDs[b]], m_Mass[i].Weights[b]);
			}

			m_VericiesOut[i] = m * m_Vericies[i];

			ShortMulM4(m_NormalesOut[i], m, m_Normales[i]);

		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
		glLoadIdentity();				// Reset MV Matrix


		glTranslatef(0.0f, -25.0f, -70.0f);	// Move 40 Units And Into The Screen	

		glRotatef(_rotation.x, 1.0f, 0.0f, 0.0f);
		glRotatef(_rotation.y, 0.0f, 1.0f, 0.0f);
		glRotatef(_rotation.z, 0.0f, 0.0f, 1.0f);

		logInfo("drawing objects");

		//glScalef(5.0f, 5.0f, 5.0f);	// Move 40 Units And Into The Screen	
		recursive_render(_scenes[0], _scenes[0]->mRootNode, 25);

		return TRUE;					// okay
	}

	int LoadGLTextures()
	{
		ILboolean success;

		/* Before calling ilInit() version should be checked. */
		if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION)
		{
			/// wrong DevIL version ///
			std::string err_msg = "Wrong DevIL version. Old devil.dll in system32/SysWow64?";
			char* cErr_msg = (char *) err_msg.c_str();
			abortGLInit(cErr_msg);
			return -1;
		}

		ilInit(); /* Initialization of DevIL */

		if (_scenes[_curScene]->HasTextures()) abortGLInit("Support for meshes with embedded textures is not implemented");

		/* getTexture Filenames and Numb of Textures */
		for (unsigned int m=0; m<_scenes[_curScene]->mNumMaterials; m++)
		{
			int texIndex = 0;
			aiReturn texFound = AI_SUCCESS;

			aiString path;	// filename

			while (texFound == AI_SUCCESS)
			{
				texFound = _scenes[_curScene]->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
				textureIdMap[path.data] = NULL; //fill map with textures, pointers still NULL yet
				texIndex++;
			}
		}

		int numTextures = textureIdMap.size();

		/* array with DevIL image IDs */
		ILuint* imageIds = NULL;
		imageIds = new ILuint[numTextures];

		/* generate DevIL Image IDs */
		ilGenImages(numTextures, imageIds); /* Generation of numTextures image names */

		/* create and fill array with GL texture ids */
		textureIds = new GLuint[numTextures];
		glGenTextures(numTextures, textureIds); /* Texture name generation */

		/* get iterator */
		std::map<std::string, GLuint*>::iterator itr = textureIdMap.begin();

		std::string basepath = getBasePath(m_ModelPath);
		for (int i=0; i<numTextures; i++)
		{

			//save IL image ID
			std::string filename = (*itr).first;  // get filename
			(*itr).second =  &textureIds[i];	  // save texture id for filename in map
			itr++;								  // next texture


			ilBindImage(imageIds[i]); /* Binding of DevIL image name */
			std::string fileloc = basepath + filename;	/* Loading of image */
			success = ilLoadImage(fileloc.c_str());
			if (!success)
			{
				std::string::size_type i = filename.find('\\');
				while (i != std::string::npos)
				{
					filename.erase(0, i + 1);
					i = filename.find('\\');
				}
				fileloc = basepath + filename;
				success = ilLoadImage(fileloc.c_str());
			}

			if (success) /* If no error occured: */
			{
				// Convert every colour component into unsigned byte.If your image contains 
				// alpha channel you can replace IL_RGB with IL_RGBA
				success = ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
				if (!success)
				{
					/* Error occured */
					abortGLInit("Couldn't convert image");
					return -1;
				}
				// Binding of texture name
				glBindTexture(GL_TEXTURE_2D, textureIds[i]); 
				// redefine standard texture values
				// We will use linear interpolation for magnification filter
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
				// We will use linear interpolation for minifying filter
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				// Texture specification
				glTexImage2D(GL_TEXTURE_2D, 0, ilGetInteger(IL_IMAGE_BPP), ilGetInteger(IL_IMAGE_WIDTH),
					ilGetInteger(IL_IMAGE_HEIGHT), 0, ilGetInteger(IL_IMAGE_FORMAT), GL_UNSIGNED_BYTE,
					ilGetData()); 
				// we also want to be able to deal with odd texture dimensions
				glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
				glPixelStorei( GL_UNPACK_ROW_LENGTH, 0 );
				glPixelStorei( GL_UNPACK_SKIP_PIXELS, 0 );
				glPixelStorei( GL_UNPACK_SKIP_ROWS, 0 );
			}
			else
			{
				/* Error occured */
				MessageBoxA(NULL, ("Couldn't load Image: " + fileloc).c_str() , "ERROR", MB_OK | MB_ICONEXCLAMATION);
			}
		}
		// Because we have already copied image data into texture data  we can release memory used by image.
		ilDeleteImages(numTextures, imageIds); 

		// Cleanup
		delete [] imageIds;
		imageIds = NULL;

		return TRUE;
	}

	bool Add3DAnimFromFile(const std::string &fileName)
	{
		// Check if file exists
		std::ifstream fin(fileName.c_str());
		if(!fin.fail())
		{
			fin.close();
		}
		else
		{
			MessageBoxA(NULL, ("Couldn't open file: " + fileName).c_str() , "ERROR", MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		_curScene = _importers.size();
		_importers.push_back(new Assimp::Importer);
		_scenes.push_back(_importers[_curScene]->ReadFile( fileName, aiProcessPreset_TargetRealtime_Quality ));
		// If the import failed, report it
		if (!_scenes[_curScene]) { 
			logInfo( _importers[_curScene]->GetErrorString());
			return false;
		}
		return true;
	}

	void SetAnimIndex(uint index)
	{
		_curScene = index;
	}

};
