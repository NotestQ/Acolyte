#include <Acolyte/resources.h>
#include <Acolyte/logger.h>
#include <Acolyte/utils.h>
#include <sdk_registry.h>
#include <windows.h>
#include <safetyhook.hpp>
#include <MinHook.h>
#include <iostream>
#include <algorithm>

static Logger textureLogger("Acolyte");
static const uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

namespace sdk::Resources {
    static std::string NormalizeTexturePath(std::string path) {
        std::replace(path.begin(), path.end(), '\\', '/');
        return path;
    }

    bool InitTextureData(const std::string& texturePath, int chunkSize) {
        if (!ResourcesInitialized || chunkSize <= 0) {
            textureLogger.LogError("Cannot initialize texture before resources are ready or with an invalid chunk size: " + texturePath);
            return false;
        }

        const std::string key = NormalizeTexturePath(texturePath);
        auto existing = TextureDataMap.find(key);
        if (existing != TextureDataMap.end() && !existing->second.images.empty()) {
            const auto size = existing->second.images.front().m_size;
            if (size.x != chunkSize || size.y != chunkSize) {
                textureLogger.LogError("Texture already initialized with a different chunk size: " + key);
                return false;
            }
            return true;
        }

        sf::Texture* texture = GetTexture(key, false);
        if (!texture) {
            textureLogger.LogError("Texture not found: " + key);
            return false;
        }

        sf::Image image{};
        CopyToImage(texture, &image);
        if (image.m_size.x == 0 || image.m_size.y == 0 ||
            image.m_size.x % chunkSize != 0 || image.m_size.y % chunkSize != 0) {
            textureLogger.LogError("Texture dimensions must be multiples of the chunk size: " + key);
            return false;
        }

        TextureData data{};
        data.texture = *texture;
        data.images = SeparateImageChunks(image, chunkSize);
        TextureDataMap[key] = std::move(data);
        return true;
    }

    bool AppendTextureImage(const std::string& texturePath, const std::string& imagePath, int chunkSize) {
        if (!ResourcesInitialized || chunkSize <= 0) {
            textureLogger.LogError("Cannot append texture image before resources are ready or with an invalid chunk size: " + texturePath);
            return false;
        }

        sf::Image image{};
        std::string path = imagePath;
        if (!LoadImageFromFile(&image, &path)) {
            textureLogger.LogError("Failed to load texture image: " + imagePath);
            return false;
        }
        if (image.m_size.x != chunkSize || image.m_size.y != chunkSize) {
            textureLogger.LogError("Image dimensions must match the chunk size: " + imagePath);
            return false;
        }

        const std::string key = NormalizeTexturePath(texturePath);
        if (!InitTextureData(key, chunkSize))
            return false;

        auto& data = TextureDataMap.at(key);
        data.images.push_back(std::move(image));
        sf::Image stitchedImage = StitchImageChunks(data.images, data.texture.m_size.x, chunkSize);

        sf::Texture texture{};
        TextureConstructor(&texture);
        sf::Rect<int> rect{};
        if (!LoadTextureFromImage(&texture, &stitchedImage, &rect)) {
            data.images.pop_back();
            textureLogger.LogError("Failed to update texture: " + key);
            return false;
        }

        data.texture = texture;
        return true;
    }

    std::vector<sf::Image> SeparateImageChunks(sf::Image& image, int chunkSize) {
        std::vector<sf::Image> returnVector;

        sf::Vector2<uint> imageSize = image.m_size;
        unsigned int width = imageSize.x;
        unsigned int height = imageSize.y;

        unsigned int xChunks = width / chunkSize;
        unsigned int yChunks = height / chunkSize;

        returnVector.reserve(xChunks * yChunks);

        const std::vector<unsigned char>& src = image.m_pixels;
        const unsigned int srcStride = width * 4;

        for (unsigned int cy = 0; cy < yChunks; cy++) {
            unsigned int yStart = cy * chunkSize;
            unsigned int yEnd = min(yStart + chunkSize, height);

            for (unsigned int cx = 0; cx < xChunks; cx++) {
                unsigned int xStart = cx * chunkSize;
                unsigned int xEnd = min(xStart + chunkSize, width);

                sf::Image newImage;
                newImage.m_size = sf::Vector2<uint>(chunkSize, chunkSize);
                newImage.m_pixels.clear();
                newImage.m_pixels.reserve(chunkSize * chunkSize * 4);

                for (unsigned int y = yStart; y < yEnd; ++y) {
                    for (unsigned int x = xStart; x < xEnd; ++x) {
                        unsigned int pixelIndex = (x + (y * width)) * 4;

                        newImage.m_pixels.push_back(src[pixelIndex + 0]);
                        newImage.m_pixels.push_back(src[pixelIndex + 1]);
                        newImage.m_pixels.push_back(src[pixelIndex + 2]);
                        newImage.m_pixels.push_back(src[pixelIndex + 3]);
                    }
                }

                returnVector.push_back(std::move(newImage));
            }
        }

        textureLogger.LogDebug("Image separated into " + std::to_string(returnVector.size()) + " chunks...");
        return returnVector;
    }

