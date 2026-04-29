#include "net/TcpServer.h"
#include "engine/MatchingEngine.h"

int main() {
    MatchingEngine engine;
    engine.start();

    TcpServer server(8888, &engine);
    server.start();
}