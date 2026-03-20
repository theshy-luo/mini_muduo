#pragma once

#include "mini_muduo/Buffer.h"
#include "mini_muduo/Callbacks.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/InetAddress.h"
#include "mini_muduo/Socket.h"
#include "mini_muduo/base/noncopyable.h"
#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

/**
 * TcpConnection 用于打包一个已建立的连接
 * 它是 Channel 的拥有者，负责 Channel 的回调处理
 */
namespace mini_muduo {
class TcpConnection : public Noncopyable,
                      public std::enable_shared_from_this<TcpConnection> {
public:
  TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *GetLoop() const { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &local_addr() const { return local_addr_; }
  const InetAddress &peer_addr() const { return peer_addr_; }
  bool Connected() const { return state_ == kConnected; }

  // 发送数据（这是个重头戏，需要处理发送不全的情况）
  void send(const void *message, size_t len);
  void send(const std::string &buf);
  void send(Buffer *buf);

  // 关闭连接（温和的关闭，会等数据发完）
  void ShutDown();

  // 设置回调
  void SetConnectionCallback(const ConnectionCallback &cb) { connection_callback_ = cb; }
  void SetCloseCallback(const CloseCallback &cb) { close_callback_ = cb; }
  void SetWriteCompleteCallback(const WriteCompleteCallback &cb) { write_complete_callback_ = cb; }
  void SetMessageCallback(const MessageCallback &cb) { message_callback_ = cb; }

  // 核心入口：当 TcpServer 决定让这个连接上线时调用
  void ConnectEstablished();

  // 核心入口：当连接要被彻底销毁时调用
  void ConnectDestroyed();

  void SetContext(std::shared_ptr<void> context){context_ = context;}
  std::shared_ptr<void> GetContext() const { return context_; }

private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

  // Channel 绑定的四个核心处理函数，由 TcpConnection 内部处理
  void HandleRead(Timestamp receive_time);
  void HandleWrite();
  void HandleClose();
  void HandleError();

  void SetState(StateE s) { state_ = s; }

  EventLoop *loop_;
  std::string name_;
  std::atomic<StateE> state_;

  // 资源拥有者
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;

  const InetAddress local_addr_;
  const InetAddress peer_addr_;

  // 回调函数
  ConnectionCallback connection_callback_;
  CloseCallback close_callback_; // 它是 TcpServer 传进来的清理函数
  WriteCompleteCallback write_complete_callback_;
  MessageCallback message_callback_;

  // 接收缓冲区
  Buffer input_buffer_;
  // 发送缓冲区
  Buffer output_buffer_;

  std::shared_ptr<void> context_;
};
} // namespace mini_muduo