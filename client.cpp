#include <iostream>
#include <Socket.h>
#include <string>
int main()
{
    Socket cli{ Socket::createNonblockingSocketOrDie(AF_INET) };
    char buffer[100] = { 0 };
    for (int i = 0;i < 10;i++)
    {
        buffer[i] = 'c';
    }
    Socket::connect(cli.fd(),{8888});
    cli.write(buffer, 100);
    cli.read(buffer, 100);
    std::cout << std::string(buffer) << std::endl;
    return 0;
}