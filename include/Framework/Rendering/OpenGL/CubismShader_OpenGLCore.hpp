/**
 * Copyright(c) Live2D Inc. All rights reserved.
 *
 * Use of this source code is governed by the Live2D Open Software license
 * that can be found at https://www.live2d.com/eula/live2d-open-software-license-agreement_en.html.
 */

#pragma once

#include "Framework/CubismFramework.hpp"
#include "Framework/Rendering/OpenGL/CubismRenderer_OpenGLCore.hpp"

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

class CubismRenderer_OpenGLCore;
class CubismClippingContext_OpenGLCore;

/**
 * @brief   OpenGL用のシェーダプログラムを生成・破棄するクラス<br>
 *           シングルトンなクラスであり、CubismShader_OpenGL::GetInstance()からアクセスする。
 *
 */
class CubismShader_OpenGLCore
{
public:
    /**
     * @brief   インスタンスを取得する（シングルトン）。
     *
     * @return  インスタンスのポインタ
     */
    static CubismShader_OpenGLCore* GetInstance();

    /**
     * @brief   インスタンスを解放する（シングルトン）。
     */
    static void DeleteInstance();

    /**
     * @brief   描画用のシェーダプログラムの一連のセットアップを実行する
     *
     * @param[in]   renderer              ->  レンダラー
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画対象のメッシュのインデックス
     */
    void SetupShaderProgramForDraw(CubismRenderer_OpenGLCore* renderer, const CubismModel& model, const csmInt32 index);

    /**
     * @brief   マスク用のシェーダプログラムの一連のセットアップを実行する
     *
     * @param[in]   renderer              ->  レンダラー
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画対象のメッシュのインデックス
     */
    void SetupShaderProgramForMask(CubismRenderer_OpenGLCore* renderer, const CubismModel& model, const csmInt32 index);

private:
    /**
    * @bref    シェーダープログラムとシェーダ変数のアドレスを保持する構造体
    *
    */
    struct CubismShaderSet
    {
        GLuint ShaderProgram;               ///< シェーダプログラムのアドレス
        GLuint AttributePositionLocation;   ///< シェーダプログラムに渡す変数のアドレス(Position)
        GLuint AttributeTexCoordLocation;   ///< シェーダプログラムに渡す変数のアドレス(TexCoord)
        GLint UniformMatrixLocation;        ///< シェーダプログラムに渡す変数のアドレス(Matrix)
        GLint UniformClipMatrixLocation;    ///< シェーダプログラムに渡す変数のアドレス(ClipMatrix)
        GLint SamplerTexture0Location;      ///< シェーダプログラムに渡す変数のアドレス(Texture0)
        GLint SamplerTexture1Location;      ///< シェーダプログラムに渡す変数のアドレス(Texture1)
        GLint UniformBaseColorLocation;     ///< シェーダプログラムに渡す変数のアドレス(BaseColor)
        GLint UniformMultiplyColorLocation; ///< シェーダプログラムに渡す変数のアドレス(MultiplyColor)
        GLint UniformScreenColorLocation;   ///< シェーダプログラムに渡す変数のアドレス(ScreenColor)
        GLint UnifromChannelFlagLocation;   ///< シェーダプログラムに渡す変数のアドレス(ChannelFlag)
    };

    /**
     * @brief   privateなコンストラクタ
     */
    CubismShader_OpenGLCore();

    /**
     * @brief   privateなデストラクタ
     */
    virtual ~CubismShader_OpenGLCore();

    /**
     * @brief   シェーダプログラムを解放する
     */
    void ReleaseShaderProgram();

    /**
     * @brief   シェーダプログラムを初期化する
     */
    void GenerateShaders();

    /**
     * @brief   シェーダプログラムをロードしてアドレス返す。
     *
     * @param[in]   vertShaderSrc   ->  頂点シェーダのソース
     * @param[in]   fragShaderSrc   ->  フラグメントシェーダのソース
     *
     * @return  シェーダプログラムのアドレス
     */
    GLuint LoadShaderProgram(const csmChar* vertShaderSrc, const csmChar* fragShaderSrc);

    /**
     * @brief   シェーダプログラムをコンパイルする
     *
     * @param[out]  outShader       ->  コンパイルされたシェーダプログラムのアドレス
     * @param[in]   shaderType      ->  シェーダタイプ(Vertex/Fragment)
     * @param[in]   shaderSource    ->  シェーダソースコード
     *
     * @retval      true         ->      コンパイル成功
     * @retval      false        ->      コンパイル失敗
     */
    csmBool CompileShaderSource(GLuint* outShader, GLenum shaderType, const csmChar* shaderSource);

    /**
     * @brief   シェーダプログラムをリンクする
     *
     * @param[in]   shaderProgram   ->  リンクするシェーダプログラムのアドレス
     *
     * @retval      true            ->  リンク成功
     * @retval      false           ->  リンク失敗
     */
    csmBool LinkProgram(GLuint shaderProgram);

    /**
     * @brief   シェーダプログラムを検証する
     *
     * @param[in]   shaderProgram   ->  検証するシェーダプログラムのアドレス
     *
     * @retval      true            ->  正常
     * @retval      false           ->  異常
     */
    csmBool ValidateProgram(GLuint shaderProgram);

    /**
     * @brief   必要な頂点属性を設定する
     *
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画対象のメッシュのインデックス
     * @param[in]   shaderSet             ->  シェーダープログラムのセット
     */
    static void SetVertexAttributes(const CubismModel& model, const csmInt32 index, CubismShaderSet* shaderSet, GLuint pos, GLuint uv);

    /**
     * @brief   テクスチャの設定を行う
     *
     * @param[in]   renderer              ->  レンダラー
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画対象のメッシュのインデックス
     * @param[in]   shaderSet             ->  シェーダープログラムのセット
     */
    static void SetupTexture(CubismRenderer_OpenGLCore* renderer, const CubismModel& model, const csmInt32 index, CubismShaderSet* shaderSet);

    /**
     * @brief   色関連のユニフォーム変数の設定を行う
     *
     * @param[in]   renderer              ->  レンダラー
     * @param[in]   model                 ->  描画対象のモデル
     * @param[in]   index                 ->  描画対象のメッシュのインデックス
     * @param[in]   shaderSet             ->  シェーダープログラムのセット
     * @param[in]   baseColor             ->  ベースカラー
     * @param[in]   multiplyColor         ->  乗算カラー
     * @param[in]   screenColor           ->  スクリーンカラー
     */
    static void SetColorUniformVariables(CubismRenderer_OpenGLCore* renderer, const CubismModel& model, const csmInt32 index, CubismShaderSet* shaderSet,
                                  CubismRenderer::CubismTextureColor& baseColor, CubismRenderer::CubismTextureColor& multiplyColor, CubismRenderer::CubismTextureColor& screenColor);

    /**
     * @brief   カラーチャンネル関連のユニフォーム変数の設定を行う
     *
     * @param[in]   shaderSet             ->  シェーダープログラムのセット
     * @param[in]   contextBuffer         ->  描画コンテクスト
     */
    static void SetColorChannelUniformVariables(CubismShaderSet* shaderSet, CubismClippingContext_OpenGLCore* contextBuffer);

    csmVector<CubismShaderSet*> _shaderSets;   ///< ロードしたシェーダプログラムを保持する変数

};

}}}}
//------------ LIVE2D NAMESPACE ------------
