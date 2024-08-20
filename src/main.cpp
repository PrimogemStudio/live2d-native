#include <jni.h>
#include <jnipp.h>
#include <Framework/CubismFramework.hpp>

import Live2D;

using namespace Live2D::Cubism::Framework;
using namespace L2D;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
	static Allocator allocator;
	static CubismFramework::Option option;
	CubismFramework::StartUp(&allocator, &option);
	CubismFramework::Initialize();
	jni::init(vm);
	JNIEnv* env;
	if (vm->GetEnv((void**)&env, JNI_VERSION_21) != JNI_OK) return JNI_EVERSION;
	if (RegisterMethods(env) != JNI_OK) return JNI_ERR;
	return JNI_VERSION_21;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved)
{
}