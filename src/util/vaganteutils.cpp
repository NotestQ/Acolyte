#include <Acolyte/vaganteutils.h>
#include <Acolyte/logger.h>
#include <Acolyte/utils.h>
#include <sdk_registry.h>
#include <windows.h>
#include <safetyhook.hpp>
#include <MinHook.h>
#include <iostream>

static Logger textureLogger("Acolyte");
static const uintptr_t moduleBaseAddress = (uintptr_t)GetModuleHandleA(NULL);

namespace VaganteUtils {
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
	LoadTextureFromFileFn LoadTextureFromFile = nullptr;
	GetTextureFn GetTexture = nullptr;
    GetTextureFn pOriginalGetTexture = nullptr;

    static sf::Texture* __cdecl GetTextureDetour(std::string filePath, bool param_2) {
        // Spammy but useful, maybe when we get debug levels
        //textureLogger.LogDebug("Requested path: " + filePath + " " + std::to_string(param_2));
        if (TextureDataMap.contains(filePath)) {
            //textureLogger.LogError("File path present, returning from texture data map...");
            return &TextureDataMap[filePath].texture;
        }

        return pOriginalGetTexture(filePath, param_2);
    }

    static void InitVaganteUtils() {
		textureLogger.LogDebug("Initializing VaganteUtils...");

		CopyToImage = reinterpret_cast<CopyToImageFn>(moduleBaseAddress + 0x4169E0);
		LoadImageFromFile = reinterpret_cast<LoadImageFromFileFn>(moduleBaseAddress + 0x4184E0);
		LoadTextureFromImage = reinterpret_cast<LoadTextureFromImageFn>(moduleBaseAddress + 0x4173D0);
		SaveImageToFile = reinterpret_cast<SaveImageToFileFn>(moduleBaseAddress + 0x418520);
		TextureConstructor = reinterpret_cast<TextureConstructorFn>(moduleBaseAddress + 0x416730);
		LoadTextureFromFile = reinterpret_cast<LoadTextureFromFileFn>(moduleBaseAddress + 0x417350);
		GetTexture = reinterpret_cast<GetTextureFn>(moduleBaseAddress + 0x34CB20);

        Utils::CreateHook(reinterpret_cast<void**>(reinterpret_cast<GetTextureFn>(moduleBaseAddress + 0x34CB20)), &GetTextureDetour, reinterpret_cast<void**>(&pOriginalGetTexture));

		textureLogger.LogDebug("VaganteUtils initialized!");
	}

	SDK_REGISTER(InitVaganteUtils);
}