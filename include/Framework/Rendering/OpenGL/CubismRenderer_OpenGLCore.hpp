/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "Framework/Rendering//CubismRenderer.hpp"
#include "Framework/Rendering//CubismClippingManager.hpp"
#include "Framework/CubismFramework.hpp"
#include "CubismOffscreenSurface_OpenGLCore.hpp"
#include "CubismShader_OpenGLCore.hpp"
#include "Framework/Type/csmVector.hpp"
#include "Framework/Type/csmRectF.hpp"
#include "Framework/Math/CubismVector2.hpp"
#include "Framework/Type/csmMap.hpp"

#if defined(CSM_TARGET_WIN_GL) || defined(CSM_TARGET_LINUX_GL)
#include <glad/gl.h>
#include <GL/gl.h>
#endif

#ifdef CSM_TARGET_MAC_GL
#ifndef CSM_TARGET_COCOS
#include <glad/gl.h>
#endif
#include <OpenGL/gl.h>
#endif

//------------ LIVE2D NAMESPACE ------------
namespace Live2D { namespace Cubism { namespace Framework { namespace Rendering {

//  前方宣言
class CubismRenderer_OpenGLCore;
class CubismClippingContext_OpenGLCore;
class CubismShader_OpenGLCore;

/**
 * @brief  クリッピングマスクの処理を実行するクラス
 *
 */
class CubismClippingManager_OpenGLCore : public CubismClippingManager<CubismClippingContext_OpenGLCore, CubismOffscreenSurface_OpenGLCore>
{
public:

    /**
     * @brief   クリッピングコンテキストを作成する。モデル描画時に実行する。
     *
     * @param[in]   model        ->  モデルのインスタンス
     * @param[in]   renderer     ->  レンダラのインスタンス
     * @param[in]   lastFBO      ->  フレームバッファ
     * @param[in]   lastViewport ->  ビューポート
     */
    void SetupClippingContext(CubismModel& model, CubismRenderer_OpenGLCore* renderer, GLint lastFBO, GLint lastViewport[4]);
};

/**
 * @brief   クリッピングマスクのコンテキスト
 */
class CubismClippingContext_OpenGLCore : public CubismClippingContext
{
    friend class CubismClippingManager_OpenGLCore;
    friend class CubismRenderer_OpenGLCore;

public:
    /**
     * @brief   引数付きコンストラクタ
     *
     */
    CubismClippingContext_OpenGLCore(CubismClippingManager<CubismClippingContext_OpenGLCore, CubismOffscreenSurface_OpenGLCore>* manager, CubismModel& model, const csmInt32* clippingDrawableIndices, csmInt32 clipCount);

    /**
     * @brief   デストラクタ
     */
    virtual ~CubismClippingContext_OpenGLCore();

    /**
     * @brief   このマスクを管理するマネージャのインスタンスを取得する。
     *
     * @return  クリッピングマネージャのインスタンス
     */
    CubismClippingManager<CubismClippingContext_OpenGLCore, CubismOffscreenSurface_OpenGLCore>* GetClippingManager();

    CubismClippingManager<CubismClippingContext_OpenGLCore, CubismOffscreenSurface_OpenGLCore>* _owner;        ///< このマスクを管理しているマネージャのインスタンス
};

/**
 * @brief   Cubismモデルを描画する直前のOpenGLのステートを保持・復帰させるクラス
 *
 */
class CubismRendererProfile_OpenGLCore
{
    friend class CubismRenderer_OpenGLCore;

private:
    /**
     * @biref   privateなコンストラクタ
     */
    CubismRendererProfile_OpenGLCore() {};

    /**
     * @biref   privateなデストラクタ
     */
    virtual ~CubismRendererProfile_OpenGLCore() {};

    /**
     * @brief   OpenGLのステートを保持する
     */
    void Save();

    /**
     * @brief   保持したOpenGLのステートを復帰させる
     *
     */
    void Restore();

    /**
     * @brief   OpenGLの機能の有効・無効をセットする
     *
     * @param[in]   index   ->  有効・無効にする機能
     * @param[in]   enabled ->  trueなら有効にする
     */
    void SetGlEnable(GLenum index, GLboolean enabled);

