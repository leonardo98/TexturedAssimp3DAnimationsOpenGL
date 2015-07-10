#include "MathFunc.h"

void set_float4(float f[4], float a, float b, float c, float d)
{
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

void color4_to_float4(const aiColor4D *c, float f[4])
{
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

// Can't send color down as a pointer to aiColor4D because AI colors are ABGR.
void Color4f(const aiColor4D *color)
{
	glColor4f(color->r, color->g, color->b, color->a);
}

uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumRotationKeys > 0);

	for (uint i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
		if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) {
			return i;
		}
	}

	assert(0);
	return 0xFFFFFFFF;
} 

void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumRotationKeys == 1) {
		Out = pNodeAnim->mRotationKeys[0].mValue;
		return;
	}

	uint RotationIndex = FindRotation(AnimationTime, pNodeAnim);
	uint NextRotationIndex = (RotationIndex + 1);
	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
	float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
	const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
	aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
	Out = Out.Normalize();
} 

uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumScalingKeys > 0);

	for (uint i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
		if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}

	assert(0);
	return 0xFFFFFFFF;
} 

void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumScalingKeys == 1) {
		Out = pNodeAnim->mScalingKeys[0].mValue;
		return;
	}

	uint ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
	uint NextScalingIndex = (ScalingIndex + 1);
	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
	float DeltaTime = pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& StartScaling = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
	const aiVector3D& EndScaling = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
	Out = StartScaling * (1 - Factor) + EndScaling * Factor;
} 

uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	assert(pNodeAnim->mNumPositionKeys > 0);

	for (uint i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
		if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}

	assert(0);
	return 0xFFFFFFFF;
} 

void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim)
{
	// we need at least two values to interpolate...
	if (pNodeAnim->mNumPositionKeys == 1) {
		Out = pNodeAnim->mPositionKeys[0].mValue;
		return;
	}

	uint PositionIndex = FindPosition(AnimationTime, pNodeAnim);
	uint NextPositionIndex = (PositionIndex + 1);
	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
	float DeltaTime = pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime;
	float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	assert(Factor >= 0.0f && Factor <= 1.0f);
	const aiVector3D& StartPosition = pNodeAnim->mPositionKeys[PositionIndex].mValue;
	const aiVector3D& EndPosition = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
	Out = StartPosition * (1 - Factor) + EndPosition * Factor;
} 

const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string &nodeName)
{
	for (uint i = 0; i < pAnimation->mNumChannels; ++i)
	{
		if (strcmp(pAnimation->mChannels[i]->mNodeName.C_Str(), nodeName.c_str()) == 0)
		{
			return pAnimation->mChannels[i];
		}
	}
	return NULL;
}

void InitM4FromM3(aiMatrix4x4& out, const aiMatrix3x3& in)
{
	out.a1 = in.a1; out.a2 = in.a2; out.a3 = in.a3; out.a4 = 0.f;
	out.b1 = in.b1; out.b2 = in.b2; out.b3 = in.b3; out.b4 = 0.f;
	out.c1 = in.c1; out.c2 = in.c2; out.c3 = in.c3; out.c4 = 0.f;
	out.d1 = 0.f;   out.d2 = 0.f;   out.d3 = 0.f;   out.d4 = 1.f;
}


void InitIdentity(aiMatrix4x4 &m)
{
	m.a1 = 1.f; m.a2 = 0.f; m.a3 = 0.f; m.a4 = 0.f;
	m.b1 = 0.f; m.b2 = 1.f; m.b3 = 0.f; m.b4 = 0.f;
	m.c1 = 0.f; m.c2 = 0.f; m.c3 = 1.f; m.c4 = 0.f;
	m.d1 = 0.f; m.d2 = 0.f; m.d3 = 0.f; m.d4 = 1.f;
	assert(m.IsIdentity());
}

void Mul(aiMatrix4x4 &out, aiMatrix4x4 &in, float m)
{
	out.a1 += in.a1 * m; out.a2 += in.a2 * m; out.a3 += in.a3 * m; out.a4 += in.a4 * m;
	out.b1 += in.b1 * m; out.b2 += in.b2 * m; out.b3 += in.b3 * m; out.b4 += in.b4 * m;
	out.c1 += in.c1 * m; out.c2 += in.c2 * m; out.c3 += in.c3 * m; out.c4 += in.c4 * m;
	out.d1 += in.d1 * m; out.d2 += in.d2 * m; out.d3 += in.d3 * m; out.d4 += in.d4 * m;
}

void ShortMul(aiVector3D &out, const aiMatrix4x4 &m, const aiVector3D &in)
{
	out.x = m.a1 * in.x + m.a2 * in.y + m.a3 * in.z;
	out.y = m.b1 * in.x + m.b2 * in.y + m.b3 * in.z;
	out.z = m.c1 * in.x + m.c2 * in.y + m.c3 * in.z;
}

long long GetCurrentTimeMillis()
{
#ifdef WIN32    
	return GetTickCount();
#else
	timeval t;
	gettimeofday(&t, NULL);

	long long ret = t.tv_sec * 1000 + t.tv_usec / 1000;
	return ret;
#endif    
}

std::string getBasePath(const std::string& path)
{
	size_t pos = path.find_last_of("\\/");
	return (std::string::npos == pos) ? "" : path.substr(0, pos + 1);
}

GLboolean abortGLInit(const char* abortMessage)
{
	MessageBoxA(NULL, abortMessage, "ERROR", MB_OK|MB_ICONEXCLAMATION);
	exit(-61);									// quit and return False
}

void logInfo(const std::string &logString)
{
	// Will add message to File with "info" Tag
	Assimp::DefaultLogger::get()->info(logString.c_str());
}


void createAILogger()
{
    // Change this line to normal if you not want to analyse the import process
	//Assimp::Logger::LogSeverity severity = Assimp::Logger::NORMAL;
	Assimp::Logger::LogSeverity severity = Assimp::Logger::VERBOSE;

	// Create a logger instance for Console Output
	Assimp::DefaultLogger::create("",severity, aiDefaultLogStream_STDOUT);

	// Create a logger instance for File Output (found in project folder or near .exe)
	Assimp::DefaultLogger::create("assimp_log.txt",severity, aiDefaultLogStream_FILE);

	// Now I am ready for logging my stuff
	Assimp::DefaultLogger::get()->info("this is my info-call");
}

void destroyAILogger()
{
	// Kill it after the work is done
	Assimp::DefaultLogger::kill();
}

void logDebug(const char* logString)
{
	// Will add message to File with "debug" Tag
	Assimp::DefaultLogger::get()->debug(logString);
}
