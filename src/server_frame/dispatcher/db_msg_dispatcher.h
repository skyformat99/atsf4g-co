//
// Created by owt50 on 2016/9/27.
//

#ifndef _DISPATCHER_DB_MSG_DISPATCHER_H
#define _DISPATCHER_DB_MSG_DISPATCHER_H

#pragma once

#include <std/functional.h>

#include <design_pattern/singleton.h>
#include <config/compiler_features.h>

#include "dispatcher_implement.h"

#include <uv.h>
#include <rpc/db/unpack.h>
#include <hiredis/hiredis.h>
#include <config/logic_config.h>

struct redisAsyncContext;
namespace hiredis {
    namespace happ {
        class cluster;
        class raw;
        class cmd_exec;
        class connection;
    }
}

class db_msg_dispatcher UTIL_CONFIG_FINAL : public dispatcher_implement, public util::design_pattern::singleton<db_msg_dispatcher> {
public:
    typedef dispatcher_implement::msg_ptr_t msg_ptr_t;
    typedef dispatcher_implement::msg_type_t msg_type_t;

    typedef int32_t (*unpack_fn_t)(hello::message_container& msg_container, const redisReply* reply);
    typedef std::function<int()> user_callback_t;

    struct channel_t {
        enum type {
            CLUSTER_BOUND = 0,
            CLUSTER_DEFAULT,

            SENTINEL_BOUND,

            RAW_DEFAULT,
            RAW_BOUND,

            MAX
        };
    };

protected:
    db_msg_dispatcher();
    ~db_msg_dispatcher();

public:
    virtual const const char* name() const UTIL_CONFIG_OVERRIDE;
    virtual int32_t init() UTIL_CONFIG_OVERRIDE;

    /**
     * @brief run tick handle and return active action number
     * @return active action number or error code
     */
    virtual int tick() UTIL_CONFIG_OVERRIDE;

    /**
     * @brief 数据解包
     * @param msg_container 填充目标
     * @param msg_buf 数据地址
     * @param msg_size 数据长度
     * @return 返回错误码或0
     */
    virtual int32_t unpack_msg(msg_ptr_t msg_container, const void* msg_buf, size_t msg_size) UTIL_CONFIG_OVERRIDE;

