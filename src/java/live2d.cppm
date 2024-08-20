module;
#include <Framework/CubismFramework.hpp>
#include <Framework/ICubismAllocator.hpp>
#include <Framework/CubismModelSettingJson.hpp>
#include <Framework/Model/CubismUserModel.hpp>
#include <string>
#include <functional>
#include <glad/gl.h>
#include <functional>
#include <jni.h>
export module Live2D;

using namespace Csm;
using namespace std;
using namespace Live2D::Cubism::Framework;

export namespace L2D
{
	class Timer final
	{
		static double currentFrame;
		static double lastFrame;
		static double deltaTime;
	public:
		static float GetDeltaTime() { return (float)deltaTime; }
		static void UpdateTime(const double currentTime)
		{
			currentFrame = currentTime;
			deltaTime = currentFrame - lastFrame;
			lastFrame = currentFrame;
		}
		Timer() = delete;
		~Timer() = delete;
	};

	class Allocator : public ICubismAllocator
	{
		void* Allocate(csmSizeType size) override;
		void Deallocate(void* memory) override;
		void* AllocateAligned(csmSizeType size, csmUint32 alignment) override;
		void DeallocateAligned(void* alignedMemory) override;
	};

	class TextureManager final
	{
	public:
		struct TextureInfo
		{
			GLuint id;
			int width;
			int height;
			std::string fileName;
		};
		csmVector<TextureInfo*> textures;

		TextureManager();
		~TextureManager();
		TextureInfo* CreateTextureFromPngFile(const std::string& fileName);
		void ReleaseTextures();
		void ReleaseTexture(csmUint32 textureId);
		void ReleaseTexture(const std::string& fileName);
		TextureInfo* GetTextureInfoById(GLuint textureId) const;
	};

	class Live2DModel final : public CubismUserModel
	{
		std::string ModelName;
		std::string ModelDir;
		csmFloat32 UserTimeSeconds;
		CubismModelSettingJson* ModelJson;
		csmVector<CubismIdHandle> EyeBlinkIds;
		csmMap<csmString, ACubismMotion*> Motions;
		csmMap<csmString, ACubismMotion*> Expressions;
		TextureManager TextureManager;
		const CubismId* AngleX;
		const CubismId* AngleY;
		const CubismId* AngleZ;
		const CubismId* BodyAngleX;
		const CubismId* EyeBallX;
		const CubismId* EyeBallY;

		Live2DModel(const std::string& name, const std::string& dir);
		~Live2DModel() override;
		std::string MakeAssetPath(const std::string& file);
		void SetAssetDirectory(const std::string& path);
		void LoadAsset(const std::string& file, const std::function<void(csmByte*, csmSizeInt)>& callback);
		CubismMotionQueueEntryHandle StartMotion(const csmChar* group, csmInt32 no, csmInt32 priority, ACubismMotion::FinishedMotionCallback onFinishedMotionHandler = nullptr);
		void ReleaseModelSetting();
		void Draw(CubismMatrix44& matrix);
		void ModelParamUpdate();
		void SetupTextures();
		void PreloadMotionGroup(const csmChar* group);
		void SetupModel();
		void ModelOnUpdate(int width, int height, double currentTime);
		static void Load(JNIEnv* env, jobject self, jobject name, jobject path);
		static void Update(JNIEnv* env, jobject self, jint width, jint height);
		static void Release(JNIEnv* env, jclass cls, jlong ptr);
	public:
		static int RegisterMethods(JNIEnv* env);
	};

	int RegisterMethods(JNIEnv* env);
}

namespace L2D
{
	double Timer::currentFrame;
	double Timer::lastFrame;
	double Timer::deltaTime;

	void Init(JNIEnv* env, jclass cls, jlong, jlong);
}
