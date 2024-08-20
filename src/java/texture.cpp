module;
#include <Framework/CubismModelSettingJson.hpp>
#include <Framework/Model/CubismUserModel.hpp>
#include <string>
#include <iostream>

#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <fstream>
#include <glad/gl.h>
module Live2D;

import Util;

using namespace Live2D::Cubism::Framework;
using namespace std;
using namespace L2D;

TextureManager::TextureManager() = default;
TextureManager::~TextureManager() = default;

TextureManager::TextureInfo* TextureManager::CreateTextureFromPngFile(const std::string& fileName)
{
	for (csmUint32 i = 0; i < textures.GetSize(); i++) if (textures[i]->fileName == fileName) return textures[i];
	GLuint textureId;
	int width, height, channels;
	auto data = LoadFile(fileName.c_str());
	auto png = stbi_load_from_memory((uint8_t*)data.data(), data.size(), &width, &height, &channels, STBI_rgb_alpha);
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, png);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(png);
	auto textureInfo = new TextureInfo();
	if (textureInfo != nullptr)
	{
		textureInfo->fileName = fileName;
		textureInfo->width = width;
		textureInfo->height = height;
		textureInfo->id = textureId;
		textures.PushBack(textureInfo);
	}
	return textureInfo;
}

void TextureManager::ReleaseTextures()
{
	for (csmUint32 i = 0; i < textures.GetSize(); i++) delete textures[i];
	textures.Clear();
}

void TextureManager::ReleaseTexture(const csmUint32 textureId)
{
	for (csmUint32 i = 0; i < textures.GetSize(); i++)
	{
		if (textures[i]->id != textureId) continue;
		delete textures[i];
		textures.Remove(i);
		break;
	}
}

void TextureManager::ReleaseTexture(const std::string& fileName)
{
	for (csmUint32 i = 0; i < textures.GetSize(); i++)
	{
		if (textures[i]->fileName == fileName)
		{
			delete textures[i];
			textures.Remove(i);
			break;
		}
	}
}

TextureManager::TextureInfo* TextureManager::GetTextureInfoById(GLuint textureId) const
{
	for (csmUint32 i = 0; i < textures.GetSize(); i++) if (textures[i]->id == textureId) return textures[i];
	return nullptr;
}
