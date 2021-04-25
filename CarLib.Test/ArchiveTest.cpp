#include <sstream>
#include "gtest/gtest.h"
#include "CarLib/Archive.h"

struct ArchiveFixture {
	std::stringstream stream;
	std::ostream& outputStream;
	std::istream& inputStream;
	Car::Archive archive;

	ArchiveFixture() : outputStream{stream}, inputStream{stream}, archive{outputStream} {}
};

TEST(ArchiveTest, emptyArchiveContainsNoFiles) {
	ArchiveFixture fixture;
	EXPECT_EQ(fixture.archive.getFileCount(), 0);
}

TEST(ArchiveTest, fileCountIncreasesAsFilesAreAdded) {
	ArchiveFixture fixture;
	std::stringstream emptyStream;
	fixture.archive.addFile("filename.txt", emptyStream);
	fixture.archive.addFile("filename2.txt", emptyStream);
	EXPECT_EQ(fixture.archive.getFileCount(), 2);
}

TEST(ArchiveTest, constructionAddsSomethingToOutputStream) {
	ArchiveFixture fixture;
	const std::string writtenBytes = fixture.stream.str();
	EXPECT_GT(writtenBytes.size(), 0);
}

TEST(ArchiveTest, readingBackNumberOfFiles) {
	/* Honestly, this is more of a thing we have due to TDD
	 * rather than a useful feature. Whatever.
	 */
	std::stringstream stream;

	std::ostream& outputStream = stream;
	Car::Archive archive1{outputStream};
	std::stringstream emptyStream;
	archive1.addFile("filename.txt", emptyStream);
	archive1.addFile("filename2.txt", emptyStream);

	std::istream& inputStream = stream;
	Car::Archive archive2{inputStream};
	EXPECT_EQ(archive2.getFileCount(), 2);
}

TEST(ArchiveTest, ifHeaderIsInvalidConstructorThrows) {
	/* Test that if the header is malformed (here CArv1 vs CARv1),
	 * we should loudly complain.
	 */
	std::stringstream stream;
	stream << "CArv1";
	std::istream& inputStream = stream;
	EXPECT_THROW((Car::Archive{inputStream}), std::runtime_error);
}

TEST(ArchiveTest, canRetrieveFilesToMemory) {
	/* Reading a file entirely into memory.
	 * We might want to enable some kind of streaming, but
	 * probably not.
	 */
	ArchiveFixture fixture;
	std::stringstream stream1, stream2;
	stream1 << "hello, world";
	stream2 << "hello, world";
	fixture.archive.addFile("h", stream1);
	fixture.archive.addFile("hello", stream2);

	const Car::Archive archive{fixture.inputStream};
	const std::string hello = archive.getAsString("hello");
	EXPECT_EQ(hello, "hello, world");
}

TEST(ArchiveTest, filesAreCompressed) {
	ArchiveFixture fixture;
	std::stringstream stream;
	for (int i = 0; i < 1024; ++i) {
		stream << 'a';
	}
	/* Assuming that zlib can figure out how to compress 1024 letters
	 * in a row shouldn't be too much of an ask.
	 */
	fixture.archive.addFile("a file name", stream);

	const std::string result = fixture.stream.str();
	EXPECT_LT(result.size(), 1024);
}
