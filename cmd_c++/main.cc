#include "tarfs.h"

using namespace Tarfs;
int
main(int argc, const char* argv[])
{
	FsMaker *fsmaker;
	Parser *parser;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s tarfile\n", argv[0]);
		return 1;
	}
	parser = Parser::create((char*)argv[1]);
	if (parser == NULL) {
		fprintf(stderr, "err paser.init()\n");
		return 1;
	}
	fsmaker = FsMaker::create((char*)argv[1]);
	if (fsmaker == NULL) {
		return 0;
	}

	File *file;
	while ((file = parser->get())) {
		fsmaker->add(file);
		delete file;
	}
	fsmaker->complete();
	delete parser;
	return 0;
}
