TARGET=main
OBJS:= $(patsubst %.cpp,%.o,$(wildcard ./*.cpp))	#当前路径的.cpp转换为.o

LDFLAGS:= -L/usr/lib/mymuduo
LIBS:= -lmymuduo -lpthread
CXXFLAGS:= -std=c++11 -w -fPIC -g

$(TARGET):$(OBJS)
# 	echo $(OBJS)
	-$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LIBS) -o ./build/$(TARGET)
	-@$(RM) $(OBJS)
	$(shell export LD_LIBRARY_PATH=/usr/lib/mymuduo:$$LD_LIBRARY_PATH)

clean:
	@$(RM) ./build/$(TARGET) $(OBJS)
.PHONY:clean