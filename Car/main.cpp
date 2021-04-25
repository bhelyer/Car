#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "CarLib/Archive.h"
namespace fs = std::filesystem;

int main(int argc, const char** argv) {
	if (argc == 1) {
		std::cerr << "usage: car [-o=filename] [files]\n";
		return 0;
	}

	std::string outputName = "output.car";
	std::vector<std::string> filenames;
	filenames.reserve(argc);
	for (int i = 1; i < argc; ++i) {
		const std::string argument = argv[i];
		if (argument.size() > 3 && argument.substr(0, 3) == "-o=") {
			outputName = argument.substr(3); /* -o=[name] */
		} else {
			filenames.push_back(argument);
		}
	}

	std::cerr << "Writing to archive '" << outputName << "'.\n\n";
	std::ofstream ofs{outputName, std::ios_base::out | std::ios_base::binary};
	Car::Archive archive{ofs};

	for (const std::string filename : filenames) {
		const fs::path filepath{filename};
		if (fs::is_regular_file(filepath)) {
			std::cerr << "Archiving '" << filename << "'.\n";
			std::ifstream ifs{filename, std::ios_base::in | std::ios_base::binary};
			archive.addFile(filepath.filename().string(), ifs);
		} else {
			std::cerr << "Skipping non-file '" << filename << "'.\n";
		}
	}
	return 0;
}