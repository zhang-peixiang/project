#pragma once 
#include <stdio.h>
#include <string.h>
#include <iostream>

// 注册请求的数据格式
//
// 1. 昵称
// 2. 学校
// 3. 用户的密码
// client to server 严格按照Register传输数据

struct RegisterInfo
{

    RegisterInfo()
    {
        memset(nick_name_, '\0',sizeof(nick_name_));
        memset(school_, '\0', sizeof(school_));
        memset(passwd_, '\0', sizeof(passwd_));
    }


    char nick_name_[20];
    char school_[20];
    char passwd_[20];
};

// 登陆请求的数据格式
//
// 1. 用户id
// 2. 密码
//
struct LoginInfo
{
    LoginInfo()
    {
        memset(passwd_, '\0', sizeof(passwd_));
    }

    uint32_t id_;
    char passwd_[20];
};



// 服务端给客户端回复应答的数据格式
//
// 1. 当前状态（注册状态， 登陆状态）
// 2. 返回用户ID，类似于注册完毕之后返回的“QQ”号

// 枚举状态
enum Status
{
    REGISTER_FAILED = 0,
    REGISTER_SUCCESS,
    LOGIN_FAILED,
    LOGIN_SUCCESS
};

struct RelpyInfo
{
    int resp_status_;
    uint32_t id_;
};

// 如何标识当前的请求是注册请求还是登陆请求
//
// 1. 对于每一种请求，在tcp层面都会发送两个tcp包
//   第一个tcp包，发送一个字节，标识请求是“注册”还是“登陆”
//   第二个tcp包，发送具体的“注册”或者“登陆请求”的数据
//
enum TesqType
{
    REGISTER_RESQ = 0,
    LOGIN_RESQ
};