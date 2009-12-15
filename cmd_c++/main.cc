#include "tarfs.h"

using namespace Tarfs;
int
main(int argc, const char* argv[])
{
	FsMaker *maker;
	Parser *parser;
	if (argc != 2) {
		fprintf(stderr, "Usage: %s tarfile\n", argv[0]);
		return 1;
	}
	parser = new Parser((char*)argv[1]);
	bool ret = parser->init();
	if (ret == false) {
		fprintf(stderr, "err paser.init()\n");
		return 1;
	}
	maker = new FsMaker((char*)argv[1]);
	int err = maker->init();
	if (err < 0) {
		fprintf(stderr, "err maker.init()\n");
		return 1;
	} else if (err == EEXIST) {
		return 0;
	}

	File *file;
	while ((file = parser->get())) {
		ret = file->parse();
		if (ret == false) {
			delete file;
			break;
		}
		maker->add(file);
		delete file;
	}
	delete parser;
	return 0;
}
