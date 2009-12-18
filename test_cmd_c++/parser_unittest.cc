#include <tarfs.h>
#include <gtest/gtest.h>

using namespace Tarfs;

class ParserTest : public ::testing::Test {
	public:
	protected:
	virtual void SetUp() {
	}
	virtual void TearDown() {
	}
};
/** sample1.tar 
 * ftype:filesize
 * REG  :0
 * DIR  :0
 * SYM  :0
 */
TEST_F(ParserTest, sample1_tar_REG_DIR_SYM)
{
	Parser *parser;
	parser = Parser::create("sample1.tar");
	ASSERT_TRUE(parser != NULL);
	/* 1:REG */
	File *file = parser->get();
	ASSERT_TRUE(file != NULL);
	ASSERT_TRUE(file->getFtype() == TARFS_IFREG);
	delete file;

	/* 2:DIR */
	file = parser->get();
	ASSERT_TRUE(file != NULL);
	ASSERT_TRUE(file->getFtype() == TARFS_IFDIR);
	delete file;

	/* 3:SYM */
	file = parser->get();
	ASSERT_TRUE(file != NULL);
	ASSERT_TRUE(file->getFtype() == TARFS_IFLNK);

	/* 4:NONE */
	file = parser->get();
	ASSERT_TRUE(file == NULL);

	delete parser;
}

TEST_F(ParserTest, parsePath)
{
	const char *org = "abc";
	char path[MAXPATHLEN];
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	std::vector<char*> *names = Parser::parsePath(path);
	ASSERT_TRUE(names->size() == 1);
	ASSERT_TRUE(strlen(names->at(0)) == strlen(path));
	ASSERT_TRUE(!memcmp(names->at(0), path, strlen(path)));
	names->clear();
	delete names;

	org = "/abc/";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names != NULL);
	ASSERT_TRUE(names->size() == 1);
	ASSERT_TRUE(strlen(names->at(0)) == strlen("abc"));
	ASSERT_TRUE(!memcmp(names->at(0), "abc", strlen("abc")));
	names->clear();
	delete names;

	org = "/abc/def";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names != NULL);
	ASSERT_TRUE(names->size() == 2);
	ASSERT_TRUE(strlen(names->at(0)) == strlen("abc"));
	ASSERT_TRUE(strlen(names->at(1)) == strlen("def"));
	ASSERT_TRUE(!memcmp(names->at(0), "abc", strlen("abc")));
	ASSERT_TRUE(!memcmp(names->at(1), "def", strlen("def")));
	names->clear();
	delete names;

	org = "/abc/def/";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names != NULL);
	ASSERT_TRUE(names->size() == 2);
	ASSERT_TRUE(strlen(names->at(0)) == strlen("abc"));
	ASSERT_TRUE(strlen(names->at(1)) == strlen("def"));
	ASSERT_TRUE(!memcmp(names->at(0), "abc", strlen("abc")));
	ASSERT_TRUE(!memcmp(names->at(1), "def", strlen("def")));
	names->clear();
	delete names;

	org = "/";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names == NULL);

	org = ".";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names == NULL);

	org = "./";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names == NULL);

	org = "..";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names == NULL);

	org = "../";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names == NULL);

	org = "abc/.";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names == NULL);

	org = "abc/..";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names == NULL);

	org = "abc/../aaa";
	memset(path, 0, MAXPATHLEN);
	memcpy(path, org, strlen(org));
	names = Parser::parsePath(path);
	ASSERT_TRUE(names == NULL);
}
TEST_F(ParserTest, many_tar_usr_include)
{
	File *file;
	Parser *parser;
	parser = Parser::create("manyFiles.tar");
	ASSERT_TRUE(parser != NULL);
	while ((file = parser->get())) {
		file->printPath();
		delete file;
	}
	delete parser;
}
TEST_F(ParserTest, longFileName)
{
	File *file;
	Parser *parser;
	parser = Parser::create("longFileName.tar");
	ASSERT_TRUE(parser != NULL);
	while ((file = parser->get())) {
		file->printPath();
		delete file;
	}
	delete parser;
}

