#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Lazy.h"
#include "syncWait.h"
#include "Socket.h"

using namespace async_simple::coro;

using Texts = std::vector<std::string>;

Lazy<> echo(int cli)
{
    Socket client{ cli };
    char buffer[100] = { 0 };
    int len = 0;
    std::cout << "ReadCLI ..." << std::endl;
    client.read(buffer, 100);
    buffer[0] = 's';
    client.write(buffer, 100);
    co_await std::suspend_never{};
}

Lazy<int> AsyncAccept()
{

    InetAddress temp;
    std::cout << "ASYNCACC ..." << std::endl;
    Socket server{ Socket::createNonblockingSocketOrDie(AF_INET) };
    server.bindAddress({ 8888 });
    server.setReuseAddr(true);
    server.setReusePort(true);
    server.listen();
    while (true)
    {
        int cli = server.accept(&temp);
        std::cout << "acc :" << temp.toIpPort() << std::endl;
        co_await echo(cli);
    }
    
    co_return 3;
}

int main()
{
    int Num = syncAwait(AsyncAccept());
    std::cout << "The number of 'x' in file.txt is " << Num << "\n";
    return 0;
}