/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#include "Framework/Rendering/OpenGL/CubismRenderer_OpenGLCore.hpp"
#include "Framework/Math/CubismMatrix44.hpp"
#include "Framework/Type/csmVector.hpp"
#include "Framework/Model/CubismModel.hpp"
#include <cfloat>

#ifdef CSM_TARGET_WIN_GL
#include <Windows.h>
#endif

#define CSM_FRAGMENT_SHADER_FP_PRECISION_HIGH "highp"
#define CSM_FRAGMENT_SHADER_FP_PRECISION_MID "mediump"
#define CSM_FRAGMENT_SHADER_FP_PRECISION_LOW "lowp"

#define CSM_FRAGMENT_SHADER_FP_PRECISION CSM_FRAGMENT_SHADER_FP_PRECISION_HIGH

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

/*********************************************************************************************************************
*                                      CubismClippingManager_OpenGLCore
********************************************************************************************************************/
void CubismClippingManager_OpenGLCore::SetupClippingContext(CubismModel& model, CubismRenderer_OpenGLCore* renderer, GLint lastFBO, GLint lastViewport[4])
{
    // 全てのクリッピングを用意する
    // 同じクリップ（複数の場合はまとめて１つのクリップ）を使う場合は１度だけ設定する
    csmInt32 usingClipCount = 0;
    for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
    {
        // １つのクリッピングマスクに関して
        CubismClippingContext_OpenGLCore* cc = _clippingContextListForMask[clipIndex];

        // このクリップを利用する描画オブジェクト群全体を囲む矩形を計算
        CalcClippedDrawTotalBounds(model, cc);

        if (cc->_isUsing)
        {
            usingClipCount++; //使用中としてカウント
        }
    }

    if (usingClipCount <= 0)
    {
        return;
    }

    // マスク作成処理
    // 生成したOffscreenSurfaceと同じサイズでビューポートを設定
    glViewport(0, 0, _clippingMaskBufferSize.X, _clippingMaskBufferSize.Y);

    // 後の計算のためにインデックスの最初をセット
    _currentMaskBuffer = renderer->GetMaskBuffer(0);
    // ----- マスク描画処理 -----
    _currentMaskBuffer->BeginDraw(lastFBO);

    renderer->PreDraw(); // バッファをクリアする

    // 各マスクのレイアウトを決定していく
    SetupLayoutBounds(usingClipCount);

    // サイズがレンダーテクスチャの枚数と合わない場合は合わせる
    if (_clearedMaskBufferFlags.GetSize() != _renderTextureCount)
    {
        _clearedMaskBufferFlags.Clear();

        for (csmInt32 i = 0; i < _renderTextureCount; ++i)
        {
            _clearedMaskBufferFlags.PushBack(false);
        }
    }
    else
    {
        // マスクのクリアフラグを毎フレーム開始時に初期化
        for (csmInt32 i = 0; i < _renderTextureCount; ++i)
        {
            _clearedMaskBufferFlags[i] = false;
        }
    }

    // 実際にマスクを生成する
    // 全てのマスクをどの様にレイアウトして描くかを決定し、ClipContext , ClippedDrawContext に記憶する
    for (csmUint32 clipIndex = 0; clipIndex < _clippingContextListForMask.GetSize(); clipIndex++)
    {
        // --- 実際に１つのマスクを描く ---
        CubismClippingContext_OpenGLCore* clipContext = _clippingContextListForMask[clipIndex];
        csmRectF* allClippedDrawRect = clipContext->_allClippedDrawRect; //このマスクを使う、全ての描画オブジェクトの論理座標上の囲み矩形
        csmRectF* layoutBoundsOnTex01 = clipContext->_layoutBounds; //この中にマスクを収める
        const csmFloat32 MARGIN = 0.05f;

        // clipContextに設定したオフスクリーンサーフェイスをインデックスで取得
        CubismOffscreenSurface_OpenGLCore* clipContextOffscreenSurface = renderer->GetMaskBuffer(clipContext->_bufferIndex);

        // 現在のオフスクリーンサーフェイスがclipContextのものと異なる場合
        if (_currentMaskBuffer != clipContextOffscreenSurface)
        {
            _currentMaskBuffer->EndDraw();
            _currentMaskBuffer = clipContextOffscreenSurface;
            // マスク用RenderTextureをactiveにセット
            _currentMaskBuffer->BeginDraw(lastFBO);

            // バッファをクリアする。
            renderer->PreDraw();
        }

        // モデル座標上の矩形を、適宜マージンを付けて使う
        _tmpBoundsOnModel.SetRect(allClippedDrawRect);
        _tmpBoundsOnModel.Expand(allClippedDrawRect->Width * MARGIN, allClippedDrawRect->Height * MARGIN);
        //########## 本来は割り当てられた領域の全体を使わず必要最低限のサイズがよい
        // シェーダ用の計算式を求める。回転を考慮しない場合は以下のとおり
        // movePeriod' = movePeriod * scaleX + offX     [[ movePeriod' = (movePeriod - tmpBoundsOnModel.movePeriod)*scale + layoutBoundsOnTex01.movePeriod ]]
        csmFloat32 scaleX = layoutBoundsOnTex01->Width / _tmpBoundsOnModel.Width;
        csmFloat32 scaleY = layoutBoundsOnTex01->Height / _tmpBoundsOnModel.Height;

        // マスク生成時に使う行列を求める
        createMatrixForMask(false, layoutBoundsOnTex01, scaleX, scaleY);

        clipContext->_matrixForMask.SetMatrix(_tmpMatrixForMask.GetArray());
        clipContext->_matrixForDraw.SetMatrix(_tmpMatrixForDraw.GetArray());

        // 実際の描画を行う
        const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
        for (csmInt32 i = 0; i < clipDrawCount; i++)
        {
            const csmInt32 clipDrawIndex = clipContext->_clippingIdList[i];

            // 頂点情報が更新されておらず、信頼性がない場合は描画をパスする
            if (!model.GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
            {
                continue;
            }

            renderer->IsCulling(model.GetDrawableCulling(clipDrawIndex) != 0);

            // マスクがクリアされていないなら処理する
            if (!_clearedMaskBufferFlags[clipContext->_bufferIndex])
            {
                // マスクをクリアする
                // 1が無効（描かれない）領域、0が有効（描かれる）領域。（シェーダーCd*Csで0に近い値をかけてマスクを作る。1をかけると何も起こらない）
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
                _clearedMaskBufferFlags[clipContext->_bufferIndex] = true;
            }

            // 今回専用の変換を適用して描く
            // チャンネルも切り替える必要がある(A,R,G,B)
            renderer->SetClippingContextBufferForMask(clipContext);

            renderer->DrawMeshOpenGL(model, clipDrawIndex);
        }
    }

    // --- 後処理 ---
    _currentMaskBuffer->EndDraw();
    renderer->SetClippingContextBufferForMask(NULL);
    glViewport(lastViewport[0], lastViewport[1], lastViewport[2], lastViewport[3]);
}

/*********************************************************************************************************************
*                                      CubismClippingContext_OpenGLCore
********************************************************************************************************************/
CubismClippingContext_OpenGLCore::CubismClippingContext_OpenGLCore(CubismClippingManager<CubismClippingContext_OpenGLCore, CubismOffscreenSurface_OpenGLCore>* manager, CubismModel& model, const csmInt32* clippingDrawableIndices, csmInt32 clipCount)
    : CubismClippingContext(clippingDrawableIndices, clipCount)
{
    _owner = manager;
}

CubismClippingContext_OpenGLCore::~CubismClippingContext_OpenGLCore()
{
}

CubismClippingManager<CubismClippingContext_OpenGLCore, CubismOffscreenSurface_OpenGLCore>* CubismClippingContext_OpenGLCore::GetClippingManager()
{
    return _owner;
}

/*********************************************************************************************************************
*                                      CubismDrawProfile_OpenGL
********************************************************************************************************************/
void CubismRendererProfile_OpenGLCore::SetGlEnable(GLenum index, GLboolean enabled)
{
    if (enabled == GL_TRUE) glEnable(index);
    else glDisable(index);
}

void CubismRendererProfile_OpenGLCore::Save()
{
    //-- push state --
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &_lastArrayBufferBinding);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &_lastElementArrayBufferBinding);
    glGetIntegerv(GL_CURRENT_PROGRAM, &_lastProgram);

    glGetIntegerv(GL_ACTIVE_TEXTURE, &_lastActiveTexture);
    glActiveTexture(GL_TEXTURE1); //テクスチャユニット1をアクティブに（以後の設定対象とする）
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &_lastTexture1Binding2D);

    glActiveTexture(GL_TEXTURE0); //テクスチャユニット0をアクティブに（以後の設定対象とする）
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &_lastTexture0Binding2D);

    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &_lastVAO);

    _lastScissorTest = glIsEnabled(GL_SCISSOR_TEST);
    _lastStencilTest = glIsEnabled(GL_STENCIL_TEST);
    _lastDepthTest = glIsEnabled(GL_DEPTH_TEST);
    _lastCullFace = glIsEnabled(GL_CULL_FACE);
    _lastBlend = glIsEnabled(GL_BLEND);

    glGetIntegerv(GL_FRONT_FACE, &_lastFrontFace);

    glGetBooleanv(GL_COLOR_WRITEMASK, _lastColorMask);

    // backup blending
    glGetIntegerv(GL_BLEND_SRC_RGB, &_lastBlending[0]);
    glGetIntegerv(GL_BLEND_DST_RGB, &_lastBlending[1]);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &_lastBlending[2]);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &_lastBlending[3]);

    // モデル描画直前のFBOとビューポートを保存
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_lastFBO);
    glGetIntegerv(GL_VIEWPORT, _lastViewport);

}

