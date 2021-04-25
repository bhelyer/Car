#pragma once
#include <string>
#include <istream>
#include <ostream>

namespace Car {

/* A compressed archive of files. */
class Archive {
public:
	/* Write a new archive to the given stream. */
	explicit Archive(std::ostream& outputStream);

	/* Read an archive from the given stream. */
	explicit Archive(std::istream& inputStream);

public:
	/* The number of files stored in this archive. */
	[[nodiscard]] int getFileCount() const noexcept;

	/* Add a new file to this archive. Filename must be unique. Output archive only. */
	void addFile(const std::string& filename, std::istream& inputStream);

	/* Get the given filename's contents as a string. Input archive only. */
	[[nodiscard]] std::string getAsString(const std::string& filename) const;

private:
	/* Throw if inputStream doesn't look like the start of a CAR file. */
	void verifyHeader(std::istream& inputStream) const;

private:
	/* Non owning pointer to the output stream. If this is null, this
	 * Archive is read only.
	 */
	std::ostream* mOutputStream = nullptr;

	/* Non owning pointer to the input stream. If this is null, this
	 * is a write archive.
	 */
	mutable std::istream* mInputStream = nullptr;

	/* A cache for when we're an output archive. */
	int mFileCount = 0;
};

}