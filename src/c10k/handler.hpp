//
// Created by lz on 1/27/17.
//

#ifndef C10K_SERVER_HANDLER_HPP
#define C10K_SERVER_HANDLER_HPP

#include <memory>
#include <functional>
#include <type_traits>
namespace c10k
{
    // interface for handler
    struct Handler: std::enable_shared_from_this<Handler>
    {
        virtual void handle_init(const ConnectionPtr &conn) = 0;
        virtual ~Handler() {}
    };

#define C10K_READ_ASYNC_THEN_MEMFUN(Conn_, OIt_, Len_, MemFunName_) \
    Conn_->read_async_then(OIt_, Len_, \
    std::bind(&std::remove_pointer_t<decltype(this)>::MemFunName_, \
            std::static_pointer_cast<std::remove_pointer_t<decltype(this)>>(shared_from_this()), _1, _2, _3))

#define C10K_WRITE_ASYNC_THEN_MEMFUN(Conn_, IIt1_, IIt2_, MemFunName_) \
    Conn_->write_async_then(IIt1_, IIt2_, \
    std::bind(&std::remove_pointer_t<decltype(this)>::MemFunName_, \
    std::static_pointer_cast<std::remove_pointer_t<decltype(this)>>(shared_from_this()), _1))
}

#endif //C10K_SERVER_HANDLER_HPP
