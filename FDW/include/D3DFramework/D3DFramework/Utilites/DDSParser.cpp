#include "DDSParser.h"

#define DDS_MAGIC 0x20534444
#define DDSD_MIPMAPCOUNT 0x20000
#define DDPF_FOURCC 0x4
#define HEADER_LENGTH_INT 31
#define OFF_MAGIC 0
#define OFF_SIZE 1
#define OFF_FLAGS 2
#define OFF_HEIGHT 3
#define OFF_WIDTH 4
#define OFF_MIPMAPCOUNT 7
#define OFF_PFFLAGS 20
#define OFF_PF_FOURCC 21

//CUBEMAP DEFS
#define OFF_CAPS2 28
#define DDSCAPS2_CUBEMAP           0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000

static constexpr uint32_t FourCCToUInt(const char* fourCC) {
	return fourCC[0] | (fourCC[1] << 8) | (fourCC[2] << 16) | (fourCC[3] << 24);
}

static uint8_t convertBitRange(uint32_t fromBits, uint32_t toBits, uint32_t value) {
	return static_cast<uint8_t>((value * ((1 << toBits) - 1) + ((1 << fromBits) / 2)) / ((1 << fromBits) - 1));
}

constexpr uint32_t FOURCC_DXT1 = FourCCToUInt("DXT1");
constexpr uint32_t FOURCC_DXT3 = FourCCToUInt("DXT3");
constexpr uint32_t FOURCC_DXT5 = FourCCToUInt("DXT5");
constexpr uint32_t FOURCC_ATI2 = FourCCToUInt("ATI2");


void DXT3Colors(uint8_t* out, uint16_t color0, uint16_t color1) {
	uint8_t r0 = convertBitRange(5, 8, (color0 >> 11) & 31);
	uint8_t g0 = convertBitRange(6, 8, (color0 >> 5) & 63);
	uint8_t b0 = convertBitRange(5, 8, color0 & 31);

	uint8_t r1 = convertBitRange(5, 8, (color1 >> 11) & 31);
	uint8_t g1 = convertBitRange(6, 8, (color1 >> 5) & 63);
	uint8_t b1 = convertBitRange(5, 8, color1 & 31);

	out[0] = r0; out[1] = g0; out[2] = b0;
	out[3] = r1; out[4] = g1; out[5] = b1;
	out[6] = (5 * r0 + 3 * r1) / 8;
	out[7] = (5 * g0 + 3 * g1) / 8;
	out[8] = (5 * b0 + 3 * b1) / 8;
	out[9] = (5 * r1 + 3 * r0) / 8;
	out[10] = (5 * g1 + 3 * g0) / 8;
	out[11] = (5 * b1 + 3 * b0) / 8;
}

void DXT5Alphas(uint8_t* out, uint8_t alpha0, uint8_t alpha1) {
	out[0] = alpha0;
	out[1] = alpha1;

	if (alpha0 > alpha1) {
		out[2] = (54 * alpha0 + 9 * alpha1) >> 6;
		out[3] = (45 * alpha0 + 18 * alpha1) >> 6;
		out[4] = (36 * alpha0 + 27 * alpha1) >> 6;
		out[5] = (27 * alpha0 + 36 * alpha1) >> 6;
		out[6] = (18 * alpha0 + 45 * alpha1) >> 6;
		out[7] = (9 * alpha0 + 54 * alpha1) >> 6;
	}
	else {
		out[2] = (12 * alpha0 + 3 * alpha1) >> 4;
		out[3] = (9 * alpha0 + 6 * alpha1) >> 4;
		out[4] = (6 * alpha0 + 9 * alpha1) >> 4;
		out[5] = (3 * alpha0 + 12 * alpha1) >> 4;
		out[6] = 0;
		out[7] = 255;
	}
}

void RGColors(uint8_t* out, uint8_t c0, uint8_t c1) {
	out[0] = c0;
	out[1] = c1;

	if (c0 > c1) {
		out[2] = (6 * c0 + 1 * c1) / 7;
		out[3] = (5 * c0 + 2 * c1) / 7;
		out[4] = (4 * c0 + 3 * c1) / 7;
		out[5] = (3 * c0 + 4 * c1) / 7;
		out[6] = (2 * c0 + 5 * c1) / 7;
		out[7] = (1 * c0 + 6 * c1) / 7;
	}
	else {
		out[2] = (4 * c0 + 1 * c1) / 5;
		out[3] = (3 * c0 + 2 * c1) / 5;
		out[4] = (2 * c0 + 3 * c1) / 5;
		out[5] = (1 * c0 + 4 * c1) / 5;
		out[6] = 0;
		out[7] = 1;
	}
}

