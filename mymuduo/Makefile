TARGET=libmymuduo.so
# TARGET=server
OBJS:= $(patsubst %.cpp,%.o,$(wildcard ./*.cpp))	#当前路径的.cpp转换为.o

LDFLAGS:= -shared
LIBS:= -lpthread
CXXFLAGS:= -std=c++11 -w -fPIC -g


$(TARGET):$(OBJS)
# 	echo $(OBJS)
	-$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o ./build/$(TARGET)
	-@$(RM) $(OBJS)

clean:
	@$(RM) ./build/$(TARGET) $(OBJS)
.PHONY:clean