void CubismRendererProfile_OpenGLCore::Restore()
{
    glUseProgram(_lastProgram);

    glBindVertexArray(_lastVAO);

    SetGlEnable(GL_SCISSOR_TEST, _lastScissorTest);
    SetGlEnable(GL_STENCIL_TEST, _lastStencilTest);
    SetGlEnable(GL_DEPTH_TEST, _lastDepthTest);
    SetGlEnable(GL_CULL_FACE, _lastCullFace);
    SetGlEnable(GL_BLEND, _lastBlend);

    glFrontFace(_lastFrontFace);

    glColorMask(_lastColorMask[0], _lastColorMask[1], _lastColorMask[2], _lastColorMask[3]);

    glBindBuffer(GL_ARRAY_BUFFER, _lastArrayBufferBinding); //前にバッファがバインドされていたら破棄する必要がある
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _lastElementArrayBufferBinding);

    glActiveTexture(GL_TEXTURE1); //テクスチャユニット1を復元
    glBindTexture(GL_TEXTURE_2D, _lastTexture1Binding2D);

    glActiveTexture(GL_TEXTURE0); //テクスチャユニット0を復元
    glBindTexture(GL_TEXTURE_2D, _lastTexture0Binding2D);

    glActiveTexture(_lastActiveTexture);

    // restore blending
    glBlendFuncSeparate(_lastBlending[0], _lastBlending[1], _lastBlending[2], _lastBlending[3]);
}

