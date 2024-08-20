module;
#include <Framework/CubismFramework.hpp>
#include <Framework/Model/CubismUserModel.hpp>
#include <Framework/Id/CubismIdManager.hpp>
#include <Framework/CubismDefaultParameterId.hpp>
#include <Framework/Motion/CubismMotion.hpp>
#include <Framework/Utils/CubismString.hpp>
#include <Framework/CubismModelSettingJson.hpp>
#include <Framework/Rendering/OpenGL/CubismRenderer_OpenGLCore.hpp>
#include <functional>
#include <jni.h>
#include <jnipp.h>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
module Live2D;

import Util;

using namespace jni;
using namespace Live2D::Cubism::Framework;
using namespace L2D;

static double (*GetTime)();

void L2D::Init(JNIEnv*, jclass, jlong handle, jlong fun)
{
	gladLoadGL((GLADloadfunc)handle);
	GetTime = (decltype(GetTime))fun;
}

int L2D::RegisterMethods(JNIEnv* env)
{
	const auto native = env->FindClass("com/primogemstudio/advancedfmk/live2d/Live2DNative");
	JNINativeMethod methods[1];
	methods[0] = JNIMethod("init", "(JJ)V", Init);
	return Live2DModel::RegisterMethods(env) + env->RegisterNatives(native, methods, std::size(methods));
}

int Live2DModel::RegisterMethods(JNIEnv* env)
{
	const auto native = env->FindClass("com/primogemstudio/advancedfmk/live2d/Live2DModel");
	JNINativeMethod methods[3];
	methods[0] = JNIMethod("load", "(Ljava/lang/String;Ljava/lang/String;)V", Load);
	methods[1] = JNIMethod("update", "(II)V", Update);
	methods[2] = JNIMethod("release", "(J)V", Release);
	return env->RegisterNatives(native, methods, std::size(methods));
}

static Live2DModel* Get(const jobject self)
{
	return (Live2DModel*)Object(self).get<jlong>("ptr");
}

void Live2DModel::Update(JNIEnv*, jobject self, jint width, jint height)
{
	Get(self)->ModelOnUpdate(width, height, GetTime());
}

void Live2DModel::Load(JNIEnv*, jobject self, jobject name, jobject path)
{
	Object self_obj(self), name_obj(name), path_obj(path);
	const auto model = new Live2DModel(name_obj.call<std::string>("toString"), path_obj.call<std::string>("toString"));
	model->SetupModel();
	self_obj.set("ptr", (jlong)model);
}

void Live2DModel::Release(JNIEnv*, jclass, jlong ptr)
{
	delete (Live2DModel*)ptr;
}

Live2DModel::Live2DModel(const std::string& name, const std::string& dir) : ModelName(name), ModelDir(dir), UserTimeSeconds(0.0f), ModelJson(nullptr)
{
	AngleX = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamAngleX);
	AngleY = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamAngleY);
	AngleZ = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamAngleZ);
	BodyAngleX = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamBodyAngleX);
	EyeBallX = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamEyeBallX);
	EyeBallY = CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamEyeBallY);
}

Live2DModel::~Live2DModel()
{
	ReleaseModelSetting();
}

std::string Live2DModel::MakeAssetPath(const std::string& file)
{
	return ModelDir + file;
}

void Live2DModel::SetAssetDirectory(const std::string& path)
{
	ModelDir = path;
}

void Live2DModel::LoadAsset(const std::string& file, const std::function<void(csmByte*, csmSizeInt)>& callback)
{
	if (file.empty()) return;
	auto buff = LoadFile(MakeAssetPath(file).c_str());
	callback((uint8_t*)buff.data(), buff.size());
}

