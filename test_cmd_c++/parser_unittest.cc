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
	parser = new Parser("sample1.tar");
	bool ret = parser->init();
	ASSERT_TRUE(ret == true);

	/* 1:REG */
	File *file = parser->get();
	ASSERT_TRUE(file != NULL);
	delete file;

	/* 2:DIR */
	file = parser->get();
	ASSERT_TRUE(file != NULL);
	delete file;

	/* 3:SYM */
	file = parser->get();
	ASSERT_TRUE(file != NULL);

	/* 4:NONE */
	file = parser->get();
	ASSERT_TRUE(file == NULL);

	delete parser;
}