DDSParser::DDSParser(const std::string& path) {
	ReadToBuffer(path);
	ParseHeaders();
}

std::vector<DDSImage> DDSParser::GetDDSImages() {
	return m_xInfo.Images;
}

std::vector<DDSImage> DDSParser::GetDDSCubemapImages(int mipLevel) {
	std::vector<DDSImage> ret;

	for (int i = 0; i < 6; ++i) {
		ret.push_back(m_xInfo.Images[mipLevel + i * m_xInfo.MipLevelsCount]);
	}
	
	return ret;
}

DXGI_FORMAT DDSParser::GetDDSFormat() {
	return m_xInfo.DXGIFormat;
}

int DDSParser::GetHeight() {
	return m_xInfo.Height;
}

int DDSParser::GetWidth() {
	return m_xInfo.Width;
}

int DDSParser::GetMipLevelsCount() {
	return m_xInfo.MipLevelsCount;
}

bool DDSParser::IsCubemap() {
	return m_xInfo.IsCubemap;
}

void DDSParser::DecodeDDS(DDSImage image, std::vector<uint8_t>& outRgbaData) {
	const int width = m_xInfo.Width;
	const int height = m_xInfo.Height;
	const DDSFormat format = m_xInfo.Format;

	const uint8_t* src = reinterpret_cast<const uint8_t*>(m_vBuffer.data()) + image.Offset;
	outRgbaData.resize(width * height * 4);

	switch (format) {
	case DDSFormat::DXT1:
		DecodeDxt1(src, width, height, outRgbaData);
		break;
	case DDSFormat::DXT3:
		DecodeDxt3(src, width, height, outRgbaData);
		break;
	case DDSFormat::DXT5:
		DecodeDxt5(src, width, height, outRgbaData);
		break;
	case DDSFormat::ATI2:
		DecodeAti2(src, width, height, outRgbaData);
		break;
	default:
		SAFE_ASSERT(false, "Unsupported DDS format for decoding");
	}
}

void DDSParser::ReadToBuffer(const std::string& path) {
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		SAFE_ASSERT(false, "Failed to open file: " + path);
	}

	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();
	file.seekg(0, std::ios::beg);
	m_vBuffer.resize(fileSize / sizeof(uint32_t));
	file.read(reinterpret_cast<char*>(m_vBuffer.data()), fileSize);
	file.close();
}

void DDSParser::ParseHeaders() {
	if (m_vBuffer[OFF_MAGIC] != DDS_MAGIC) {
		SAFE_ASSERT(false, "Invalid magic number in DDS header");
	}

	if (!(m_vBuffer[OFF_PFFLAGS] & DDPF_FOURCC)) {
		SAFE_ASSERT(false, "Unsupported format: Missing FourCC");
	}

	uint32_t fourCC = m_vBuffer[OFF_PF_FOURCC];
	uint32_t blockBytes;
	DDSFormat format;
	DXGI_FORMAT dxgiFormat;

	switch (fourCC) {
	case FOURCC_DXT1:
		blockBytes = 8;
		format = DDSFormat::DXT1;
		dxgiFormat = DXGI_FORMAT_R8G8B8A8_UINT;
		break;
	case FOURCC_DXT3:
		blockBytes = 16;
		format = DDSFormat::DXT3;
		dxgiFormat = DXGI_FORMAT_R8G8B8A8_UINT;
		break;
	case FOURCC_DXT5:
		blockBytes = 16;
		format = DDSFormat::DXT5;
		dxgiFormat = DXGI_FORMAT_R8G8B8A8_UINT;
		break;
	case FOURCC_ATI2:
		blockBytes = 16;
		format = DDSFormat::ATI2;
		dxgiFormat = DXGI_FORMAT_R8G8_UINT;
		break;
	default:

		SAFE_ASSERT(false, "Unsupported FourCC format: " + std::to_string(fourCC));
	}

	int flags = m_vBuffer[OFF_FLAGS];
	int mipmapCount = 1;
	if (flags & DDSD_MIPMAPCOUNT) {
		mipmapCount = std::max(1u, m_vBuffer[OFF_MIPMAPCOUNT]);
	}
	m_xInfo.MipLevelsCount = mipmapCount;

	int width = m_vBuffer[OFF_WIDTH];
	int height = m_vBuffer[OFF_HEIGHT];
	int dataOffset = m_vBuffer[OFF_SIZE] + 4;

	m_xInfo.Width = width;
	m_xInfo.Height = height;
	m_xInfo.Flags = flags;
	m_xInfo.Format = format;
	m_xInfo.DXGIFormat = dxgiFormat;

	const uint32_t caps2 = m_vBuffer[OFF_CAPS2];
	if (caps2 & DDSCAPS2_CUBEMAP) {
		m_xInfo.IsCubemap = true;

		int faceCount = 0;
		for (int i = 0; i < 6; ++i) {
			if (caps2 & (DDSCAPS2_CUBEMAP_POSITIVEX << i)) {
				faceCount++;
			}
		}

		for (int face = 0; face < faceCount; ++face) {
			for (int mip = 0; mip < mipmapCount; ++mip) {
				int w = std::max(1, width >> mip);
				int h = std::max(1, height >> mip);
				int length = std::max(4, w) / 4 * std::max(4, h) / 4 * blockBytes;

				m_xInfo.Images.push_back({ dataOffset, length });
				dataOffset += length;
			}
		}
	}
	else {
		for (int i = 0; i < mipmapCount; ++i) {
			int w = std::max(1, width >> i);
			int h = std::max(1, height >> i);
			int length = std::max(4, w) / 4 * std::max(4, h) / 4 * blockBytes;

			m_xInfo.Images.push_back({ dataOffset, length });
			dataOffset += length;
		}
	}
}

