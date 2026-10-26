#pragma once
#include <Vagante/sfml.h>
#include <Acolyte/exports.h>
#include <sigslot/signal.hpp>
#include <unordered_map>
#include <string>

struct TextureData {
	sf::Texture texture{};
	std::vector<sf::Image> images;
};
inline std::unordered_map<std::string, TextureData> TextureDataMap;

namespace sdk::Resources {
	std::vector<sf::Image> SeparateImageChunks(sf::Image& image, int chunkSize);
	sf::Image StitchImageChunks(std::vector<sf::Image> images, int width, int chunkSize);

	// Call once resources are ready (or from OnResourceInitialized).
	// Captures the texture's existing square frames without resetting earlier additions.
	ACOLYTE_API bool InitTextureData(const std::string& texturePath, int chunkSize);
	// Initializes on first use, appends one image, and updates the texture override.
	// Preserves the original sheet width; the image must be chunkSize by chunkSize.
	ACOLYTE_API bool AppendTextureImage(const std::string& texturePath, const std::string& imagePath, int chunkSize);

	using CopyToImageFn = sf::Image* (__thiscall*)(sf::Texture* textureThis, sf::Image* __return_storage_ptr__);
	using LoadImageFromFileFn = bool(__thiscall*)(sf::Image* imageThis, std::string* path);
	using LoadTextureFromImageFn = bool(__thiscall*)(sf::Texture* textureThis, sf::Image* param_1, sf::Rect<int>* param_2);
	using SaveImageToFileFn = bool(__thiscall*)(sf::Image* imageThis, std::string* param_1);
	using TextureConstructorFn = sf::Texture* (__thiscall*)(sf::Texture* textureThis);
	using TextureDestructorFn = void (__thiscall*)(sf::Texture* textureThis);
	using LoadTextureFromFileFn = bool(__thiscall*)(sf::Texture* textureThis, std::string* filePath, sf::Rect<int>* size);
	using GetTextureFn = sf::Texture* (__cdecl*)(std::basic_string<char> filePath, bool param_2);

	ACOLYTE_API extern CopyToImageFn CopyToImage;
	ACOLYTE_API extern LoadImageFromFileFn LoadImageFromFile;
	ACOLYTE_API extern LoadTextureFromImageFn LoadTextureFromImage;
	ACOLYTE_API extern SaveImageToFileFn SaveImageToFile;
	ACOLYTE_API extern TextureConstructorFn TextureConstructor;
	ACOLYTE_API extern TextureDestructorFn TextureDestructor;
	ACOLYTE_API extern LoadTextureFromFileFn LoadTextureFromFile;
	ACOLYTE_API extern GetTextureFn GetTexture;

	ACOLYTE_API extern bool ResourcesInitialized;

	ACOLYTE_API extern sigslot::signal<> OnResourceInitialized;
}
