//
// Created by owt50 on 2016/9/26.
//

#include <data/player.h>
#include <logic/player_manager.h>
#include <log/log_wrapper.h>
#include <time/time_utility.h>

#include <dispatcher/ss_msg_dispatcher.h>

#include "task_action_ss_req_base.h"

task_action_ss_req_base::task_action_ss_req_base() {}
task_action_ss_req_base::~task_action_ss_req_base() {}

uint64_t task_action_ss_req_base::get_request_bus_id() const {
    msg_cref_type msg = get_request();
    if (!msg.has_src_server()) {
        return 0;
    }

    return msg.src_server().bus_id();
}

const std::string& task_action_ss_req_base::get_player_openid() const {
    msg_cref_type msg = get_request();
    if (msg.has_ssmsg() && msg.ssmsg().has_head() && !msg.ssmsg().head().openid().empty()) {
        msg.ssmsg().head().openid();
    }

    return player_openid_;
}

std::shared_ptr<player> task_action_ss_req_base::get_player() const {
    if (player_inst_) {
        return player_inst_;
    }

    player_inst_ = player_manager::me()->find(get_player_openid());
    return player_inst_;
}

task_action_ss_req_base::msg_ref_type task_action_ss_req_base::add_rsp_msg(int32_t svr_msg_type, uint64_t dst_pd) {
    rsp_msgs_.push_back(msg_type());
    msg_ref_type msg = rsp_msgs_.back();

    msg.mutable_ssmsg()->mutable_head()->set_error_code(get_rsp_code());
    dst_pd = 0 == dst_pd? get_request_bus_id(): dst_pd;

    msg_cref_type req_msg = get_request();
    init_msg(msg, dst_pd, svr_msg_type, req_msg);

    return msg;
}

void task_action_ss_req_base::send_rsp_msg() {
    if (rsp_msgs_.empty()) {
        return;
    }

    for(std::list<msg_type>::iterator iter = rsp_msgs_.begin(); iter != rsp_msgs_.end(); ++iter) {
        if(0 == (*iter).src_server().bus_id()) {
            WLOGERROR("task %s [0x%llx] send message to unknown server", name(), get_task_id_llu());
            continue;
        }
        (*iter).mutable_ssmsg()->mutable_head()->set_error_code(get_rsp_code());

        // send message using ss dispatcher
        int32_t res = ss_msg_dispatcher::me()->send_to_proc((*iter).src_server().bus_id(), &(*iter));
        if (res) {
            WLOGERROR("task %s [0x%llx] send message to server 0x%llx failed, res: %d", name(), get_task_id_llu(),
                static_cast<unsigned long long>((*iter).src_server().bus_id()),
                res
            );
        }
    }

    rsp_msgs_.clear();
}

int32_t task_action_ss_req_base::init_msg(msg_ref_type msg, uint64_t dst_pd, int32_t ss_type) {
    msg.mutable_src_server()->set_bus_id(dst_pd);
    msg.mutable_src_server()->set_type(ss_type);
    msg.mutable_ssmsg()->mutable_head()->set_timestamp(util::time::time_utility::get_now());

    return 0;
}

int32_t task_action_ss_req_base::init_msg(msg_ref_type msg, uint64_t dst_pd, int32_t ss_type, msg_cref_type req_msg) {
    init_msg(msg, dst_pd, ss_type);

    // set task information
    if (req_msg.has_src_server()) {
        if (0 != req_msg.src_server().src_task_id()) {
            msg.mutable_src_server()->set_dst_task_id(req_msg.src_server().src_task_id());
        }

        if (0 != req_msg.src_server().dst_task_id()) {
            msg.mutable_src_server()->set_src_task_id(req_msg.src_server().dst_task_id());
        }
    }

    // copy sequence
    if (req_msg.has_ssmsg() && req_msg.ssmsg().has_head()) {
        msg.mutable_ssmsg()->mutable_head()->set_sequence(req_msg.ssmsg().head().sequence());
    }

    return 0;
}