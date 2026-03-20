import socket
import threading
import time

def simulate_client(client_id):
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect(('127.0.0.1', 20000))
        
        for i in range(10):
            msg = f"我是客户端 {client_id}，这是第 {i+1} 条消息"
            client.send(msg.encode('utf-8'))
            response = client.recv(1024).decode('utf-8')
            print(f"客户端 {client_id} 收到回显: {response}")
            time.sleep(0.1) # 模拟间隔发送
            
        client.close()
    except Exception as e:
        print(f"客户端 {client_id} 出错: {e}")

# 模拟 5 个客户端并发
threads = []
for i in range(10000):
    t = threading.Thread(target=simulate_client, args=(i,))
    threads.append(t)
    t.start()

for t in threads:
    t.join()