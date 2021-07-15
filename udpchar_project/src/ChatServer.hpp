#pragma once
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>

using namespace std;
#include "ConnectInfo.hpp"
#include "tools.hpp"
#include "UserManager.hpp"
#include "MessagePool.hpp"

#define MAX_ROUND_COUNT 10
#define THREAD_COUNT 1

class TcpConnect
{
    public:
        TcpConnect()
        {
            new_sock_ = -1;
            server_ = NULL;
        }

        ~TcpConnect()
        {

        }

        void SetSockFd(int fd)
        {
            new_sock_ = fd;
        }

        void SetServer(void* server)
        {
            server_ = server;
        }
        int GetSockFd()
        {
            return new_sock_;
        }

        void* GetServer()
        {
            return server_;
        }

    private:
        int new_sock_;

        // 保存Chatserver这个类的this指针，确保在tcp线程入口函数中可以获取到用户管理模块的实例化指针
        void* server_;
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
            udp_port_ = UDP_PORT;

            user_manager_ = NULL;
            memset(con_tid_,'\0',THREAD_COUNT*sizeof(pthread_t));
            memset(pro_tid_,'\0',THREAD_COUNT*sizeof(pthread_t));

            msg_pool_ = NULL;

            udp_msg = NULL;
        }

        ~ChatServer()
        {
            if(user_manager_)
            {
                delete user_manager_;
                user_manager_ = NULL;
            }

            if(msg_pool_)
            {
                delete msg_pool_;
                msg_pool_ =NULL;
            }

            if(udp_msg_)
            {
                delete udp_msg_;
                udp_msg_ = NULL;
            }
        }

        // 初始化变量的接口，被调用者调用
        // 用户管理模块的实例化对象，消息池的实例化对象
        int InitSvr(uint16_t tcp_port = TCP_PORT)
        {
            // 创建消息池
            msg_pool_ = new MsgPool(1024);
            if(msg_pool_ == NULL)
            {
                LOG(ERROR, "init msgpool failed")<<endl;
            }


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

            LOG(INFO, "listen port is ")<< tcp_port <<endl;
            //创建用户管理模块的指针
            user_manager_ = new UserManager();
            if(!user_manager_)
            {
                LOG(INFO, "listen port is ")<< tcp_port <<endl;
                return -1;
            }

            // 暂时还没有考虑的是udp通信，消息池的初始化
            udp_sock_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if(udp_sock_ < 0)
            {
                LOG(ERROR, "create udp socket failed")<<endl;
                return -1;
            }


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
            for(int i = 0; i < THREAD_COUNT;++i)
            {
                int ret = pthread_create(&con_tid_[i], NULL, ConsumeStart, (void*) this);
                if(ret<0)
                {
                    LOG(ERROR, "Start udp thread failed")<<endl;
                    return -1;
                }
                ret = pthread_create(&pro_tid_[i], NULL, ProductStart, (void*) this);
                if(ret < 0)
                {
                    LOG(ERROR, "Start udp thread failed")<<endl;
                    return -1;

                }
            }



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
                tc->SetServer((void*)this);

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
        static void* ConsumeStart(void* arg)
        {
            pthread_detach(pthread_self());
            ChatServer* cs = (ChatServer*) arg;
            // 1. 从消息池中获取消息
            // 2. 推送给所有的在线用户
            while(1)
            {
                cs->SendMsg();
            }
        }

        static void* ProductStart(void* arg)
        {
            pthread_detach(pthread_self());
            ChatServer* cs = (ChatServer*) arg;
            //1. 接收udp数据
            //2. 将数据发送到消息池当中
            while(1)
            {
                cs->RecvMsg();
            }
        }




        static void* LoginRegisterStart(void* arg)
        {
            // 1. 分离自己，当线程退出之后，线程所占用的资源就被操作系统回收了
            // 2. 接收一个字节的数据，进而判断请求的类型，根据不同的请求类型，调用不同的函数进行处理（注册&登陆）
            //注册
            // 
            //登陆
            pthread_detach(pthread_self());
            TcpConnect* tc = (TcpConnect*) arg;
            ChatServer* cs = (ChatServer*) tc->GetServer();

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
            int resp_status = -1;
            uint32_t user_id;
            switch(ques_type)
            {
                case REGISTER_RESQ:
                    {
                        // TODO

                        // 处理注册请求, DealRegister
                        resp_status = cs->DealRegister(tc->GetSockFd(), &user_id);
                        break;
                    }
                case LOGIN_RESQ:
                    {

                        // 处理登陆请求
                        resp_status = cs->DealLogin(tc->GetSockFd(), &user_id);
                        break;
                    }
            }

            // 响应的构造
            // struct RelpyInfo
            //   status
            //  userid
            // send进行发送

            struct RelpyInfo ri;
            ri.resp_status_ = resp_status;
            ri.id_ = user_id;

            LOG(INFO,"resp_status is ")<<ri.resp_status_ << endl;
            LOG(INFO,"id is ")<< ri.id_ <<endl;

            int max_tound_count = MAX_ROUND_COUNT;
            while(max_tound_count)
            {
                ssize_t send_size = send(tc->GetSockFd(), &ri, sizeof(ri), 0);
                if(send_size >= 0)
                {
                    LOG(INFO,"Send reply success")<<endl;
                    break;
                }
                // 发送失败，是否需要重复发送? (需不需要将待发送的数据进行缓存起来)
                LOG(WARNING, "Send reply failed")<<endl;

                max_tound_count--;
            }

            // 关闭当前连接的套接字
            close(tc->GetSockFd());
            delete tc;
            tc =NULL;
            return NULL;
        }

    private:
        // 不管是注册成功了，还是注册失败了，都需要给客户端一个应答
        int DealRegister(int new_sock, uint32_t* user_id)
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

            int ret = user_manager_->DealRegister(ri.nick_name_, ri.school_, ri.passwd_, user_id);
            if(ret < 0)
            {
                // 注册失败了
                return REGISTER_FAILED;
            }

            // 注册成功
            return REGISTER_SUCCESS;

        }

        int DealLogin(int new_sock, uint32_t* user_id)
        {
            // 继续从tcp连接中接收登陆数据，策略就是直接使用LoginInfo
            struct LoginInfo li;

            ssize_t recv_size = recv(new_sock, &li, sizeof(li), 0);
            if(recv_size < 0)
            {
                return -1;
            }
            else if(recv_size == 0)
            {
                close(new_sock);
                return -2;
            }

            *user_id = li.id_;
            // 正常逻辑
            // TODO 调用用户管理模块，判断登录请求当中的id和密码是否正确
            int ret = user_manager_->DealLogin(li.id_, li.passwd_);
            if(ret < 0)
            {
                return LOGIN_FAILED;
            }
            return LOGIN_SUCCESS;
        }

        int RecvMsg()
        {
            // 1. 接收udp数据
            // 2，判断该用户是否是登陆用户
            // 3. 判断该用户是否是第一次发送udp数据
            //     如果是：需要保存该用户的udp地址，并且将该用户放到在线用户列表当中 
            //     如果不是:说明这个用户就是老用户，之前已经保存过该用户的udp地址信息
            // 4. 将数据发送到消息池当中
            
            char buf[UDP_MAX_DATA_LEN] = {0};
            ssize_t recv_size = recvfrom(udp_sock_, buf, sizeof(buf)-1, 0, NULL, NULL);
            if(recv_size < 0)
            {
                return -1;
            }

            UdpMsg um;
            string msg;
            msg.assign(buf, strlen(buf));
            um.deserialize(msg);


            // 需要使用user_id和用户管理模块进行验证
            //     1. 先使用该user_id去map当中查找
            //     2.需要判断当前用户的状态，保存用户的udp地址信息
            
        
            
        }

        int SendMsg()
        {

        }

    private:
        int tcp_sock_;
        int udp_sock_;
        uint16_t tcp_port_;
        uint16_t udp_port_;

        UserManager* user_manager_;
        // UDP线程的标识符数组
        pthread_t con_tid_[THREAD_COUNT];
        pthread_t pro_tid_[THREAD_COUNT];

        MsgPool* msg_pool_;

        UdpMsg* udp_msg_;

};
