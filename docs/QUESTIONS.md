# Critical Thinking Questions

### 1. How do you efficiently broadcast to multiple clients without blocking?

- Use **non-blocking sockets** with `MSG_DONTWAIT`.
- Maintain a **client list** and attempt send for each.
- If a client buffer is full, skip it temporarily to avoid blocking the main tick loop.
- Optionally, implement **per-client queues** for backpressure handling.

### 2. What happens when a client's TCP send buffer fills up?

- `send()` returns `EAGAIN` or `EWOULDBLOCK`.
- The server skips sending data to that client for the current tick.
- Client may lag behind in receiving updates.
- Slow consumers are isolated to protect overall throughput.

### 3. How do you ensure fair distribution when some clients are slower?

- Keep all sends **non-blocking**.
- Do not let slow clients block fast ones.
- Optionally implement **ring buffers or per-client queues** to store latest ticks.
- Periodically drop oldest messages for slow clients to catch up.

### 4. How would you handle 1000+ concurrent client connections?

- Use **epoll** for scalable event handling.
- Non-blocking I/O for all sockets.
- Single-threaded or sharded broadcasting to avoid contention.
- Lock-free data structures to avoid mutex overhead.
- Monitor system limits (`ulimit -n`) and optimize network stack (`SO_SNDBUF`, TCP_NODELAY).



