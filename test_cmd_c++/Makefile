#change dir according to your env.
GTEST_DIR = /home/tmori/downloads/gtest-1.4.0

# HERE add your test modules
TEST_MODULES = parser_unittest.o

#dont change
USER_DIR = ../cmd_c++
TEST_DIR = .
lib_LTLIBRARIES=$(GTEST_DIR)/lib/.libs/
CPPFLAGS += -I$(GTEST_DIR) -I$(GTEST_DIR)/include -I$(USER_DIR) -I../include
CXXFLAGS += -g -Wall -Wextra
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)
TESTS = unittest 

.cc.o : 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

all : $(TESTS)

run : $(TESTS)
	sh $(TEST_DIR)/runTest.sh $(TEST_DIR)/$(TESTS)


clean :
	(cd $(USER_DIR); make clean)
	rm -f $(TESTS) *.o *.a

user_modules.a : $(USER_DIR)/*.cc $(USER_DIR)/*.h
	(cd $(USER_DIR); make)
	$(AR) $(ARFLAGS) $@ $(USER_DIR)/*.o

unittest : user_modules.a main.o $(TEST_MODULES)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(TEST_MODULES) user_modules.a main.o -o $@ -L$(lib_LTLIBRARIES) -lgtest 


