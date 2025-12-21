#include "exchange_simulator.h"


void run_exchange(int port) {
    ExchangeSimulator sim(port, 100);
    sim.set_tick_rate(10000);
    sim.enable_fault_injection(false);
    sim.start();
}

int main(int argc, char* argv[]) {
    int port = 9876; // default

    if (argc >= 3 && std::string(argv[1]) == "--port") {
        port = std::atoi(argv[2]);
    }

    std::cout << "[server] Starting exchange on port " << port << "\n";
    run_exchange(port);
}

// int main() {
//     ExchangeSimulator sim(9000, 100); // port 9000, 100 symbols
//     sim.set_tick_rate(10000);          // 10k ticks/sec
//     sim.start();
//     return 0;
// }