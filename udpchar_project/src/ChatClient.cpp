#include <unistd.h>
#include "ChatClient.hpp"
#include "ChatWindows.hpp"

void Menu()
{
    cout<<"*******************************"<<endl;
    cout<<"*****1. register *** 2. login**"<<endl;
    cout<<"*****3. logout   *** 4. exit***"<<endl;
    cout<<"*******************************"<<endl;
}
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


    UdpClient* uc = new UdpClient(ip);
    ChatWindow* cw = new ChatWindow();

    while(1)
    {
        Menu();
        int select = -1;
        cout << "please  enter your select: ";
        fflush(stdout);

        cin >> select;
        if(select == 1)
        {
            // 注册
            int ret = uc->RegistertoSvr();
            if(ret < 0)
            {
                LOG(WARNING, "please retry register")<<endl;
            }
            else if(ret == 0)
            {
                LOG(INFO, "register success ! please login...")<<endl;
            }
            uc->CloseFd();
        }
        else if(select == 2)
        {
            // 登录
            int ret = uc->LoginToSvr();
            if(ret < 0)
            {
                LOG(ERROR, "please retry login")<<endl;
            }
            else if(ret == 0)
            {
                LOG(INFO, "login success, please chatting...")<<endl;

                cw->start(uc);
            }
            
        }
        else if(select == 3)
        {
            // 登出
        }
        else if(select == 4)
        {
            //退出
            LOG(INFO,"exit chat client")<<endl;
            exit(0);
        }

    }


    while(1)
    {
        sleep(1);
    }
    return 0;
}