    sf::Image StitchImageChunks(std::vector<sf::Image> images, int width, int chunkSize) {
        sf::Image returnImage;

        unsigned int xChunks = width / chunkSize;
        unsigned int yChunks = (images.size() + xChunks - 1) / xChunks;
        unsigned int stitchHeight = yChunks * chunkSize;

        returnImage.m_size = sf::Vector2<uint>(width, stitchHeight);
        returnImage.m_pixels.resize(width * stitchHeight * 4, 0);
        textureLogger.LogDebug("Stitching " + std::to_string(images.size()) + 
            ((images.size() > 1) ? " chunks" : " chunk") + " into one " + 
            std::to_string(width) + " by " + std::to_string(stitchHeight) + " image");
        
        for (unsigned int idx = 0; idx < images.size(); ++idx) {
            const sf::Image& chunk = images[idx];
            unsigned int cx = idx % xChunks; // horizontal position in chunks
            unsigned int cy = idx / xChunks; // vertical position in chunks

            for (unsigned int y = 0; y < chunk.m_size.y; ++y) {
                for (unsigned int x = 0; x < chunk.m_size.x; ++x) {
                    unsigned int srcIndex = (y * chunk.m_size.x + x) * 4;
                    unsigned int destX = cx * chunkSize + x;
                    unsigned int destY = cy * chunkSize + y;
                    unsigned int destIndex = (destY * width + destX) * 4;

                    returnImage.m_pixels[destIndex + 0] = chunk.m_pixels[srcIndex + 0];
                    returnImage.m_pixels[destIndex + 1] = chunk.m_pixels[srcIndex + 1];
                    returnImage.m_pixels[destIndex + 2] = chunk.m_pixels[srcIndex + 2];
                    returnImage.m_pixels[destIndex + 3] = chunk.m_pixels[srcIndex + 3];
                }
            }
        }

        return returnImage;
    }

	CopyToImageFn CopyToImage = nullptr;
	LoadImageFromFileFn LoadImageFromFile = nullptr;
	LoadTextureFromImageFn LoadTextureFromImage = nullptr;
	SaveImageToFileFn SaveImageToFile = nullptr;
	TextureConstructorFn TextureConstructor = nullptr;
	TextureDestructorFn TextureDestructor = nullptr;
	LoadTextureFromFileFn LoadTextureFromFile = nullptr;
	GetTextureFn GetTexture = nullptr;
    GetTextureFn pOriginalGetTexture = nullptr;
    
    bool ResourcesInitialized = false;
    sigslot::signal<> OnResourceInitialized;

    static sf::Texture* __cdecl GetTextureDetour(std::string filePath, bool param_2) {
        // Spammy but useful, maybe when we get debug levels
        textureLogger.LogDebug("Requested path: " + filePath + " " + std::to_string(param_2));
        const std::string key = NormalizeTexturePath(filePath);
        if (TextureDataMap.contains(key)) {
            //textureLogger.LogWarning("File path present, returning from texture data map...");
            return &TextureDataMap[key].texture;
        }

        return pOriginalGetTexture(filePath, param_2);
    }

    void resourceInitializedDetour(SafetyHookContext& ctx) {
        textureLogger.LogWarning("Resources initialized!");
        ResourcesInitialized = true;
        sdk::Resources::OnResourceInitialized();
    }

    static void InitVaganteUtils() {
		textureLogger.LogDebug("Initializing VaganteUtils...");

		CopyToImage = reinterpret_cast<CopyToImageFn>(moduleBaseAddress + 0x4169E0);
		TextureDestructor = reinterpret_cast<TextureDestructorFn>(moduleBaseAddress + 0x4167F0);
		LoadImageFromFile = reinterpret_cast<LoadImageFromFileFn>(moduleBaseAddress + 0x4184E0);
		LoadTextureFromImage = reinterpret_cast<LoadTextureFromImageFn>(moduleBaseAddress + 0x4173D0);
		SaveImageToFile = reinterpret_cast<SaveImageToFileFn>(moduleBaseAddress + 0x418520);
		TextureConstructor = reinterpret_cast<TextureConstructorFn>(moduleBaseAddress + 0x416730);
		LoadTextureFromFile = reinterpret_cast<LoadTextureFromFileFn>(moduleBaseAddress + 0x417350);
		GetTexture = reinterpret_cast<GetTextureFn>(moduleBaseAddress + 0x34CB20);

        Utils::CreateHook(reinterpret_cast<void**>(reinterpret_cast<GetTextureFn>(moduleBaseAddress + 0x34CB20)), &GetTextureDetour, reinterpret_cast<void**>(&pOriginalGetTexture));

        static SafetyHookMid resourceInitializedHook = safetyhook::create_mid(reinterpret_cast<void*>(moduleBaseAddress + 0x33CA41), resourceInitializedDetour); // Main after line 670

		textureLogger.LogDebug("VaganteUtils initialized!");
	}

	SDK_REGISTER(InitVaganteUtils);
}