/*********************************************************************************************************************
 *                                      CubismRenderer_OpenGLCore
 ********************************************************************************************************************/

#ifdef CSM_TARGET_WIN_GL


void CubismRenderer_OpenGLCore::CheckGlError(const csmChar* message)
{
    GLenum errcode = glGetError();
    if (errcode != GL_NO_ERROR)
    {
        CubismLogError("0x%04X(%4d) : %s", errcode, errcode, message);
    }
}

#endif  //CSM_TARGET_WIN_GL

CubismRenderer* CubismRenderer::Create()
{
    return CSM_NEW CubismRenderer_OpenGLCore();
}

void CubismRenderer::StaticRelease()
{
    CubismRenderer_OpenGLCore::DoStaticRelease();
}

CubismRenderer_OpenGLCore::CubismRenderer_OpenGLCore() : _clippingManager(NULL)
                                                     , _clippingContextBufferForMask(NULL)
                                                     , _clippingContextBufferForDraw(NULL)
{
    // テクスチャ対応マップの容量を確保しておく.
    _textures.PrepareCapacity(32, true);
    glGenBuffers(1, &VBO1);
    glGenBuffers(1, &VBO2);
    glGenBuffers(1, &IBO);
    glGenVertexArrays(1, &VAO);
}

CubismRenderer_OpenGLCore::~CubismRenderer_OpenGLCore()
{
    CSM_DELETE_SELF(CubismClippingManager_OpenGLCore, _clippingManager);

    for (csmInt32 i = 0; i < _offscreenSurfaces.GetSize(); ++i)
    {
        if (_offscreenSurfaces[i].IsValid())
        {
            _offscreenSurfaces[i].DestroyOffscreenSurface();
        }
    }
    _offscreenSurfaces.Clear();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &VBO2);
    glDeleteBuffers(1, &IBO);
}

