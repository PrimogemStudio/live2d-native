module;
#include <fstream>
#include <vector>
#include <Framework/CubismFramework.hpp>
#include <jni.h>
export module Util;

using namespace std;
using namespace Csm;

export
{
	vector<char> LoadFile(const char* path)
	{
		ifstream in(path, ios::binary);
		in.seekg(0, ios::end);
		auto size = (int)in.tellg();
		in.seekg(0, ios::beg);
		vector<char> buff(size);
		in.read(buff.data(), size);
		return buff;
	}

	namespace Constants
	{
		constexpr csmFloat32 ViewScale = 1.0f;
		constexpr csmFloat32 ViewMaxScale = 2.0f;
		constexpr csmFloat32 ViewMinScale = 0.8f;
		constexpr csmFloat32 ViewLogicalLeft = -1.0f;
		constexpr csmFloat32 ViewLogicalRight = 1.0f;
		constexpr csmFloat32 ViewLogicalBottom = -1.0f;
		constexpr csmFloat32 ViewLogicalTop = -1.0f;
		constexpr csmFloat32 ViewLogicalMaxLeft = -2.0f;
		constexpr csmFloat32 ViewLogicalMaxRight = 2.0f;
		constexpr csmFloat32 ViewLogicalMaxBottom = -2.0f;
		constexpr csmFloat32 ViewLogicalMaxTop = 2.0f;
		const csmChar* MotionGroupIdle = "Idle";
		const csmChar* MotionGroupTapBody = "TapBody";
		const csmChar* HitAreaNameHead = "Head";
		const csmChar* HitAreaNameBody = "Body";
		constexpr csmInt32 PriorityNone = 0;
		constexpr csmInt32 PriorityIdle = 1;
		constexpr csmInt32 PriorityNormal = 2;
		constexpr csmInt32 PriorityForce = 3;
		constexpr csmBool MocConsistencyValidationEnable = true;
		constexpr csmBool DebugLogEnable = true;
		constexpr csmBool DebugTouchLogEnable = false;
		constexpr CubismFramework::Option::LogLevel CubismLoggingLevel = CubismFramework::Option::LogLevel_Verbose;
		constexpr csmInt32 RenderTargetWidth = 1900;
		constexpr csmInt32 RenderTargetHeight = 1000;
	}

	inline void Throw(JNIEnv* env, const char* msg)
	{
		env->ThrowNew(env->FindClass("com/sun/jdi/NativeMethodException"), msg);
	}

	template <typename F> requires std::is_function_v<std::remove_pointer_t<F>>
	consteval JNINativeMethod JNIMethod(const char* name, const char* sig, F ptr)
	{
		return { const_cast<char*>(name),const_cast<char*>(sig), (void*)ptr };
	}
}
