CXXFLAGS := -std=c++20

main: motion.cc
	$(CXX) $(CXXFLAGS) -o $@ $<;
