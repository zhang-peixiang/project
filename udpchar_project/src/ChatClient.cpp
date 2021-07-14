#include <unistd.h>
#include "ChatClient.hpp"

int main(int argc, char* argv[])
{
    if(argc != 3)
    {
        // ./ChatClient -ip [ip]
        cout << "using ./ChatClient [ip]" <<endl;
        return -1;
    }

    string ip;
    for(int i = 0;i < argc;i++)
    {
        if(strcmp(argv[i], "-ip") == 0 && (i+1) < argc)
        {
            ip = argv[i+1];
        }
    }

    if(ip.size() == 0)
    {
        LOG(ERROR, "Illegal IP, please retry start ChatClent")<<endl;
    }

    UdpClient uc;
    uc.RegistertoSvr(ip);

    while(1)
    {
        sleep(1);
    }
    return 0;
}
