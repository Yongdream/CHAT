// 自定义头文件
#include "json.hpp"
#include "group.hpp"
#include "user.hpp"
#include "public.hpp"
using json = nlohmann::json;
 
// 标准库头文件
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
 
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>
 
// 全局变量、对象、函数
User g_currentUser;                   // 记录当前系统登录的用户信息
vector<User> g_currentUserFriendList; // 记录当前登录用户的好友列表信息
vector<Group> g_currentUserGroupList; // 记录当前登录用户的群组列表信息
bool isMainMenuRunning = false;       // 控制主菜单页面程序
 
// 信号量
sem_t rwsem; // 用于读写线程之间的通信
 
/*atomic原子变量是一种多线程编程中常用的同步机制，它能够确保对共享变量的操作在执行时不会被其他线程的操作干扰
原子变量它具有类似于普通变量的操作，但是这些操作都是原子级别的，即要么全部完成，要么全部未完成。*/
 
atomic_bool g_isLoginSuccess{false}; // 记录登录状态           atomic防止线程安全引发的静态条件问题
 
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
 
// 显示当前登录成功用户的基本信息
void showCurrentUserData();
 
// 主聊天页面程序
void mainMenu(int);
 
// 接收线程
void readTaskHandler(int clientfd);
 
// 聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程
int main(int argc, char **argv) // argc 参数个数    argv 参数序列或指针
{
    if (argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl; // cerr:程序错误信息
        exit(-1);
    }
 
    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
 
    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET:IPv4  SOCK_STREAM:可靠的、双向的通信数据流 0:调用者不指定协议
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }
 
    // 填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in)); // memset清0
 
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
 
    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);
 
    // 连接服务器成功，启动接收子线程
    std::thread readTask(readTaskHandler, clientfd); // pthread_create
    readTask.detach();                               // pthread_detach
 
    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "========================" << endl;
        cout << "======  1. login  ======" << endl;
        cout << "======  2. register  ===" << endl;
        cout << "======  3. quit  =======" << endl;
        cout << "========================" << endl;
        cout << "Please input your choice:";
        int choice = 0;
        cin >> choice; // 读取功能选项
        cin.get();     // 读掉缓冲区残留的回车
 
        switch (choice)
        {
        case 1: // 登录业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "user id:";
            cin >> id;
            cin.get(); // 读掉缓冲区残留的回车
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
                // 进入聊天主菜单页面
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2: // 注册业务
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
        break;
        case 3: // 退出业务
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }
 
    return 0;
}
 
// 显示当前登录成功用户的基本信息
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
 
// 获取系统时间（聊天信息需要添加时间信息）
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
 
// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 注册失败
    {
        cerr << "name is already exist, register error!" << endl;
    }
    else // 注册成功
    {
        cout << "name register success, userid is " << responsejs["id"]
             << ", do not forget it!" << endl;
    }
}
 
// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 登录失败
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else // 登录成功
    {
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
 
        g_isLoginSuccess = true;        //登录状态
    }
}
 
// 子线程 - 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // 阻塞
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }
 
        // 接收服务器转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>(); // 获取处理业务msgid
        if (ONE_CHAT_MSG == msgtype)          // 点对点聊天
        {
            cout << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
 
        if (GROUP_CHAT_MSG == msgtype) // 群聊
        {
            cout << "群消息[" << js["groupid"] << "]:" << js["time"].get<string>() << " [" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
 
        if (LOGIN_MSG_ACK == msgtype) // 登录业务
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem);    // 通知主线程，登录结果处理完成
            continue;
        }
 
        if (REG_MSG_ACK == msgtype) // 注册业务
        {
            doRegResponse(js); // 处理注册响应的业务逻辑
            sem_post(&rwsem);  // 通知主线程，注册结果处理完成
            continue;
        }
    }
}
 
// 客户端命令列表
unordered_map<string, string> CommandMap{
    {"help", "显示所有支持的命令 格式 help"},
    {"chat", "一对一聊天 格式 chat:friendid:message"},
    {"addfriend", "添加好友 格式 addfriend:friendid"},
    {"creategroup", "创建群组 格式 creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组 格式 addgroup:groupid"},
    {"groupchat", "群聊 格式 groupchat:groupid:message"},
    {"loginout", "注销 格式 loginout"},
};
 
// help command Handler
void help(int fd = 0, string = "");
// chat command Handler
void chat(int, string);
// addfriend command Handler
void addfriend(int, string);
// creategroup command Handler
void creategroup(int, string);
// addgroup command Handler
void addgroup(int, string);
// groupchat command Handler
void groupchat(int, string);
// oginout command Handler
void loginout(int, string);
 
// 客户端命令处理
unordered_map<string, function<void(int, string)>> CommandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout},
};
 
// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);      // 捕获客户端命令
        string commandbuf(buffer);      // 由char * 转为string类型
        string command;                 // 存储命令
        int idx = commandbuf.find(":"); // 判断是否为help命令或loginout命令
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx); // 截取命令类型
        }
        auto it = CommandHandlerMap.find(command);
        if (it == CommandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
 
        // 调用相应命令的事件处理回调，mainMenu对修改封闭，添加新功能无需更改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令处理方法
    }
}
 
// help command Handler
void help(int, string)
{
    cout << "--------------show command list-------------" << endl;
    for (auto &p : CommandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}
 
// chat command Handler              "chat:friendid:message"
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
 
    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
 
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error ->" << buffer << endl;
    }
}
 
// addfriend command Handler         "addfriend:friendid"
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
 
    string buffer = js.dump(); // 序列化发出
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error ->" << buffer << endl;
    }
}
 
// creategroup command Handler       “creategroup:groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);
 
    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();
 
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send creategroup msg error ->" << buffer << endl;
    }
}
 
// addgroup command Handler              "addgroup:groupid"
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
 
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroup msg error ->" << buffer << endl;
    }
}
// groupchat command Handler         "groupchat:groupid:message"
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "groupchat command invalid1" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
 
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
 
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send groupchat msg error ->" << buffer << endl;
    }
}
// loginout command Handler
void loginout(int clientfd, string str){
    json js;
    js["msgid"] =LOGINOUT_MSG ;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send loginout msg error ->" << buffer << endl;
    }else{
        isMainMenuRunning = false;
    }
}