void CubismRenderer_OpenGLCore::DoStaticRelease()
{
    CubismShader_OpenGLCore::DeleteInstance();
}

void CubismRenderer_OpenGLCore::Initialize(CubismModel* model)
{
    Initialize(model, 1);
}

void CubismRenderer_OpenGLCore::Initialize(CubismModel* model, csmInt32 maskBufferCount)
{
    // 1未満は1に補正する
    if (maskBufferCount < 1)
    {
        maskBufferCount = 1;
        CubismLogWarning("The number of render textures must be an integer greater than or equal to 1. Set the number of render textures to 1.");
    }

    if (model->IsUsingMasking())
    {
        _clippingManager = CSM_NEW CubismClippingManager_OpenGLCore();  //クリッピングマスク・バッファ前処理方式を初期化
        _clippingManager->Initialize(
            *model,
            maskBufferCount
        );

        _offscreenSurfaces.Clear();
        for (csmInt32 i = 0; i < maskBufferCount; ++i)
        {
            CubismOffscreenSurface_OpenGLCore offscreenSurface;
            offscreenSurface.CreateOffscreenSurface(_clippingManager->GetClippingMaskBufferSize().X, _clippingManager->GetClippingMaskBufferSize().Y);
            _offscreenSurfaces.PushBack(offscreenSurface);
        }

    }

    _sortedDrawableIndexList.Resize(model->GetDrawableCount(), 0);

    CubismRenderer::Initialize(model, maskBufferCount);  //親クラスの処理を呼ぶ
}

void CubismRenderer_OpenGLCore::PreDraw()
{
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glColorMask(1, 1, 1, 1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0); //前にバッファがバインドされていたら破棄する必要がある

    //異方性フィルタリング。プラットフォームのOpenGLによっては未対応の場合があるので、未設定のときは設定しない
    if (GetAnisotropy() > 0.0f)
    {
        for (csmInt32 i = 0; i < _textures.GetSize(); i++)
        {
            glBindTexture(GL_TEXTURE_2D, _textures[i]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, GetAnisotropy());
        }
    }
}