void DDSParser::DecodeDxt1(const uint8_t* src, int width, int height, std::vector<uint8_t>& dst) {
	int blockCountX = width / 4;
	int blockCountY = height / 4;

	for (int by = 0; by < blockCountY; ++by) {
		for (int bx = 0; bx < blockCountX; ++bx) {
			int i = 8 * (by * blockCountX + bx);
			uint16_t color0 = src[i] | (src[i + 1] << 8);
			uint16_t color1 = src[i + 2] | (src[i + 3] << 8);

			uint8_t colors[4][4];

			auto unpackColor = [](uint16_t c, uint8_t* rgb) {
				rgb[0] = convertBitRange(5, 8, (c >> 11) & 0x1F);
				rgb[1] = convertBitRange(6, 8, (c >> 5) & 0x3F);
				rgb[2] = convertBitRange(5, 8, c & 0x1F);
				rgb[3] = 255;
				};

			unpackColor(color0, colors[0]);
			unpackColor(color1, colors[1]);

			if (color0 > color1) {
				for (int c = 0; c < 3; ++c) {
					colors[2][c] = (2 * colors[0][c] + colors[1][c]) / 3;
					colors[3][c] = (colors[0][c] + 2 * colors[1][c]) / 3;
				}
				colors[2][3] = 255;
				colors[3][3] = 255;
			}
			else {
				for (int c = 0; c < 3; ++c)
					colors[2][c] = (colors[0][c] + colors[1][c]) / 2;
				colors[2][3] = 255;
				colors[3][0] = colors[3][1] = colors[3][2] = 0;
				colors[3][3] = 0;
			}

			uint32_t code = src[i + 4] | (src[i + 5] << 8) | (src[i + 6] << 16) | (src[i + 7] << 24);

			for (int row = 0; row < 4; ++row) {
				for (int col = 0; col < 4; ++col) {
					int pixelIndex = ((by * 4 + row) * width + (bx * 4 + col)) * 4;
					uint8_t colorIdx = (code >> (2 * (4 * row + col))) & 0x03;

					for (int c = 0; c < 4; ++c)
						dst[pixelIndex + c] = colors[colorIdx][c];
				}
			}
		}
	}
}