    GLint _lastArrayBufferBinding;          ///< モデル描画直前の頂点バッファ
    GLint _lastElementArrayBufferBinding;   ///< モデル描画直前のElementバッファ
    GLint _lastProgram;                     ///< モデル描画直前のシェーダプログラムバッファ
    GLint _lastActiveTexture;               ///< モデル描画直前のアクティブなテクスチャ
    GLint _lastTexture0Binding2D;           ///< モデル描画直前のテクスチャユニット0
    GLint _lastTexture1Binding2D;           ///< モデル描画直前のテクスチャユニット1
    GLint _lastVAO;
    GLboolean _lastScissorTest;             ///< モデル描画直前のGL_VERTEX_ATTRIB_ARRAY_ENABLEDパラメータ
    GLboolean _lastBlend;                   ///< モデル描画直前のGL_SCISSOR_TESTパラメータ
    GLboolean _lastStencilTest;             ///< モデル描画直前のGL_STENCIL_TESTパラメータ
    GLboolean _lastDepthTest;               ///< モデル描画直前のGL_DEPTH_TESTパラメータ
    GLboolean _lastCullFace;                ///< モデル描画直前のGL_CULL_FACEパラメータ
    GLint _lastFrontFace;                   ///< モデル描画直前のGL_CULL_FACEパラメータ
    GLboolean _lastColorMask[4];            ///< モデル描画直前のGL_COLOR_WRITEMASKパラメータ
    GLint _lastBlending[4];                 ///< モデル描画直前のカラーブレンディングパラメータ
    GLint _lastFBO;                         ///< モデル描画直前のフレームバッファ
    GLint _lastViewport[4];                 ///< モデル描画直前のビューポート
};

/**
 * @brief   OpenGL用の描画命令を実装したクラス
 *
 */
class CubismRenderer_OpenGLCore : public CubismRenderer
{
    friend class CubismRenderer;
    friend class CubismClippingManager_OpenGLCore;
    friend class CubismShader_OpenGLCore;

public:
    /**
     * @brief    レンダラの初期化処理を実行する<br>
     *           引数に渡したモデルからレンダラの初期化処理に必要な情報を取り出すことができる
     *
     * @param[in]  model -> モデルのインスタンス
     */
    void Initialize(Framework::CubismModel* model);

    void Initialize(Framework::CubismModel* model, csmInt32 maskBufferCount);

    /**
     * @brief   OpenGLテクスチャのバインド処理<br>
     *           CubismRendererにテクスチャを設定し、CubismRenderer中でその画像を参照するためのIndex値を戻り値とする
     *
     * @param[in]   modelTextureIndex  ->  セットするモデルテクスチャの番号
     * @param[in]   glTextureIndex     ->  OpenGLテクスチャの番号
     *
     */
    void BindTexture(csmUint32 modelTextureIndex, GLuint glTextureIndex);

    /**
     * @brief   OpenGLにバインドされたテクスチャのリストを取得する
     *
     * @return  テクスチャのアドレスのリスト
     */
    const csmMap<csmInt32, GLuint>& GetBindedTextures() const;

    /**
     * @brief  クリッピングマスクバッファのサイズを設定する<br>
     *         マスク用のOffscreenSurfaceを破棄・再作成するため処理コストは高い。
     *
     * @param[in]  size -> クリッピングマスクバッファのサイズ
     *
     */
    void SetClippingMaskBufferSize(csmFloat32 width, csmFloat32 height);

    /**
     * @brief  レンダーテクスチャの枚数を取得する。
     *
     * @return  レンダーテクスチャの枚数
     *
     */
    csmInt32 GetRenderTextureCount() const;

    /**
     * @brief  クリッピングマスクバッファのサイズを取得する
     *
     * @return クリッピングマスクバッファのサイズ
     *
     */
    CubismVector2 GetClippingMaskBufferSize() const;

    /**
     * @brief  クリッピングマスクのバッファを取得する
     *
     * @return クリッピングマスクのバッファへのポインタ
     *
     */
    CubismOffscreenSurface_OpenGLCore* GetMaskBuffer(csmInt32 index);

protected:
    /**
     * @brief   コンストラクタ
     */
    CubismRenderer_OpenGLCore();

