#pragma once
#include "../pch.h"


enum class DDSFormat {
	DXT1,
	DXT3,
	DXT5,
	ATI2
};

struct DDSImage {
	int Offset;
	int Length;
};

struct DDSInfo {
	int Width;
	int Height;
	
	DXGI_FORMAT DXGIFormat;
	DDSFormat Format;
	int Flags;
	int MipLevelsCount;

	bool IsCubemap;
	

	std::vector<DDSImage> Images;
};

class DDSParser {
public:
	DDSParser(const std::string& path);

	std::vector<DDSImage> GetDDSImages();
	std::vector<DDSImage> GetDDSCubemapImages(int mipLevel);
	DXGI_FORMAT GetDDSFormat();
	int GetHeight();
	int GetWidth();
	int GetMipLevelsCount();
	bool IsCubemap();
	
	void DecodeDDS(DDSImage image, std::vector<uint8_t>& outRgbaData);

protected:
	void ReadToBuffer(const std::string& path);
	void ParseHeaders();

protected:
	void DecodeDxt1(const uint8_t* src, int width, int height, std::vector<uint8_t>& dst);
	void DecodeDxt3(const uint8_t* src, int width, int height, std::vector<uint8_t>& dst);
	void DecodeDxt5(const uint8_t* src, int width, int height, std::vector<uint8_t>& dst);
	void DecodeAti2(const uint8_t* src, int width, int height, std::vector<uint8_t>& dst);

protected:
	DDSInfo m_xInfo;
	std::vector<uint32_t> m_vBuffer;

protected:
	uint8_t m_aDXT3colors[12];
	uint8_t m_aDXT5alphas[8];
	uint8_t m_aRed[8];
	uint8_t m_aGreen[8];
};