void DDSParser::DecodeDxt3(const uint8_t* src, int width, int height, std::vector<uint8_t>& dst) {
	int rowBytes = width * 4;
	int blockWidth = width / 4;
	int blockHeight = height / 4;

	for (int by = 0; by < blockHeight; ++by) {
		for (int bx = 0; bx < blockWidth; ++bx) {
			int i = 16 * (by * blockWidth + bx);

			uint16_t color0 = src[i + 8] | (src[i + 9] << 8);
			uint16_t color1 = src[i + 10] | (src[i + 11] << 8);
			DXT3Colors(m_aDXT3colors, color0, color1);

			int dstI = (by * 16) * width + bx * 16;

			for (int row = 0; row < 4; ++row) {
				uint16_t alphaBits = src[i + row * 2] | (src[i + 1 + row * 2] << 8);
				uint8_t colorBits = src[i + 12 + row];

				for (int column = 0; column < 4; ++column) {
					int dstIndex = dstI + column * 4;
					int colorIndex = ((colorBits >> (column * 2)) & 3) * 3;

					dst[dstIndex + 0] = m_aDXT3colors[colorIndex + 0];
					dst[dstIndex + 1] = m_aDXT3colors[colorIndex + 1];
					dst[dstIndex + 2] = m_aDXT3colors[colorIndex + 2];
					dst[dstIndex + 3] = ((alphaBits >> (column * 4)) & 0xf) * 17;
				}

				dstI += rowBytes;
			}
		}
	}
}

void DDSParser::DecodeDxt5(const uint8_t* src, int width, int height, std::vector<uint8_t>& dst) {
	int rowBytes = width * 4;
	int blockWidth = width / 4;
	int blockHeight = height / 4;

	for (int by = 0; by < blockHeight; ++by) {
		for (int bx = 0; bx < blockWidth; ++bx) {
			int i = 16 * (by * blockWidth + bx);

			DXT5Alphas(m_aDXT5alphas, src[i], src[i + 1]);
			uint16_t color0 = src[i + 8] | (src[i + 9] << 8);
			uint16_t color1 = src[i + 10] | (src[i + 11] << 8);
			DXT5Alphas(m_aDXT3colors, color0, color1);

			int dstI = (by * 16) * width + bx * 16;

			for (int block = 0; block < 2; ++block) {
				int alphaOffset = i + 2 + block * 3;
				int colorOffset = i + 12 + block * 2;

				uint32_t alphaBits = src[alphaOffset] | (src[alphaOffset + 1] << 8) | (src[alphaOffset + 2] << 16);

				for (int row = 0; row < 2; ++row) {
					uint8_t colorBits = src[colorOffset + row];

					for (int column = 0; column < 4; ++column) {
						int dstIndex = dstI + column * 4;
						int colorIndex = ((colorBits >> (column * 2)) & 3) * 3;
						int alphaIndex = (alphaBits >> (row * 12 + column * 3)) & 7;

						dst[dstIndex + 0] = m_aDXT3colors[colorIndex + 0];
						dst[dstIndex + 1] = m_aDXT3colors[colorIndex + 1];
						dst[dstIndex + 2] = m_aDXT3colors[colorIndex + 2];
						dst[dstIndex + 3] = m_aDXT5alphas[alphaIndex];
					}

					dstI += rowBytes;
				}
			}
		}
	}
}

void DDSParser::DecodeAti2(const uint8_t* src, int width, int height, std::vector<uint8_t>& dst) {
	int rowBytes = width * 4;
	int blockWidth = width / 4;
	int blockHeight = height / 4;

	for (int by = 0; by < blockHeight; ++by) {
		for (int bx = 0; bx < blockWidth; ++bx) {
			int i = 16 * (by * blockWidth + bx);

			RGColors(m_aRed, src[i], src[i + 1]);
			RGColors(m_aGreen, src[i + 8], src[i + 9]);

			int dstI = (by * 16) * width + bx * 16;

			for (int block = 0; block < 2; ++block) {
				int redOffset = i + 2 + block * 3;
				int greenOffset = i + 10 + block * 3;

				uint32_t redBits = src[redOffset] | (src[redOffset + 1] << 8) | (src[redOffset + 2] << 16);
				uint32_t greenBits = src[greenOffset] | (src[greenOffset + 1] << 8) | (src[greenOffset + 2] << 16);

				for (int row = 0; row < 2; ++row) {
					for (int column = 0; column < 4; ++column) {
						int pixel = dstI + column * 4;
						int shift = 3 * (row * 4 + column);
						dst[pixel + 0] = m_aRed[(redBits >> shift) & 7];
						dst[pixel + 1] = m_aGreen[(greenBits >> shift) & 7];
						dst[pixel + 2] = 0;
						dst[pixel + 3] = 255;
					}
					dstI += rowBytes;
				}
			}
		}
	}
}