    /**
     * @brief   デストラクタ
     */
    virtual ~CubismRenderer_OpenGLCore();

    /**
     * @brief   モデルを描画する実際の処理
     *
     */
    virtual void DoDrawModel() override;

    /**
     * @brief    描画オブジェクト（アートメッシュ）を描画する。
     *
     * @param[in]   model       ->  描画対象のモデル
     * @param[in]   index       ->  描画対象のメッシュのインデックス
     *
     */
    void DrawMeshOpenGL(const CubismModel& model, const csmInt32 index);

private:
    // Prevention of copy Constructor
    CubismRenderer_OpenGLCore(const CubismRenderer_OpenGLCore&);
    CubismRenderer_OpenGLCore& operator=(const CubismRenderer_OpenGLCore&);

    /**
     * @brief   レンダラが保持する静的なリソースを解放する<br>
     *           OpenGLの静的なシェーダプログラムを解放する
     */
    static void DoStaticRelease();

    /**
     * @brief   描画開始時の追加処理。<br>
     *           モデルを描画する前にクリッピングマスクに必要な処理を実装している。
     */
    void PreDraw();

    /**
     * @brief   描画完了後の追加処理。
     *
     */
    void PostDraw(){};

    /**
     * @brief   モデル描画直前のOpenGLのステートを保持する
     */
    virtual void SaveProfile();

    /**
     * @brief   モデル描画直前のOpenGLのステートを保持する
     */
    virtual void RestoreProfile();

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストをセットする。
     */
    void SetClippingContextBufferForMask(CubismClippingContext_OpenGLCore* clip);

    /**
     * @brief   マスクテクスチャに描画するクリッピングコンテキストを取得する。
     *
     * @return  マスクテクスチャに描画するクリッピングコンテキスト
     */
    CubismClippingContext_OpenGLCore* GetClippingContextBufferForMask() const;

    /**
     * @brief   画面上に描画するクリッピングコンテキストをセットする。
     */
    void SetClippingContextBufferForDraw(CubismClippingContext_OpenGLCore* clip);

    /**
     * @brief   画面上に描画するクリッピングコンテキストを取得する。
     *
     * @return  画面上に描画するクリッピングコンテキスト
     */
    CubismClippingContext_OpenGLCore* GetClippingContextBufferForDraw() const;

    /**
     * @brief   マスク生成時かを判定する
     *
     * @return  判定値
     */
    const csmBool inline IsGeneratingMask() const;

    /**
     * @brief   テクスチャマップにバインドされたテクスチャIDを取得する。<br>
     *          バインドされていなければダミーとして-1が返される。
     *
     * @return  バインドされたテクスチャID
     */
    GLuint GetBindedTextureId(csmInt32 textureId);

#ifdef CSM_TARGET_WIN_GL
    /**
     * @brief   Windows対応。OpenGLのエラーチェック。
     */
    void  CheckGlError(const csmChar* message);
#endif

    csmMap<csmInt32, GLuint> _textures;                      ///< モデルが参照するテクスチャとレンダラでバインドしているテクスチャとのマップ
    csmVector<csmInt32> _sortedDrawableIndexList;       ///< 描画オブジェクトのインデックスを描画順に並べたリスト
    CubismRendererProfile_OpenGLCore _rendererProfile;               ///< OpenGLのステートを保持するオブジェクト
    CubismClippingManager_OpenGLCore* _clippingManager;               ///< クリッピングマスク管理オブジェクト
    CubismClippingContext_OpenGLCore* _clippingContextBufferForMask;  ///< マスクテクスチャに描画するためのクリッピングコンテキスト
    CubismClippingContext_OpenGLCore* _clippingContextBufferForDraw;  ///< 画面上描画するためのクリッピングコンテキスト

    csmVector<CubismOffscreenSurface_OpenGLCore>   _offscreenSurfaces;          ///< マスク描画用のフレームバッファ
    GLuint VAO, VBO1, VBO2, IBO;
};

}}}}
//------------ LIVE2D NAMESPACE ------------