void Live2DModel::SetupModel()
{
	_updating = true;
	_initialized = false;
	LoadAsset(ModelName + ".model3.json", [this](auto buff, auto size) { ModelJson = new CubismModelSettingJson(buff, size); });
	LoadAsset(ModelJson->GetModelFileName(), [this](auto buff, auto size) { LoadModel(buff, size); });
	for (auto expressionIndex = 0; expressionIndex < ModelJson->GetExpressionCount(); ++expressionIndex)
	{
		LoadAsset(ModelJson->GetExpressionFileName(expressionIndex), [this, expressionIndex](csmByte* buff, csmSizeInt size) {
			auto expressionName = ModelJson->GetExpressionName(expressionIndex);
			if (auto motion = LoadExpression(buff, size, expressionName))
			{
				if (Expressions[expressionName])
				{
					ACubismMotion::Delete(Expressions[expressionName]);
					Expressions[expressionName] = nullptr;
				}
				Expressions[expressionName] = motion;
			}
			});
	}
	LoadAsset(ModelJson->GetPoseFileName(), [this](auto buff, auto size) { LoadPose(buff, size); });
	LoadAsset(ModelJson->GetPhysicsFileName(), [this](auto buff, auto size) { LoadPhysics(buff, size); });
	LoadAsset(ModelJson->GetUserDataFile(), [this](auto buff, auto size) { LoadUserData(buff, size); });
	csmMap<csmString, csmFloat32> layout;
	ModelJson->GetLayoutMap(layout);
	_modelMatrix->SetupFromLayout(layout);
	_model->SaveParameters();
	for (csmInt32 i = 0; i < ModelJson->GetMotionGroupCount(); i++)
	{
		const csmChar* group = ModelJson->GetMotionGroupName(i);
		PreloadMotionGroup(group);
	}
	_motionManager->StopAllMotions();
	_breath = CubismBreath::Create();
	csmVector<CubismBreath::BreathParameterData> breathParameters;
	breathParameters.PushBack(CubismBreath::BreathParameterData(AngleX, 0.0f, 15.0f, 6.5345f, 0.5f));
	breathParameters.PushBack(CubismBreath::BreathParameterData(AngleY, 0.0f, 8.0f, 3.5345f, 0.5f));
	breathParameters.PushBack(CubismBreath::BreathParameterData(AngleZ, 0.0f, 10.0f, 5.5345f, 0.5f));
	breathParameters.PushBack(CubismBreath::BreathParameterData(BodyAngleX, 0.0f, 4.0f, 15.5345f, 0.5f));
	breathParameters.PushBack(CubismBreath::BreathParameterData(CubismFramework::GetIdManager()->GetId(DefaultParameterId::ParamBreath), 0.5f, 0.5f, 3.2345f, 0.5f));
	_breath->SetParameters(breathParameters);
	CreateRenderer();
	SetupTextures();
	_updating = false;
	_initialized = true;
}

void Live2DModel::PreloadMotionGroup(const csmChar* group)
{
	const csmInt32 count = ModelJson->GetMotionCount(group);
	for (csmInt32 i = 0; i < count; i++)
	{
		csmString name = Utils::CubismString::GetFormatedString("%s_%d", group, i);
		csmString path = ModelJson->GetMotionFileName(group, i);
		path = csmString(ModelDir.c_str()) + path;
		auto buff = LoadFile(path.GetRawString());
		if (auto tmpMotion = static_cast<CubismMotion*>(LoadMotion((uint8_t*)buff.data(), buff.size(), name.GetRawString())))
		{
			csmFloat32 fadeTime = ModelJson->GetMotionFadeInTimeValue(group, i);
			if (fadeTime >= 0.0f) tmpMotion->SetFadeInTime(fadeTime);
			fadeTime = ModelJson->GetMotionFadeOutTimeValue(group, i);
			if (fadeTime >= 0.0f) tmpMotion->SetFadeOutTime(fadeTime);
			if (Motions[name]) ACubismMotion::Delete(Motions[name]);
			Motions[name] = tmpMotion;
		}
	}
}

void Live2DModel::ReleaseModelSetting()
{
	for (auto it = Motions.Begin(); it != Motions.End(); ++it) ACubismMotion::Delete(it->Second);
	Motions.Clear();
	for (auto it = Expressions.Begin(); it != Expressions.End(); ++it) ACubismMotion::Delete(it->Second);
	Expressions.Clear();
	delete ModelJson;
}

