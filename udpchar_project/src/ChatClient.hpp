#pragma once
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "ConnectInfo.hpp"
#include "tools.hpp"

struct MySelf
{
    string nick_name_;
    string school_;
    string passwd_;
    uint32_t user_id;
};

class UdpClient
{
    public:
        UdpClient()
        {
            tcp_sock_ = -1;
        }
        ~UdpClient()
        {

        }

        int CreateSock()
        {
            tcp_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(tcp_sock_ < 0)
            {
                LOG(ERROR, "create socket failed")<<endl;
                return -1;
            }

            // 为了客户端可以在同一台机器上多开，我们不主动绑定端口
            // 让操作系统进行绑定
            return 0;
        }

        //ip是服务端的ip地址
        //服务端的ip地址已经约定好了，在ConnectInfo这个文件当中
        int ConnectoSvr(const string& ip)
        {
            struct sockaddr_in dest_addr;
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(TCP_PORT);
            dest_addr.sin_addr.s_addr = inet_addr(ip.c_str());

            int ret = connect(tcp_sock_,(struct sockaddr*)&dest_addr,sizeof(dest_addr));
            if(ret < 0)
            {
                LOG(ERROR,"Connect server failed, addr is ")<<ip<<":"<<TCP_PORT<<endl;
                return -1;
            }

            return 0;
        }

        int RegistertoSvr(const string& ip)
        {
            // 创建套接字
            int ret = CreateSock();
            if(ret < 0)
            {
                return -1;
            }
            // 连接服务端
            ret = ConnectoSvr(ip);
            if(ret < 0)
            {
                return -1;
            }
            // 准备注册包
            char type = REGISTER_RESQ;
            ssize_t send_size = send(tcp_sock_, &type, 1, 0);
            if(send_size < 0)
            {
                return -1;
            }

            struct RegisterInfo ri;
            cout<<"please enter nick_name: ";
            fflush(stdout);
            cin >> ri.nick_name_;
            me_.nick_name_ = ri.nick_name_;
            cout << "please enter school: ";
            fflush(stdout);
            cin >> ri.school_;
            me_.school_ = ri.school_;
            // 对于密码字段，我们需要进行一个双重校验，防止用户在输入密码的时候，“手心”不一致
            while(1)
            {
                string first_passwd;
                string second_passwd;
                cout << "please enter your passwd: ";
                fflush(stdout);
                cin >> first_passwd;
                cout<< "please retry enter your passwd: ";
                fflush(stdout);
                cin >> second_passwd;

                if(first_passwd == second_passwd)
                {
                    strncpy(ri.passwd_, first_passwd.c_str(),sizeof(ri.passwd_));
                    me_.passwd_ = first_passwd;
                    break;
                }
            }

            // 发送注册包
            send_size = send(tcp_sock_, &ri, sizeof(ri), 0);
            if(send_size < 0)
            {
                LOG(ERROR, "Send register failed ")<<endl;
                return -1;
            }
            // 接收应答
            struct RelpyInfo reply_info;
            size_t recv_size = recv(tcp_sock_, &reply_info, sizeof(reply_info), 0);
            if(recv_size < 0)
            {
                LOG(ERROR, "recv register response failed")<<endl;
                return -1;
            }
            else if(recv_size == 0)
            {
                LOG(ERROR, "udpchat server shutdown connect")<<endl;
                CloseFd();
                return -1;
            }
            // 判断应答结果
            if(reply_info.resp_status_ == REGISTER_FAILED)
            {
                LOG(ERROR, "server response register failed")<<endl;
                return -1;
            }
            // 返回给上层调用者注册的结果
            LOG(INFO, "register success")<<endl;
            me_.user_id = reply_info.id_;
            return 0;
        }

        int LoginToSvr(string& ip)
        {
            // 创建套接字
            int ret = CreateSock();
            if(ret < 0)
            {
                return -1;
            }
            // 连接服务端
            ret = ConnectoSvr(ip);
            if(ret < 0)
            {
                return -1;
            }
            // 发送类型
            char type = LOGIN_RESQ;
            ssize_t send_size = send(tcp_sock_, &type, 1, 0);

            if(send_size <0 )
            {
                LOG(ERROR,"send login packet failed")<<endl;
                return -1;
            }

            // 发送登陆包
            struct LoginInfo ri;
            ri.id_ = me_.user_id;
            strncpy(ri.passwd_, me_.passwd_.c_str(),sizeof(ri.passwd_));
            send_size = send(tcp_sock_, &ri, sizeof(ri), 0);
            if(send_size<0)
            {
                LOG(ERROR, "senf login packet failed")<<endl;
                return -1;
            }

            // 接收应答
            struct RelpyInfo reply_info;
            ssize_t recv_size = recv(tcp_sock_, &reply_info, sizeof(reply_info), 0);
            if(recv_size < 0)
            {
                LOG(ERROR, "recv failed")<<endl;
                return -1;
            }
            else if(recv_size == 0)
            {
                CloseFd();
                LOG(ERROR, "server shutdown connect")<<endl;
                return -1;
            }

            // 分析应答数据
            if(reply_info.resp_status_ != LOGIN_SUCCESS)
            {
                LOG(ERROR, "recv status not LOGIN_SUCCESS")<<endl;
                return -1;
            }
            LOG(INFO, "login success")<<endl;
            return 0;

        }

        void CloseFd()
        {
            if(tcp_sock_ > 0)
            {
                close(tcp_sock_);
                tcp_sock_ = -1;
            }
        }
    private:
        int tcp_sock_;

        MySelf me_;
};