void CubismRenderer_OpenGLCore::DoDrawModel()
{
    //------------ クリッピングマスク・バッファ前処理方式の場合 ------------
    if (_clippingManager != NULL)
    {
        PreDraw();

        // サイズが違う場合はここで作成しなおし
        for (csmInt32 i = 0; i < _clippingManager->GetRenderTextureCount(); ++i)
        {
            if (_offscreenSurfaces[i].GetBufferWidth() != static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().X) ||
                _offscreenSurfaces[i].GetBufferHeight() != static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().Y))
            {
                _offscreenSurfaces[i].CreateOffscreenSurface(
                    static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().X), static_cast<csmUint32>(_clippingManager->GetClippingMaskBufferSize().Y));
            }
        }

        if (IsUsingHighPrecisionMask())
        {
           _clippingManager->SetupMatrixForHighPrecision(*GetModel(), false);
        }
        else
        {
           _clippingManager->SetupClippingContext(*GetModel(), this, _rendererProfile._lastFBO, _rendererProfile._lastViewport);
        }
    }

    // 上記クリッピング処理内でも一度PreDrawを呼ぶので注意!!
    PreDraw();

    const csmInt32 drawableCount = GetModel()->GetDrawableCount();
    const csmInt32* renderOrder = GetModel()->GetDrawableRenderOrders();

    // インデックスを描画順でソート
    for (csmInt32 i = 0; i < drawableCount; ++i)
    {
        const csmInt32 order = renderOrder[i];
        _sortedDrawableIndexList[order] = i;
    }

    // 描画
    for (csmInt32 i = 0; i < drawableCount; ++i)
    {
        const csmInt32 drawableIndex = _sortedDrawableIndexList[i];

        // Drawableが表示状態でなければ処理をパスする
        if (!GetModel()->GetDrawableDynamicFlagIsVisible(drawableIndex))
        {
            continue;
        }

        // クリッピングマスク
        CubismClippingContext_OpenGLCore* clipContext = (_clippingManager != NULL)
            ? (*_clippingManager->GetClippingContextListForDraw())[drawableIndex]
            : NULL;

        if (clipContext != NULL && IsUsingHighPrecisionMask()) // マスクを書く必要がある
        {
            if(clipContext->_isUsing) // 書くことになっていた
            {
                // 生成したOffscreenSurfaceと同じサイズでビューポートを設定
                glViewport(0, 0, _clippingManager->GetClippingMaskBufferSize().X, _clippingManager->GetClippingMaskBufferSize().Y);

                PreDraw(); // バッファをクリアする

                // ---------- マスク描画処理 ----------
                // マスク用RenderTextureをactiveにセット
                GetMaskBuffer(clipContext->_bufferIndex)->BeginDraw(_rendererProfile._lastFBO);

                // マスクをクリアする
                // 1が無効（描かれない）領域、0が有効（描かれる）領域。（シェーダで Cd*Csで0に近い値をかけてマスクを作る。1をかけると何も起こらない）
                glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            {
                const csmInt32 clipDrawCount = clipContext->_clippingIdCount;
                for (csmInt32 index = 0; index < clipDrawCount; index++)
                {
                    const csmInt32 clipDrawIndex = clipContext->_clippingIdList[index];

                    // 頂点情報が更新されておらず、信頼性がない場合は描画をパスする
                    if (!GetModel()->GetDrawableDynamicFlagVertexPositionsDidChange(clipDrawIndex))
                    {
                        continue;
                    }

                    IsCulling(GetModel()->GetDrawableCulling(clipDrawIndex) != 0);

                    // 今回専用の変換を適用して描く
                    // チャンネルも切り替える必要がある(A,R,G,B)
                    SetClippingContextBufferForMask(clipContext);

                    DrawMeshOpenGL(*GetModel(), clipDrawIndex);
                }
            }

            {
                // --- 後処理 ---
                GetMaskBuffer(clipContext->_bufferIndex)->EndDraw();
                SetClippingContextBufferForMask(NULL);
                glViewport(_rendererProfile._lastViewport[0], _rendererProfile._lastViewport[1], _rendererProfile._lastViewport[2], _rendererProfile._lastViewport[3]);

                PreDraw(); // バッファをクリアする
            }
        }

        // クリッピングマスクをセットする
        SetClippingContextBufferForDraw(clipContext);

        IsCulling(GetModel()->GetDrawableCulling(drawableIndex) != 0);

        DrawMeshOpenGL(*GetModel(), drawableIndex);
    }

    PostDraw();

}

