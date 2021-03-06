#include "HttpServer.h"

namespace network
{
    HttpServer::HttpServer():event_callback_key_(this)
    {
        this->fd_ = 0;
        this->max_read_size_ = 10*1024*1024;
        this->max_write_size_ = 10*1024*1024;
        this->time_out_ = 100000;
        this->use_ipv6_ = false;
        this->listen_accept_ = false;
        event_callback_key_->hook_notify_read(&HttpServer::on_accept);
    }

    HttpServer::~HttpServer()
    {
        if (this->fd_ > 0)
        {
            if (this->poll_ && this->listen_accept_)
            {
                this->poll_->del_event_listener(this->fd_, event_read);
            }
            ::close(this->fd_);
        }
    }

    ret HttpServer::init()
    {
        return ret_success;
    }

    ret HttpServer::listen(const char* _addr, int _port, int _backlog)
    {
        if_return(0 != this->fd_, ret_fail);
        this->use_ipv6_ = false;

        struct sockaddr_in _addr_struct;
        memset(&_addr_struct, 0, sizeof(struct sockaddr_in));
        _addr_struct.sin_family = AF_INET;
        _addr_struct.sin_port = htons(_port);

        if (_addr == nullptr_t) 
        {
            _addr_struct.sin_addr.s_addr = htonl(INADDR_ANY);
        }
        else
        {
            if_return(inet_aton(_addr, &_addr_struct.sin_addr) <= 0, ret_args_error);
        }

        if (this->fd_ = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP), fd_ == -1)
        {
            perror("bind socket");
            this->fd_ = 0;
            return ret_fail;
        }

