/**
 * Entry point for the emulator; this parses command line parameters and sets up
 * the state of the rest of the emulator.
 */
#include <unistd.h>

#include <iostream>
#include <string>

#include <glog/logging.h>

#include "Emulator.h"


static void SetUpLogging(int argc, char const *argv[]);

static int ParseCommandLine(int argc, char const *argv[]);
static void PrintUsage(const char *binName);

/**
 * File paths and whatnot
 */
static struct {
	// location of rom file
	std::string romPath = "rom.bin";
	// location of NVRAM file
	std::string nvramPath = "nvram.bin";
} gState;


/**
 * Entry point
 */
int main(int argc, char const *argv[]) {
	int err;

	// parse command line options; exit if returns less than 1
	err = ParseCommandLine(argc, argv);

	if(err <= 0) {
		return err;
	}

	// early setup
	SetUpLogging(argc, argv);


	// set up CPU emulation
	Emulator *emu = new Emulator(gState.romPath, gState.nvramPath);

	// start
	emu->start();

	// clean up
	delete emu;

  return 0;
}



/**
 * Sets up logging.
 */
static void SetUpLogging(int argc, char const *argv[]) {
	// set up logging
	FLAGS_logtostderr = 1;
	FLAGS_colorlogtostderr = 1;

#if DEBUG
	FLAGS_v = 2;
#else
	FLAGS_v = 0;
#endif

	google::InitGoogleLogging(argv[0]);
	google::InstallFailureSignalHandler();

	LOG(INFO) << "nixieclock_emu " << GIT_HASH << "/" << GIT_BRANCH
			  << " compiled on " << COMPILE_TIME;
}

/**
 * Parses the command line.
 */
static int ParseCommandLine(int argc, char const *argv[]) {
	int c;

	while((c = getopt(argc, const_cast<char **>(argv), "hr:n:")) != -1) {
		switch(c) {
			case 'h':
				PrintUsage(argv[0]);
				return 0;

				// boot rom file
				case 'r':
					gState.romPath = std::string(optarg);
					break;

				// nvram shadow file
				case 'n':
					gState.nvramPath = std::string(optarg);
					break;

				// something went wrong
				case '?':
				// case ':':
					return -1;
			}
	}

	// assume success
	return 1;
}

/**
 * Prints the usage instructions.
 */
static void PrintUsage(const char *binName) {
	// print to cout, not log
	std::cout << "usage: " << binName << "[-r rom] [-n nvram] -h" << std::endl;
	std::cout << "\t-r: Path to boot ROM file" << std::endl;
	std::cout << "\t-n: Path to NVRAM file" << std::endl;
	std::cout << "\t-h: Displays help on using this program" << std::endl;
	std::cout << std::endl;
	std::cout << "git version " << GIT_HASH << "/" << GIT_BRANCH << std::endl;
}
