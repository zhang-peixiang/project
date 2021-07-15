#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <pthread.h>
using namespace std;

#include "ConnectInfo.hpp"
// key-value 结构体 保存用户信息
// key ： 用户id
// value ： 用来代表单个用户的信息
//  
//  缺点： 每次重启服务进程后，之前注册的所有用户都没有
//  解决： 将之前注册的数据放到数据库当中保存
//         1. 启动服务的时候，需要将数据库当中获取之前注册用户的信息，加载到进程内存当中，在判断登陆的时候，不需要查询数据库，用户访问就比较快捷
//         2. 当注册完毕之后，还需要将数据及时写到数据当中
//  解决2：将注册的数据放到文件当中
//
class UserInfo
{
    public:
        UserInfo(const string& nick_name,const  string& school,const  string& passwd, uint32_t user_id)
        {
            nick_name_ = nick_name;
            school_ = school;
            passwd_ = passwd;

            user_id_ = user_id;

            user_status_ = REGISTER_FAILED;

            // 新增udp地址信息的初始化
            memset((void*)& addr_,'\0',sizeof(struct sockaddr_in));
            addr_len_ = 0;
        }

        ~UserInfo()
        {

        }

        string& GetPasswd()
        {
            return passwd_;
        }

        void SetUserStatus(int status)
        {
            user_status_ = status;
        }

        int GetUserStatus()
        {
            return user_status_;
        }

        void SetaddrInfo(struct sockaddr_in addr)
        {
            memcpy(&addr_,&addr, sizeof(addr));
        }

        void SetaddrLenInfo(socklen_t addr_len)
        {
            addr_len_ = addr_len;
        }
    private:
        string nick_name_;
        string school_;
        string passwd_;

        // 用户id
        uint32_t user_id_;

        int user_status_;

        //新增udp地址信息
        struct sockaddr_in addr_;
        socklen_t addr_len_;
        
};


class UserManager
{
    public:
        UserManager()
        {
            user_map_.clear();
            pthread_mutex_init(&map_lock_, NULL);
            prepare_id_ = 0;
        }

        ~UserManager()
        {
            pthread_mutex_destroy(&map_lock_);
        }

        // 处理注册请求
        // user_id是出参，返回给调用者
        int DealRegister(const string& nick_name,const string& school,const  string& passwd, uint32_t*  user_id)
        {
            //1.判断密码字段是否为空
            if(nick_name.size()==0 || school.size()==0 || passwd.size()==0)
            {
                return -1;
            }
            //2.创建单个用户 UserInfo 这个类的对象
            pthread_mutex_lock(&map_lock_);
            //3.分配用户id
            UserInfo ui(nick_name, school, passwd, prepare_id_);
            *user_id = prepare_id_;
            // 需要更改当前用户的状态
            ui.SetUserStatus(REGISTER_SUCCESS);

            //4.将用户的数据插入到map当中
            user_map_.insert(make_pair(prepare_id_,ui));
            
            //5.更新预分配的用户id
            prepare_id_++;

            pthread_mutex_unlock(&map_lock_);
           
            return 0; 
        }

        int DealLogin(uint32_t id, const string& passwd)
        {
            // 1.判断passwd是否为空
            // 2.使用id，在unordered_map当中查找是否有该id对应的值
            //   2.1 没找到该id对应的值，返回登陆失败
            //   2.2 找到了id对应的值
            //      对比保存的密码的值和该次提交上来的密码的值是否一致
            //        a. 如果不一致，则登陆失败
            //        b. 如果一致，则登陆成功
            if(passwd.size() == 0)
            {
                return -1;
            }

            unordered_map<uint32_t, UserInfo>::iterator iter;

            pthread_mutex_lock(&map_lock_);

            iter = user_map_.find(id);
            if(iter == user_map_.end())
            {
                pthread_mutex_unlock(&map_lock_);
                return -2;
            }

            // 查找到了,比较密码
            string reg_passwd = iter->second.GetPasswd();
        
            if(reg_passwd != passwd)
            {
                iter->second.SetUserStatus(LOGIN_FAILED);
                pthread_mutex_unlock(&map_lock_);
                return -3;
            }

            // 密码对比上了
            iter->second.SetUserStatus(LOGIN_SUCCESS);
            pthread_mutex_unlock(&map_lock_);
            return 0;
        }

        // user_id:用户id
        // addr： udp客户端的地址信息，为了后面推送消息所保存的
        // addr_len: udp客户端的地址信息长度
        int IsLogin(uint32_t user_id, struct sockaddr_in addr, socklen_t addr_len)
        {
            // 1.使用use_id去map当中查询，是否存在该用户
            //  如果存在，则获取用户信息，判断用户状态
            //  如果不存在，直接返回，刚刚接收的udp数据直接丢弃掉
            unordered_map<uint32_t, UserInfo>::iterator iter;
            pthread_mutex_lock(&map_lock_);
            iter = user_map_.find(user_id);
            if(iter == user_map_.end())
            {
                pthread_mutex_unlock(&map_lock_);
                return -1;
            }

            UserInfo ui = iter->second;

            // 2.判断用户状态
            //   2.1 第一次发送，则我们保存该用户的地址信息
            //   2.2 如果是第n次发送，则不用添加地址信息
            if(ui.GetUserStatus()<=LOGIN_FAILED)
            {
                pthread_mutex_unlock(&map_lock_);
                return -1;
            }
            if(ui.GetUserStatus() == LOGIN_SUCCESS)
            {
                // 第一次发送udp消息（刚刚登陆完成）
               ui.SetUserStatus(ONLINE);
               ui.SetaddrInfo(addr);
               ui.SetaddrLenInfo(addr_len);

                
            }
            else
            {
                // 第n次发送，老用户
            }

        }
    private:
        // string --id
        // UserInfo -- 保存的具体用户的信息
        unordered_map<uint32_t, UserInfo> user_map_;
        pthread_mutex_t map_lock_;

        //预分配的用户id，当用户管理模块接收到注册请求之后，将prepare_id分配给注册的用户，分配完毕之后，需要对prepare_id进行更新
        uint32_t prepare_id_;
};
