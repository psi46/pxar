.PHONY: clean cleanall

########
# OBJS #
########
OBJS = rpcgen.o datatype.o file.o cpp_error.o cpp_parser.o cpp_scanner.o

#########
# FLAGS #
#########
CXXFLGAS = -g -Wall -fPIC -pipe -Os

#################
# PATTERN RULES #
#################
%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

###################
# Main executable #
###################
rpcgen: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)
	@true

clean:
	rm -rf $(OBJS)

cleanall: clean
	rm -rf rpcgen