void CubismRenderer_OpenGLCore::DrawMeshOpenGL(const CubismModel& model, const csmInt32 index)
{
#ifndef CSM_DEBUG
    if (_textures[model.GetDrawableTextureIndex(index)] == 0) return;    // モデルが参照するテクスチャがバインドされていない場合は描画をスキップする
#endif

    // 裏面描画の有効・無効
    if (IsCulling())
    {
        glEnable(GL_CULL_FACE);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

    glFrontFace(GL_CCW);    // Cubism SDK OpenGLはマスク・アートメッシュ共にCCWが表面

    csmInt32 indexCount = model.GetDrawableVertexIndexCount(index);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(csmUint16) * indexCount, model.GetDrawableVertexIndices(index), GL_STREAM_DRAW);
    if (IsGeneratingMask())  // マスク生成時
    {
        CubismShader_OpenGLCore::GetInstance()->SetupShaderProgramForMask(this, model, index);
    }
    else {
        CubismShader_OpenGLCore::GetInstance()->SetupShaderProgramForDraw(this, model, index);
    }
    // ポリゴンメッシュを描画する
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);

    // 後処理
    glUseProgram(0);
    SetClippingContextBufferForDraw(NULL);
    SetClippingContextBufferForMask(NULL);
}

void CubismRenderer_OpenGLCore::SaveProfile()
{
    _rendererProfile.Save();
}

void CubismRenderer_OpenGLCore::RestoreProfile()
{
    _rendererProfile.Restore();
}

void CubismRenderer_OpenGLCore::BindTexture(csmUint32 modelTextureIndex, GLuint glTextureIndex)
{
    _textures[modelTextureIndex] = glTextureIndex;
}

const csmMap<csmInt32, GLuint>& CubismRenderer_OpenGLCore::GetBindedTextures() const
{
    return _textures;
}

void CubismRenderer_OpenGLCore::SetClippingMaskBufferSize(csmFloat32 width, csmFloat32 height)
{
    if (_clippingManager == NULL)
    {
        return;
    }

    // インスタンス破棄前にレンダーテクスチャの数を保存
    const csmInt32 renderTextureCount = _clippingManager->GetRenderTextureCount();

    //OffscreenSurfaceのサイズを変更するためにインスタンスを破棄・再作成する
    CSM_DELETE_SELF(CubismClippingManager_OpenGLCore, _clippingManager);

    _clippingManager = CSM_NEW CubismClippingManager_OpenGLCore();

    _clippingManager->SetClippingMaskBufferSize(width, height);

    _clippingManager->Initialize(
        *GetModel(),
        renderTextureCount
    );
}

csmInt32 CubismRenderer_OpenGLCore::GetRenderTextureCount() const
{
    return _clippingManager->GetRenderTextureCount();
}

CubismVector2 CubismRenderer_OpenGLCore::GetClippingMaskBufferSize() const
{
    return _clippingManager->GetClippingMaskBufferSize();
}

CubismOffscreenSurface_OpenGLCore* CubismRenderer_OpenGLCore::GetMaskBuffer(csmInt32 index)
{
    return &_offscreenSurfaces[index];
}

void CubismRenderer_OpenGLCore::SetClippingContextBufferForMask(CubismClippingContext_OpenGLCore* clip)
{
    _clippingContextBufferForMask = clip;
}

CubismClippingContext_OpenGLCore* CubismRenderer_OpenGLCore::GetClippingContextBufferForMask() const
{
    return _clippingContextBufferForMask;
}

void CubismRenderer_OpenGLCore::SetClippingContextBufferForDraw(CubismClippingContext_OpenGLCore* clip)
{
    _clippingContextBufferForDraw = clip;
}

CubismClippingContext_OpenGLCore* CubismRenderer_OpenGLCore::GetClippingContextBufferForDraw() const
{
    return _clippingContextBufferForDraw;
}

const csmBool inline CubismRenderer_OpenGLCore::IsGeneratingMask() const
{
    return (GetClippingContextBufferForMask() != NULL);
}

GLuint CubismRenderer_OpenGLCore::GetBindedTextureId(csmInt32 textureId)
{
    return (_textures[textureId] != 0) ? _textures[textureId] : -1;
}

}}}}

//------------ LIVE2D NAMESPACE ------------
