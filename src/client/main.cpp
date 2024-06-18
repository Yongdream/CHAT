#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 全局变量 对象
User g_currentUser;                     // 记录当前系统登录的用户信息
vector<User> g_currentUserFriendList;   // 记录当前用户的好友信息
vector<Group> g_currentUserGroupList;   // 记录当前用户的群组信息
void showCurrentUserData();             // 显示当前登录成功用户的基本信息
bool isMainMenuRunning = false;         // 控制menu界面的全局变量

// 信号量
sem_t rwsem;                         // 用于读写线程之间的通信
atomic_bool g_isLoginSuccess{false}; // 原子变量记录登录状态


string getCurrentTime();        // 获取系统时间
void showCurrentUserData();     // 显示当前登录成功用户基本信息
string getCurrentTime();        // 获取系统时间
void mainMenu(int clientfd);    // 主聊天页面

void readTaskHandler(int clientfd); // 接受线程


// 显示当前登录成功用户基本信息
void showCurrentUserData()
{
    cout << "======================login user======================" << endl;
    cout << "current login user -> id:" << g_currentUser.getId() << " name:" << g_currentUser.getName() << endl;
    cout << "----------------------friend list---------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << endl;
    cout << "----------------------group list----------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << user.getId() << " " << user.getName() << " " << user.getState()
                     << " " << user.getRole() << endl;
            }
            cout << endl;
        }
    }
    cout << "======================================================" << endl;
}

// 获取系统时间
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return std::string(date);
}

// 登录
void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) 
    {
        // 登录失败
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else 
    {
        // 登录成功
        // 记录当前用户的id和name
        g_currentUser.setId(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);
 
        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();
 
            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str); // 反序列化
                User user;
                user.setId(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }
 
        // 记录当前用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();
 
            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr : vec1)
            {
                json grpjs = json::parse(groupstr); // 反序列化
                Group group;
                group.setId(grpjs["id"].get<int>());
                group.setName(grpjs["groupname"]);
                group.setDesc(grpjs["groupdesc"]);
 
                vector<string> vec2 = grpjs["users"]; // 群组内成员信息
                for (string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
 
                g_currentUserGroupList.push_back(group);
            }
        }
 
        // 显示登录用户的基本信息
        showCurrentUserData();
 
        // 显示当前用户的离线消息  个人聊天信息或者群组消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);                 // 反序列化
                if (ONE_CHAT_MSG == js["msgid"].get<int>()) // 点对点聊天
                {
                    cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
                else // 群聊
                {
                    cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }
 
        g_isLoginSuccess = true;
    }
}

// 注册
void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) 
    {
        // 注册失败
        cerr << "name is already exist, register error!" << endl;
    }
    else 
    {
        // 注册成功
        cout << "name register success, userid is " << responsejs["id"]
             << ", do not forget it!" << endl;
    }
}

// 退出
void loginout(int clientfd, string str)
{
    // 退出菜单
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump(); // 进行json序列化，进行发送

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0); // strlen数据长度+1，包含空字符 发送
    if (-1 == len)                                                           // 发送失败
    {
        cerr << "send loginout msg error ->" << buffer << endl;
    }
    else
    {
        // 退出menu
        isMainMenuRunning = false;
    }

}

// 子线程 - 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);  // 阻塞
        if (-1 == len || 0  == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收服务器转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();   // 获取处理业务msgid
        // 私聊
        if (ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        // 群聊
        if (GROUP_CHAT_MSG == msgtype) // 群聊
        {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        // 登录
        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem);    // 通知主线程，登录结果处理完成
            continue;
        }
        // 注册
        if (REG_MSG_ACK == msgtype) // 注册业务
        {
            doRegResponse(js); // 处理注册响应的业务逻辑
            sem_post(&rwsem);  // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 192.168.122.129 6000" << endl; // cerr:程序错误信息
        exit(-1);
    }

    // 解析传递的ip+port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET:IPv4
                                                    // SOCK_STREAM:可靠的、双向的通信数据流 
                                                    // 0:调用者不指定协议
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));    // memset清零

    server.sin_family = AF_INET;            // IPv4
    server.sin_port = htons(port);          // 端口
    server.sin_addr.s_addr = inet_addr(ip); // ip地址

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信的信号量
    sem_init(&rwsem, 0 ,0);

    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd);    // pthread_create
    readTask.detach();                                  // pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for (;;)    // 死循环
    {
        // 显示首页页面 登录、登录、注册、退出
        cout << "========================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.quit" << endl;
        cout << "========================" << endl;
        cout << "choice:" << endl;
        int choice;
        cin >> choice;
        cin.get();  // 读取缓冲区残留的回车

        switch (choice)
        {
            case 1: // login
            {
                int id = 0;
                char pwd[50] = {0};
                cout << "user id:";
                cin >> id;
                cin.get();  // 读掉缓冲区残留的回车
                cout << "user password:";
                cin.getline(pwd, 50);
    
                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();
    
                g_isLoginSuccess = false;
    
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0); // 通过网络send给服务器端
                if (len == -1)
                {
                    cerr << "send login msg error:" << request << endl;
                }
    
                sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后，通知这里
    
                if (g_isLoginSuccess)
                {
                    isMainMenuRunning = true;   // 进入聊天主菜单页面
                    // mainMenu(clientfd);
                }
            }
            case 2: // register
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "user name:";
                cin.getline(name, 50);
                cout << "user password:";
                cin.getline(pwd, 50);
    
                json js;
                js["msgid"] = REG_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();
    
                int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0); // 通过网络send给服务器端
                if (len == -1)
                {
                    cerr << "send reg msg error:" << request << endl;
                }
    
                sem_wait(&rwsem); // 等待信号量，子线程处理完注册消息会通知
                }
            case 3: // quit
            {
                close(clientfd);
                sem_destroy(&rwsem);
                exit(0);
            }
            default:
            {
                cerr << "invalid input!" << endl;
                break;
            }
        }
    }
    return 0;
}



