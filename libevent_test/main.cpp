#include <iostream>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <signal.h>
#include <cstring>

// 1. 定时器回调函数：每秒打印一次
void timer_callback(evutil_socket_t fd, short events, void* arg) {
    static int count = 0;
    std::cout << "[TIMER] Tick " << ++count << " - Hello from timer every 1s!" << std::endl;
    
    // 重新添加定时器（实现周期性）
    struct event* timer = (struct event*)arg;
    struct timeval tv = {1, 0}; // 1秒
    evtimer_add(timer, &tv);
}

// 2. 信号回调函数：捕获SIGINT(Ctrl+C)优雅退出
void signal_callback(evutil_socket_t fd, short events, void* arg) {
    struct event_base* base = (struct event_base*)arg;
    std::cout << "\n[SIGNAL] Caught SIGINT, shutting down gracefully..." << std::endl;
    
    // 两秒后退出事件循环
    struct timeval delay = {2, 0};
    event_base_loopexit(base, &delay);
}

// 3. HTTP请求回调函数：处理所有HTTP请求
void http_request_callback(struct evhttp_request* req, void* arg) {
    const char* uri = evhttp_request_get_uri(req);
    std::cout << "[HTTP] Received request for URI: " << uri << std::endl;
    
    // 创建响应缓冲区
    struct evbuffer* buf = evbuffer_new();
    const char* response = "<html><body><h1>Hello from libevent HTTP server!</h1>"
                          "<p>Current time: " __TIME__ "</p>"
                          "<p>Try to access <a href='/test'>/test</a> or any other path.</p>"
                          "</body></html>";
    
    evbuffer_add_printf(buf, "%s", response);
    
    // 设置HTTP响应头
    evhttp_add_header(evhttp_request_get_output_headers(req), 
                      "Content-Type", "text/html; charset=UTF-8");
    
    // 发送响应
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    
    // 释放缓冲区
    evbuffer_free(buf);
}

int main() {
    std::cout << "=== Libevent2 Demo Starting ===" << std::endl;
    std::cout << "Timer is running, HTTP server on port 8080, press Ctrl+C to exit" << std::endl;
    
    // 1. 创建event_base（Reactor实例）
    struct event_base* base = event_base_new();
    if (!base) {
        std::cerr << "Failed to create event base!" << std::endl;
        return 1;
    }
    
    // 2. 配置定时器事件
    struct event* timer = evtimer_new(base, timer_callback, event_self_cbarg());
    struct timeval tv = {1, 0}; // 1秒后首次触发
    evtimer_add(timer, &tv);
    std::cout << "[INIT] Timer event configured" << std::endl;
    
    // 3. 配置信号事件（捕获Ctrl+C）
    struct event* signal_event = evsignal_new(base, SIGINT, signal_callback, base);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        std::cerr << "Failed to create signal event!" << std::endl;
        return 1;
    }
    std::cout << "[INIT] Signal handler configured (SIGINT)" << std::endl;
    
    // 4. 创建HTTP服务器
    struct evhttp* http_server = evhttp_new(base);
    if (!http_server) {
        std::cerr << "Failed to create HTTP server!" << std::endl;
        return 1;
    }
    
    // 绑定到0.0.0.0:8080
    if (evhttp_bind_socket(http_server, "0.0.0.0", 8080) < 0) {
        std::cerr << "Failed to bind HTTP server to port 8080!" << std::endl;
        return 1;
    }
    
    // 设置通用回调函数（处理所有请求）
    evhttp_set_gencb(http_server, http_request_callback, NULL);
    std::cout << "[INIT] HTTP server listening on http://0.0.0.0:8080" << std::endl;
    
    // 5. 启动事件循环
    std::cout << "[LOOP] Starting event loop..." << std::endl;
    event_base_dispatch(base);
    
    // 6. 清理资源（执行到此处说明事件循环已退出）
    std::cout << "[CLEANUP] Freeing resources..." << std::endl;
    event_free(timer);
    event_free(signal_event);
    evhttp_free(http_server);
    event_base_free(base);
    
    std::cout << "=== Libevent2 Demo Exit ===" << std::endl;
    return 0;
}