//
// Created by owt50 on 2016/10/9.
//

#include <dispatcher/cs_msg_dispatcher.h>

#include <logic/task_action_login_authorization.h>

#include "handle_cs_msg.h"

#define REG_MSG_HANDLE(dispatcher, ret, act, proto)\
    if (ret < 0) { dispatcher::me()->register_action<act>(proto); } \
    else { ret = dispatcher::me()->register_action<act>(proto); }

int app_handle_cs_msg::init() {
    int ret = 0;

    REG_MSG_HANDLE(cs_msg_dispatcher, ret, task_action_login_authorization, "mcs_login_auth_req");

    return ret;
}