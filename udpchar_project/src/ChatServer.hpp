#pragma once
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>

#include "ConnectInfo.hpp"

#define TCP_PORT 17878

class TcpConnect
{
    public:
        TcpConnect()
        {
            new_sock_ = -1;
        }

        ~TcpConnect()
        {

        }

        void SetSockFd(int fd)
        {
            new_sock_ = fd;
        }

        int GetSockFd()
        {
            return new_sock_;
        }
    private:
        int new_sock_;
};

class ChatServer
{
    public:
        ChatServer()
        {
            // 登陆注册
            tcp_sock_ = -1;
            udp_sock_ = -1;
            tcp_port_ = TCP_PORT;
        }

        ~ChatServer()
        {

        }

        // 初始化变量的接口，被调用者调用
        // 用户管理模块的实例化对象，消息池的实例化对象
        int InitSvr(uint16_t tcp_port = TCP_PORT)
        {
            // 1. 创建tcp_socket，并且绑定地址信息，监听
            // 注册+登陆
            tcp_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if(tcp_sock_ < 0)
            {
                return -1;
            }

            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(tcp_port);
            addr.sin_addr.s_addr = inet_addr("0.0.0.0");
            int ret = bind(tcp_sock_, (struct sockaddr*)&addr, sizeof(addr));
            if(ret < 0)
            {
                return -2;
            }

            // 监听
            ret = listen(tcp_sock_, 5);
            if(ret < 0)
            {
                return -3;
            }

            // 暂时还没有考虑的是udp通信，以及登陆注册模块，消息池的初始化
            //
            return 0;
        }

        // 启动线程
        int Start()
        {
            // udp应该有两类线程
            // 1类： 生产线程，负责接收udp数据，将udp数据放到消息池当中
            // 2类： 消费线程，负责从消息池当中获取消息，发送到在线用户的客户端
            //
            // tcp
            //   针对每一个注册登陆请求（tcp连接），针对每一个tcp连接都创建一个线程，专门为该客户端处理注册和登陆请求。注册和登陆完毕后，服务端关闭连接，销毁线程
            //
            //   tcp是否创建线程，取决于accept函数是否调用成功（阻塞）
            //
            //   TODO
            //   udp 线程的创建
            //
            struct sockaddr_in peer_addr;
            socklen_t peer_addrlen = sizeof(peer_addr);
            while(1)
            {
                int new_sock = accept(tcp_sock_,(struct sockaddr*)&peer_addr, &peer_addrlen);
                if(new_sock < 0)
                {
                    continue;
                }
                
                // 正常接收到了
                // 创建线程，为客户端的注册和登陆请求负责
                TcpConnect* tc = new TcpConnect();
                tc->SetSockFd(new_sock);

                pthread_t tid;
                int ret = pthread_create(&tid, NULL, LoginRegisterStart, (void*)tc);
                if(ret < 0)
                {
                    close(new_sock);
                    delete tc;
                    continue;
                }
           }
        }

    private:
        static void* LoginRegisterStart(void* arg)
        {
            // 1. 分离自己，当线程退出之后，线程所占用的资源就被操作系统回收了
            // 2. 接收一个字节的数据，进而判断请求的类型，根据不同的请求类型，调用不同的函数进行处理（注册&登陆）
            //注册
            // 
            //登陆
            pthread_detach(pthread_self());
            TcpConnect* tc = (TcpConnect*) arg;
            char ques_type = -1;
            ssize_t recv_size = recv(tc->GetSockFd(), &ques_type,1, 0);
            if(recv_size < 0)
            {
                close(tc->GetSockFd());
                return NULL;
            }
            else if(recv_size == 0)
            {
                close(tc->GetSockFd());
                return NULL;
            }
            // 接收回来了一个字节的数据
            switch(ques_type)
            {
                case REGISTER_RESQ:
                    {
                        // TODO

                        // 处理注册请求, DealRegister
                        break;
                    }
                case LOGIN_RESQ:
                    {

                        // 处理登陆请求
                        break;
                    }
            }
        }

        // 不管是注册成功了，还是注册失败了，都需要给客户端一个应答
        int DealRegister(int new_sock)
        {
            // 继续从tcp连接当中接收注册数据，策略就是 直接使用RegisterInfo
            struct RegisterInfo ri;
            ssize_t recv_size = recv(new_sock,&ri,sizeof(ri),0);
            if(recv_size < 0)
            {
                return -1;
            }
            else if(recv_size == 0)
            {
                close(new_sock);
                return -2;
            }

            // 正常接收到了
            // 需要将数据递交给用户管理模块，进行注册，并且将用户数据进行留存
            // 需要和用户管理模块进行交互了
            // TODO
        }

        int DealLogin()
        {
            // 继续从tcp连接中接收登陆数据，策略就是直接使用LoginInfo
            
        }

    private:
        int tcp_sock_;
        int udp_sock_;
        uint16_t tcp_port_;
        };
