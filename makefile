all: test_server test_client
test_server: test_server.cpp server.cpp server.h global.h
	$(CXX) $^ -o $@  -lpthread -lmysqlclient -lhiredis 
test_client: test_client.cpp client.cpp client.h global.h
	$(CXX) $^ -o $@  -lpthread -lmysqlclient -lhiredis
clean:
	rm test_server
	rm test_client