.
├── src
│   ├── server
│   │   ├── exchange_simulator.cpp
│   │   └── client_manager.cpp
│   ├── client
│   │   └── feed_handler.cpp
│   └── common
│       └── protocol.h
├── scripts
│   ├── build.sh
│   ├── run_server.sh
│   └── run_client.sh
├── build
├── docs
│   ├── DESIGN.md
│   ├── GBM.md
│   ├── NETWORK.md
│   ├── PERFORMANCE.md
│   └── QUESTIONS.md
└── README.md

# Make scripts executable
chmod +x scripts/*.sh

# Build server and client binaries
./scripts/build.sh

# Usage: ./scripts/run_server.sh <PORT>
./scripts/run_server.sh 9876