    /**
     * @brief 获取任务信息
     * @param msg_container 填充目标
     * @return 相关的任务id
     */
    virtual uint64_t pick_msg_task(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE;

    /**
     * @brief 获取消息名称
     * @param msg_container 填充目标
     * @return 消息名称
     */
    virtual const std::string& pick_msg_name(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE;

    /**
     * @brief 获取消息名称
     * @param msg_container 填充目标
     * @return 消息类型ID
     */
    virtual msg_type_t pick_msg_type_id(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE;

    /**
     * @brief 获取消息名称到ID的映射
     * @param msg_name 消息名称
     * @return 消息类型ID
     */
    virtual msg_type_t msg_name_to_type_id(const std::string& msg_name) UTIL_CONFIG_OVERRIDE;


    /**
     * @brief 启动标准的DB数据发送流程
     * @param t 发送的数据库渠道
     * @param ks redis key的内容（仅cluster渠道有效）
     * @param kl redis key的长度（仅cluster渠道有效）
     * @param task_id 任务id
     * @param pd 启动进程pd
     * @param fn 解包函数
     * @param argc @see redisAsyncCommandArgv
     * @param argv @see redisAsyncCommandArgv
     * @param argvlen @see redisAsyncCommandArgv
     * @return 0或错误码
     */
    int send_msg(channel_t::type t, const char* ks, size_t kl,
        uint64_t task_id, uint64_t pd,
        unpack_fn_t fn, int argc, const char **argv, const size_t *argvlen);

    /**
     * @brief 启动标准的DB数据发送流程
     * @param t 发送的数据库渠道
     * @param ks redis key的内容（仅cluster渠道有效）
     * @param kl redis key的长度（仅cluster渠道有效）
     * @param task_id 任务id
     * @param pd 启动进程pd
     * @param fn 解包函数
     * @param format @see redisAsyncCommand
     * @param ... @see redisAsyncCommand
     * @return 0或错误码
     */
    int send_msg(channel_t::type t, const char* ks, size_t kl,
        uint64_t task_id, uint64_t pd,
        unpack_fn_t fn, const char *format, ...);


    /**
     * @brief 获取用于protobuf序列化的临时缓冲区
     * @param 缓冲区长度
     * @return 缓冲区真实地址
     */
    void* get_cache_buffer(size_t len);

    /**
     * @brief 数据库服务是否有效
     * @return 数据库服务有效返回true
     */
    bool is_available(channel_t::type t) const;

    /*
     * @brief 获取表脚本SHA1
     */
    const std::string &get_db_script_sha1(uint32_t type) const;

    /*
     * @brief 设置表脚本SHA1
     * @param 字符串指针
     * @param 字符串长度
     */
    void set_db_script_sha1(uint32_t type, const char * str, int len);

    /*
     * @brief 连接完成时调用用户的设置的回调函数
     */
    void set_on_connected(channel_t::type t, user_callback_t fn);
private:
    static void log_debug_fn(const char* content);
    static void log_info_fn(const char* content);

    int script_load(redisAsyncContext *c, uint32_t type);
    int open_file(const char *file, std::string &script);

    // common helper
    static void on_timer_proc(uv_timer_t* handle);
    static void script_callback(redisAsyncContext *c, void *r, void *privdata);

    // cluster
    int cluster_init(const std::vector<logic_config::LC_DBCONN>& conns, int index);
    static void cluster_request_callback(hiredis::happ::cmd_exec* , struct redisAsyncContext *c, void *r, void *privdata);
    static void cluster_on_connect(hiredis::happ::cluster*, hiredis::happ::connection*);
    static void cluster_on_connected(hiredis::happ::cluster*, hiredis::happ::connection*, const struct redisAsyncContext*, int status);

    /**
     * @brief 启动标准的DB数据发送流程
     * @param clu 发送的数据库渠道
     * @param ks redis key的内容
     * @param kl redis key的长度
     * @param task_id 任务id
     * @param pd 启动进程pd
     * @param fn 解包函数
     * @param argv @see redisAsyncCommandArgv
     * @param argvlen @see redisAsyncCommandArgv
     * @return 0或错误码
     */

    int cluster_send_msg(hiredis::happ::cluster& clu, const char* ks, size_t kl,
        uint64_t task_id, uint64_t pd,
        unpack_fn_t fn, int argc, const char **argv, const size_t *argvlen);

    /**
     * @brief 启动标准的DB数据发送流程
     * @param clu 发送的数据库渠道
     * @param ks redis key的内容
     * @param kl redis key的长度
     * @param task_id 任务id
     * @param pd 启动进程pd
     * @param fn 解包函数
     * @param format @see redisvAsyncCommand
     * @param ap @see redisvAsyncCommand
     * @return 0或错误码
     */
    int cluster_send_msg(hiredis::happ::cluster& clu, const char* ks, size_t kl,
        uint64_t task_id, uint64_t pd,
        unpack_fn_t fn, const char *format, va_list ap);

    // raw
    int raw_init(const std::vector<logic_config::LC_DBCONN>& conns, int index);
    static void raw_request_callback(struct redisAsyncContext *c, void *r, void *privdata);
    static void raw_on_connect(hiredis::happ::raw *c, hiredis::happ::connection*);
    static void raw_on_connected(hiredis::happ::raw *c, hiredis::happ::connection*, const struct redisAsyncContext*, int status);

    /**
     * @brief 启动标准的DB数据发送流程
     * @param task_id 任务id
     * @param pd 启动进程pd
     * @param fn 解包函数
     * @param argc @see redisAsyncCommandArgv
     * @param argv @see redisAsyncCommandArgv
     * @param argvlen @see redisAsyncCommandArgv
     * @return 0或错误码
     */
    int raw_send_msg(hiredis::happ::raw& raw_conn, uint64_t task_id, uint64_t pd, unpack_fn_t fn, int argc, const char **argv, const size_t *argvlen);

    /**
     * @brief 启动标准的DB数据发送流程
     * @param task_id 任务id
     * @param pd 启动进程pd
     * @param fn 解包函数
     * @param format @see redisAsyncCommand
     * @param ... @see redisAsyncCommand
     * @return 0或错误码
     */
    int raw_send_msg(hiredis::happ::raw& raw_conn, uint64_t task_id, uint64_t pd, unpack_fn_t fn, const char *format, va_list ap);

private:
    uv_timer_t* tick_timer_;
    int tick_msg_count_;
    std::vector<char> pack_cache_;

    // user callbacks
    std::list<user_callback_t> user_callback_onconnected_[channel_t::MAX];

    // script sha1
    std::string db_script_sha1_[hello::EnDBScriptShaType_ARRAYSIZE];

    // channels
    std::shared_ptr<hiredis::happ::cluster> db_cluster_conns_[channel_t::SENTINEL_BOUND];
    std::shared_ptr<hiredis::happ::raw> db_raw_conns_[channel_t::RAW_BOUND - channel_t::SENTINEL_BOUND];
};

#endif //ATF4G_CO_DB_MSG_DISPATCHER_H