        int _opt_open_value = 1;
        setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&_opt_open_value, sizeof(int));
        setsockopt(this->fd_, SOL_SOCKET, SO_KEEPALIVE, (const void*)&_opt_open_value, sizeof(int));
        setsockopt(this->fd_, SOL_SOCKET, TCP_NODELAY, (const void*)&_opt_open_value, sizeof(int));
        fcntl(this->fd_, F_SETFL, fcntl(this->fd_, F_GETFL)|O_NONBLOCK);

        if (::bind(this->fd_, (sockaddr*)(&_addr_struct), sizeof(sockaddr)) == -1)
        {
            perror("bind error");
            ::close(this->fd_); this->fd_ = 0;
            return ret_fail;
        }

        if (::listen(this->fd_, _backlog) == -1)
        {
            perror("listen error");
            ::close(this->fd_);
            this->fd_ = 0;
            return ret_fail;
        }
        return ret_success;
    }
    
    ret HttpServer::listen_ipv6(const char* _addr, int _port, int _backlog)
    {
        if_return(0 != this->fd_, ret_fail);
        this->use_ipv6_ = true;

        struct sockaddr_in6 _addr_struct;
        memset(&_addr_struct, 0, sizeof(struct sockaddr_in6));
        _addr_struct.sin6_family = AF_INET6;
        _addr_struct.sin6_port = htons(_port);

        if (_addr == nullptr_t) 
        {
            _addr_struct.sin6_addr = in6addr_any;// IN6ADDR_ANY;
        }
        else
        {
            if_return(inet_pton(AF_INET6, _addr, &_addr_struct.sin6_addr) <= 0, ret_args_error);
        }

        if (this->fd_ = (int)socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP), fd_ == -1)
        {
            perror("bind socket");
            this->fd_ = 0;
            return ret_fail;
        }

        int _opt_open_value = 1;
        setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&_opt_open_value, sizeof(int));
        setsockopt(this->fd_, SOL_SOCKET, SO_KEEPALIVE, (const void*)&_opt_open_value, sizeof(int));
        setsockopt(this->fd_, IPPROTO_TCP, TCP_NODELAY, (const void*)&_opt_open_value, sizeof(int));
        fcntl(this->fd_, F_SETFL, fcntl(this->fd_, F_GETFL)|O_NONBLOCK);

        if (::bind(this->fd_, (sockaddr*)(&_addr_struct), sizeof(sockaddr_in6)) == -1)
        {
            perror("bind error");
            ::close(this->fd_); this->fd_ = 0;
            return ret_fail;
        }

        if (::listen(this->fd_, _backlog) == -1)
        {
            perror("listen error");
            ::close(this->fd_);
            this->fd_ = 0;
            return ret_fail;
        }
        return ret_success;
    }
    
    ret HttpServer::erase(int _fd)
    {
        _t_map_http_connection_pool::iterator _it = this->pool_.find(_fd);
        if (this->pool_.end() != _it)
        {
            this->pool_.erase(_it);
        }
        return ret_success;
    }
    
    ret HttpServer::close()
    {
        if (this->fd_ > 0)
        {
            if (this->poll_ && this->listen_accept_)
            {
                this->poll_->del_event_listener(this->fd_, event_read);
            }
            ::close(this->fd_);
        }
        this->fd_ = 0;
        return ret_success;
    }
    
    ret HttpServer::use_ipv6(bool* _use_ipv6)
    {
        if_return(nullptr_t == _use_ipv6, ret_args_error);
        *_use_ipv6 = this->use_ipv6_;
        return ret_success;
    }
    
    ret HttpServer::set_protocol(ITcpProtocol* _protocol)
    {
        return ret_fail;
    }
    
    ret HttpServer::get_protocol(ITcpProtocol** _protocol)
    {
        if_return(nullptr_t == _protocol, ret_args_error);
        *_protocol = nullptr_t;
        return ret_fail;
    }
    
    ret HttpServer::set_max_read_buffer(int _max)
    {
        this->max_read_size_ = _max;
        return ret_success;
    }
    
    ret HttpServer::set_max_write_buffer(int _max)
    {
        this->max_write_size_ = _max;
        return ret_success;
    }
    
    ret HttpServer::get_max_read_buffer(int* _max)
    {
        if_return(nullptr_t == _max, ret_args_error);
        *_max = this->max_read_size_;
        return ret_success;
    }
    
    ret HttpServer::get_max_write_buffer(int* _max)
    {
        if_return(nullptr_t == _max, ret_args_error);
        *_max = this->max_write_size_;
        return ret_success;
    }
    
    ret HttpServer::set_timeout(int _timeout)
    {
        this->time_out_ = _timeout;
        return ret_success;
    }
    
    ret HttpServer::get_timeout(int* _timeout)
    {
        if_return(nullptr_t == _timeout, ret_args_error);
        *_timeout = this->time_out_;
        return ret_success;
    }
    
    ret HttpServer::set_callback(ITcpCallback* _callback)
    {
        return ret_fail;
    }
    
    ret HttpServer::get_callback(ITcpCallback** _callback)
    {
        if_return(nullptr_t == _callback, ret_args_error);
        *_callback = nullptr_t;
        return ret_fail;
    }
    
    ret HttpServer::set_http_callback(IHttpCallback* _callback)
    {
        this->http_callback_ = _callback;
        return ret_success;
    }
    
    ret HttpServer::get_http_callback(IHttpCallback** _callback)
    {
        if_return(nullptr_t == _callback, ret_args_error);
        *_callback = nullptr_t;
        if (nullptr_t != this->http_callback_)
        {
            return this->http_callback_->convert(_type_id(IHttpCallback), (void**)_callback);
        }
        return ret_fail;
    }
    
    ret HttpServer::wait_for_event(IEventPoll* _poll)
    {
        if_return(nullptr_t == _poll, ret_args_error);
        if_return(this->listen_accept_, ret_success);

        this->poll_ = _poll;
        if (this->fd_ > 0)
        {
            this->listen_accept_ = __success(this->poll_
                ->set_event_listener(this->fd_, event_read, this->event_callback_key_));
        }
        return ret_success;
    }
    
    ret HttpServer::on_accept(int _fd, IEventPoll* poll)
    {
        if_return(nullptr_t == poll, ret_args_error);
        if_return(_fd != this->fd_ || nullptr_t == poll, ret_args_error);

        int _client_fd =0;
        if (this->use_ipv6_)
        {
            struct sockaddr_in6 _client_addr;
            socklen_t _socket_len = sizeof(sockaddr_in);
            _client_fd = ::accept(_fd, (sockaddr*)&_client_addr, &_socket_len);
        }
        else
        {
            struct sockaddr_in _client_addr;
            socklen_t _socket_len = sizeof(sockaddr_in);
            _client_fd = ::accept(_fd, (sockaddr*)&_client_addr, &_socket_len);
        }

        if_return(_client_fd < 0 || (-1 == _client_fd && (errno != EAGAIN 
                && errno != ECONNABORTED && errno != EPROTO && errno != EINTR)), callback_complete);

        __sp<IHttpConnection> _http_connection;
        let_object_with_implement<IHttpConnection, HttpConnection, __rtns>(&_http_connection);

        do
        {
            if_break(!_http_connection);
            if_break(!__success(_http_connection->init()));
            if_break(!__success(_http_connection->follow_fd(_client_fd, __get_i_entity<IHttpServer>())));
            if_break(!__success(_http_connection->wait_for_event(poll)));
            this->pool_[_client_fd] = _http_connection;
            return callback_continue;
        } while(0);
        
        ::close(_client_fd);
        return callback_continue;
    }
}