#pragma once
#include <iostream>
#include <pthread.h>
#include <vector>

#include <ncurses.h>
#include "tools.hpp"
#include "ChatClient.hpp"

using namespace std;

class ChatWindow;

class Pram
{
    public:
        Pram(int thread_num, ChatWindow* cw, UdpClient* uc)
        {
            thread_num_ = thread_num;
            cw_ = cw;
            uc_ = uc;
        }


    public:
        int thread_num_;
        ChatWindow* cw_;
        UdpClient* uc_;
};

class ChatWindow
{
    public:
        ChatWindow()
        {
            header_ = NULL;
            output_ = NULL;
            user_list_ = NULL;
            input_ = NULL;
            vec_.clear();

            pthread_mutex_init(&lock_win_, NULL);
        }

        ~ChatWindow()
        {
            if(header_)
            {
                delwin(header_);
            }
            if(output_)
            {
                delwin(output_);
            }
            if(user_list_)
            {
                delwin(user_list_);
            }
            if(input_)
            {
                delwin(input_);
            }

            pthread_mutex_destroy(&lock_win_);

            endwin();

        }

        // 启动线程
        int start(UdpClient* uc)
        {
            // 初始化ncuress
            initscr();
            for(int i = 0; i < 4; i++)
            {
                Pram* pram = new Pram(i, this, uc);
                if(!pram)
                {
                    LOG(ERROR, "create parm failed") <<endl;
                    exit(1);
                }
                pthread_t tid;
                int ret = pthread_create(&tid, NULL, RunWindowStart, (void*)pram);
                if(ret < 0)
                {
                    LOG(ERROR, "create window thread failed")<<endl;
                    exit(1);
                }

                vec_.push_back(tid);
            }

            for(size_t i = 0; i < vec_.size(); i++)
            {
                // 专门让调用start函数的客户端在主线程进行等待
                pthread_join(vec_[i], NULL);
            }
        }

       static  void* RunWindowStart(void* arg)
        {
            // 1. 需要区分不同的线程，让不同的线程绘制不同的窗口
            //    使用线程创建时的变量i进行区分
            // 2. 传递进来窗口的句柄
            Pram* pram = (Pram*)arg;
            int thread_num = pram->thread_num_;
            ChatWindow* cw = pram->cw_;
            UdpClient* uc = pram->uc_;

            switch(thread_num)
            {
                case 0:
                    // header
                    cw->Runheader();
                    break;
                case 1:
                    // output
                    cw->RunOutput(uc); 
                    break;
                case 2:
                    // user_list
                    cw->RunUserList(uc);
                    break;
                case 3:
                    // input
                    cw->RunInput(uc);
                    break;
            }

            delete pram;
            return NULL;
        }


       void RunUserList(UdpClient* uc)
       {
           WINDOW* old_user_list = NULL;
           int line = 1;
           int cols = 1;
           
           while(1)
           {
               user_list_ = newwin((3*LINES)/5, COLS/3,LINES/5,(3*COLS)/4);
               box(user_list_, 0, 0);
               RefreshWin(user_list_);
               if(old_user_list)
               {
                   delwin(old_user_list);
               }

              vector<UdpMsg> vec =  uc->GetVec();
              for(size_t i = 0; i< vec.size(); i++)
              {
                  string msg;
                  msg += vec[i].nick_name_;
                  msg += ":";
                  msg += vec[i].school_;

                  mvwaddstr(user_list_,line+i, cols, msg.c_str());
                  RefreshWin(user_list_);
              }


               old_user_list = user_list_;

               sleep(1);
           }
       }

       void RefreshWin(WINDOW* win)
       {
            pthread_mutex_lock(&lock_win_);
            wrefresh(win);
            pthread_mutex_unlock(&lock_win_);

       }


       void DrowOutput()
       {
            output_ = newwin((LINES*3)/5, (COLS*3)/4, LINES/5, 0);
            box(output_, 0, 0);
            RefreshWin(output_);
       }

       void RunInput(UdpClient* uc)
       {

           WINDOW* old_input = NULL;
           while(1)
           {
               input_ = newwin(LINES/5, COLS, (LINES*4)/5, 0);
               box(input_, 0, 0);
               RefreshWin(input_);

               if(old_input)
               {
                   delwin(old_input);
               }

               string tips = "please enter msg# ";
               mvwaddstr(input_,1, 1, tips.c_str());
               RefreshWin(input_);

               char buf[UDP_MAX_DATA_LEN] = {0};
               wgetnstr(input_, buf, sizeof(buf)-1);

               UdpMsg um;
               
               um.nick_name_ = uc->GetMe().nick_name_;
               um.school_ = uc->GetMe().school_;
               um.user_id_ = uc->GetMe().user_id_;
               um.msg_.assign(buf, strlen(buf));

               string send_msg;
               um.serialize(&send_msg); // 反序列化

               uc->SendUdpMsg(send_msg);

               old_input = input_;

               sleep(1);
           }
       }

       void RunOutput(UdpClient* uc)
       {
           // 比较复杂，原因： 需要调用udp的接收，并且将数据展示在界面当中
           
           int line = 1;
           int cols = 1;

           DrowOutput();
           int y,x;
           getmaxyx(output_, y, x);

            string msg;

           while(1)
           {
               msg.clear();
               // 调用udp接收
               uc->RecvUdpMsg(&msg);

               // udp消息反序列化
               UdpMsg um;
               um.deserialize(msg);

               // nick_name-school:msg
               // 组指展示在output窗口的消息
               string show_msg;
               show_msg += um.nick_name_;
               show_msg += "-";
               show_msg += um.school_;
               show_msg += ":";
               show_msg += um.msg_;

               if(line >= y-2)
               {
                   DrowOutput();
                   line = 1;
               }

               // 把show_msg展示在output窗口
               mvwaddstr(output_,line, cols, show_msg.c_str());
               RefreshWin(output_);

               line++;

               vector<UdpMsg>& vec = uc->GetVec();

               int flag = 1;
               for(size_t i = 0; i < vec.size(); i++)
               {
                   if(um.user_id_ == vec[i].user_id_)
                   {
                       flag = 0;
                       break;
                   }
               }
               if(flag == 1)
               {
                   vec.push_back(um);
               }

           }
       }
       void Runheader()
       {
           WINDOW* old_header_ = NULL;
           while(1)
           {
               if(old_header_)
               {
                   delwin(old_header_);
               }
               header_ = newwin(LINES/5, COLS, 0, 0);
               box(header_, 0, 0);
               RefreshWin(header_);
               string msg = "welcome to out chat systerm";

               int y, x;
               getmaxyx(header_, y, x);
               mvwaddstr(header_, y/2, (x-msg.size())/2, msg.c_str());
               RefreshWin(header_);
               old_header_ = header_; 

               sleep(1);
           }
       }

    private :
        WINDOW* header_;
        WINDOW* output_;
        WINDOW* user_list_;
        WINDOW* input_;

        pthread_mutex_t lock_win_;

        vector<pthread_t> vec_;
};
