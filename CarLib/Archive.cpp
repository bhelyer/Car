#include "CarLib/Archive.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include "zlib.h"

namespace {
/* All archives start with this. */
const std::string magicWord  = "CAR";
/* The version of this archive format. */
const std::string carVersion = "v1";

/* Write a value as a binary to the given output stream. */
template<typename T>
void writeBinary(std::ostream& outputStream, const T& value) {
	outputStream.write((char*)&value, sizeof(value));
}

/* Read a value as a binary from the given input stream. */
template<typename T>
[[nodiscard]] T readBinary(std::istream& inputStream) {
	T value;
	inputStream.read((char*)&value, sizeof(T));
	if (!inputStream) {
		throw std::runtime_error{"readBinary: read failure."};
	}
	return value;
}

/* Read a string encoded as <length> <bytes>. */
[[nodiscard]] std::string readString(std::istream& inputStream);

/* Read the contents of an input stream into a vector. */
[[nodiscard]] std::vector<uint8_t> readBytes(std::istream& inputStream);

/* Encrypt / decrypt a string with trivial XOR encryption. */
[[nodiscard]] std::string xorEncryptDecrypt(const std::string& str);

}

Car::Archive::Archive(std::ostream& outputStream) : mOutputStream{&outputStream} {
	outputStream.write(magicWord.c_str(), magicWord.size());
	outputStream.write(carVersion.c_str(), carVersion.size());
}

Car::Archive::Archive(std::istream& inputStream) : mInputStream{&inputStream} {
	verifyHeader(inputStream);
}

int Car::Archive::getFileCount() const noexcept {
	if (mOutputStream != nullptr) {
		/* We're writing, so just return the number of times addFile was called. */
		return mFileCount;
	}

	/* This is a read-only archive, so check the archive for the number of files. */

	int fileCount = 0;
	assert(mInputStream != nullptr && "Archive::getFileCount: input stream AND output stream is null: shouldn't happen.");
	mInputStream->clear();
	mInputStream->seekg(0);
	verifyHeader(*mInputStream);
	while (*mInputStream) {
		try {
			(void)readString(*mInputStream);
		} catch (const std::runtime_error&) {
			break;
		}
		(void)readBinary<size_t>(*mInputStream);  /* Ignore the uncompressed size. */
		const size_t length = readBinary<size_t>(*mInputStream);
		mInputStream->seekg(length, std::ios::cur);
		++fileCount;
	}
	return fileCount;
}

void Car::Archive::addFile(const std::string& filename, std::istream& inputStream) {
	if (mOutputStream == nullptr) {
		throw std::logic_error{"Archive::addFile: Tried to write to read-only Archive."};
	}
	const std::string encryptedFilename = xorEncryptDecrypt(filename);
	writeBinary(*mOutputStream, encryptedFilename.size());
	mOutputStream->write(encryptedFilename.c_str(), encryptedFilename.size());

	/* Read the stream into memory.
	 * If this ever needs to handle big files, you should probably
	 * stream bytes in a chunk at a time.
	 * (If this needs to handle big files, you should probably
	 * be using a different format!)
	 */
	const std::vector<uint8_t> uncompressedBytes = readBytes(inputStream);
	const Bytef* src = uncompressedBytes.data();
	const auto srcLen = static_cast<uLong>(uncompressedBytes.size());

	/* Get a buffer big enough to hold a compressed uncompressedBytes. */
	uLong dstLen = compressBound(srcLen);
	std::vector<Bytef> compressedBytes(dstLen);
	Bytef* dst = compressedBytes.data();

	/* Call into zlib to actually compress the buffer. */
	if (compress(dst, &dstLen, src, srcLen) != Z_OK) {
		throw std::runtime_error{"Archive::addFile: compression failure."};
	}
	compressedBytes.resize(dstLen); /* compress() modifies destLen to the size of the actual compressed data. */

	/* We need to know the uncompressed size, so store that first. */
	writeBinary(*mOutputStream, uncompressedBytes.size());
	/* Now store the compressed data block. */
	writeBinary(*mOutputStream, compressedBytes.size());
	mOutputStream->write((char*)compressedBytes.data(), compressedBytes.size());
	++mFileCount;
}

std::string Car::Archive::getAsString(const std::string& filename) const {
	if (mInputStream == nullptr) {
		throw std::logic_error{"Archive::getAsString: Tried to read from write-only Archive."};
	}

	mInputStream->clear();
	mInputStream->seekg(0);
	verifyHeader(*mInputStream);

	while (*mInputStream) {
		const std::string currentFilename = xorEncryptDecrypt(readString(*mInputStream));
		auto uncompressedSize = static_cast<uLong>(readBinary<size_t>(*mInputStream));
		const auto compressedSize = static_cast<uLong>(readBinary<size_t>(*mInputStream));
		if (currentFilename == filename) {
			std::vector<Bytef> compressedData(compressedSize);
			mInputStream->read((char*)compressedData.data(), compressedSize);

			std::string str(uncompressedSize, 0);

			if (uncompress((Bytef*)str.data(), &uncompressedSize, compressedData.data(), compressedSize) != Z_OK) {
				throw std::runtime_error{"Archive::getAsString: decompression failure."};
			}

			return str;
		} else {
			mInputStream->seekg(compressedSize, std::ios_base::cur);
		}
	}

	throw std::runtime_error{"getAsString: Could not read filename."};
}

void Car::Archive::verifyHeader(std::istream& inputStream) const {
	char word[4]; word[3] = 0;
	char version[3]; version[2] = 0;
	inputStream.read(word, magicWord.size());
	inputStream.read(version, carVersion.size());
	if (!inputStream) {
		throw std::runtime_error{"Archive::verifyHeader: Unexpected EOF when reading archive."};
	}

	const std::string gotWord = word;
	const std::string gotVersion = version;

	if (gotWord != magicWord) {
		throw std::runtime_error{"Archive::verifyHeader: Input does not appear to be a CAR file."};
	}
	if (gotVersion != carVersion) {
		throw std::runtime_error{"Archive::verifyHeader: CAR file version is unsupported."};
	}
}

namespace {

std::vector<uint8_t> readBytes(std::istream& inputStream) {
	std::vector<uint8_t> result;
	std::array<uint8_t, 512> buf;
	while (inputStream.read((char*)buf.data(), buf.size())) {
		result.insert(result.end(), buf.data(), buf.data() + buf.size());
	}
	result.insert(result.end(), buf.data(), buf.data() + inputStream.gcount());
	return result;
}

std::string readString(std::istream& inputStream) {
	const size_t length = readBinary<size_t>(inputStream);
	std::string str;
	str.resize(length, 0);
	inputStream.read(str.data(), length);
	if (!inputStream) {
		throw std::runtime_error{"readString: unexpected EOF."};
	}
	return str;
}

std::string xorEncryptDecrypt(const std::string& str) {
	std::string output = str;
	for (size_t i = 0; i < output.size(); ++i) {
		output[i] = str[i] ^ 'X';  /* "Painstakingly developed!" */
	}
	return output;
}

}