CubismMotionQueueEntryHandle Live2DModel::StartMotion(const csmChar* group, csmInt32 no, csmInt32 priority, ACubismMotion::FinishedMotionCallback onFinishedMotionHandler)
{
	if (!ModelJson->GetMotionCount(group)) return InvalidMotionQueueEntryHandleValue;
	if (priority == Constants::PriorityForce) _motionManager->SetReservePriority(priority);
	else if (!_motionManager->ReserveMotion(priority)) return InvalidMotionQueueEntryHandleValue;
	auto fileName = ModelJson->GetMotionFileName(group, no);
	auto name = Utils::CubismString::GetFormatedString("%s_%d", group, no);
	auto motion = static_cast<CubismMotion*>(Motions[name.GetRawString()]);
	auto autoDelete = false;
	if (!motion)
	{
		csmString path = fileName;
		path = csmString(ModelDir.c_str()) + path;
		auto buff = LoadFile(path.GetRawString());
		motion = static_cast<CubismMotion*>(LoadMotion((uint8_t*)buff.data(), buff.size(), nullptr, onFinishedMotionHandler));
		if (motion)
		{
			csmFloat32 fadeTime = ModelJson->GetMotionFadeInTimeValue(group, no);
			if (fadeTime >= 0.0f) motion->SetFadeInTime(fadeTime);
			fadeTime = ModelJson->GetMotionFadeOutTimeValue(group, no);
			if (fadeTime >= 0.0f) motion->SetFadeOutTime(fadeTime);
			autoDelete = true;
		}
	}
	else motion->SetFinishedMotionHandler(onFinishedMotionHandler);
	return _motionManager->StartMotionPriority(motion, autoDelete, priority);
}

void Live2DModel::SetupTextures()
{
	for (csmInt32 modelTextureNumber = 0; modelTextureNumber < ModelJson->GetTextureCount(); modelTextureNumber++)
	{
		if (!strcmp(ModelJson->GetTextureFileName(modelTextureNumber), "")) continue;
		csmString texturePath = ModelJson->GetTextureFileName(modelTextureNumber);
		texturePath = csmString(ModelDir.c_str()) + texturePath;
		auto texture = TextureManager.CreateTextureFromPngFile(texturePath.GetRawString());
		GetRenderer<Rendering::CubismRenderer_OpenGLCore>()->BindTexture(modelTextureNumber, texture->id);
	}
	GetRenderer<Rendering::CubismRenderer_OpenGLCore>()->IsPremultipliedAlpha(false);
}

void Live2DModel::ModelParamUpdate()
{
	auto deltaTimeSeconds = Timer::GetDeltaTime();
	UserTimeSeconds += deltaTimeSeconds;
	_dragManager->Update(deltaTimeSeconds);
	_dragX = _dragManager->GetX();
	_dragY = _dragManager->GetY();
	csmBool motionUpdated = false;
	_model->LoadParameters();
	if (_motionManager->IsFinished()) StartMotion(Constants::MotionGroupIdle, 0, Constants::PriorityIdle);
	else motionUpdated = _motionManager->UpdateMotion(_model, deltaTimeSeconds);
	_model->SaveParameters();
	if (!motionUpdated && _eyeBlink) _eyeBlink->UpdateParameters(_model, deltaTimeSeconds);
	if (_expressionManager) _expressionManager->UpdateMotion(_model, deltaTimeSeconds);
	_model->AddParameterValue(AngleX, _dragX * 30.0f);
	_model->AddParameterValue(AngleY, _dragY * 30.0f);
	_model->AddParameterValue(AngleZ, _dragX * _dragY * -30.0f);
	_model->AddParameterValue(BodyAngleX, _dragX * 10.0f);
	_model->AddParameterValue(EyeBallX, _dragX);
	_model->AddParameterValue(EyeBallY, _dragY);
	if (_breath) _breath->UpdateParameters(_model, deltaTimeSeconds);
	if (_physics) _physics->Evaluate(_model, deltaTimeSeconds);
	if (_pose) _pose->UpdateParameters(_model, deltaTimeSeconds);
	_model->Update();
}

void Live2DModel::Draw(CubismMatrix44& matrix)
{
	if (!_model) return;
	matrix.MultiplyByMatrix(_modelMatrix);
	GetRenderer<Rendering::CubismRenderer_OpenGLCore>()->SetMvpMatrix(&matrix);
	GetRenderer<Rendering::CubismRenderer_OpenGLCore>()->DrawModel();
}

void Live2DModel::ModelOnUpdate(int width, int height, double currentTime)
{
	Timer::UpdateTime(currentTime);
	CubismMatrix44 projection;
	projection.LoadIdentity();
	if (_model->GetCanvasWidth() > 1.0f && width < height)
	{
		GetModelMatrix()->SetWidth(2.0f);
		projection.Scale(1.0f, static_cast<float>(width) / static_cast<float>(height));
	}
	else projection.Scale(static_cast<float>(height) / static_cast<float>(width), 1.0f);
	ModelParamUpdate();
	Draw(projection);
}