#include "test.pb.h"
#include <iostream>
#include <string>

using namespace fixbug;

int test()
{
    // 封装了请求对象的数据
    LoginRequest req;
    req.set_name("peiqi"); // ban li 
    req.set_pwd("i'm a pig");
    // 对象数据序列化 -> char *
    std::string send_str;
    if (req.SerializeToString(&send_str)){
        std::cout << send_str.c_str() << std::endl;
    }

    // 从send_str反序列化一个login请求
    LoginRequest reqB;
    if (reqB.ParseFromString(send_str)){
        std::cout << reqB.name() << std::endl;
        std::cout << reqB.pwd() << std::endl;
    }

    return 0;
}

int main()
{
    // LoginResponse rsp;
    // ResultCode * rc = rsp.mutable_result();
    // rc->set_errcode(1);
    // rc->set_errmsg("Login Failure");

    GetFriendListResponse rsp;
    ResultCode * rc = rsp.mutable_result();
    rc->set_errcode(0);

    User * user1 = rsp.add_friend_list();
    user1->set_age(20);
    user1->set_name("zhang san");
    user1->set_sex(User::MAN);
    
    User * user2 = rsp.add_friend_list();
    user2->set_age(21);
    user2->set_name("li si");
    user2->set_sex(User::WOMAN);

    std::cout << rsp.friend_list_size() << std::endl;

    return 0;
}