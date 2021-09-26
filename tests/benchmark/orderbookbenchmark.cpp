#include <benchmark/benchmark.h>
#include <vector>
#include <string>
#include <random>
#include <boost/asio.hpp>

#include "orderbookmanager.hpp"

// naive orderbook 'micro' benchmarking
// does not benchmark order entry server performance as a whole

using namespace server::tradeorder;
using namespace ::tradeorder;

constexpr uint64_t NUM_ORDERS = 100000;
constexpr uint8_t BID_SIDE = 1;
constexpr uint8_t ASK_SIDE = 1;
static uint64_t ORDER_IDS = 0;
static OrderBookManager bench_manager(nullptr);
static std::vector<tradeorder::Order> adds;
static std::vector<info::ModifyOrder> mods;
static std::vector<info::CancelOrder> cancels;


static void setupCancelOrders(OrderBookManager& m, std::vector<info::CancelOrder>& cancels) {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist10(1, 10);
    for (uint64_t i = 0; i < NUM_ORDERS / 2; ++i) {
        uint64_t instrument = i % 100;
        if (i < (NUM_ORDERS / 5)) { // 20% of cancels will result in level data-structure erasure
            Order tempone(ASK_SIDE, nullptr, i + 160, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
            m.addOrder(tempone);
            cancels.emplace_back(ORDER_IDS - 1, i, instrument, nullptr);
            Order temptwo(ASK_SIDE, nullptr, i + 160, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
            m.addOrder(temptwo);
            cancels.emplace_back(ORDER_IDS - 1, i, instrument, nullptr);
        }
        else {
            int num_iterations = dist10(rng); // want to randomise the position of our orders that must be cancelled
            for (int i = 0; i < num_iterations; ++i) {
                Order bid_temp(BID_SIDE, nullptr, 50, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
                m.addOrder(bid_temp);
                Order ask_temp(ASK_SIDE, nullptr, 150, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
                m.addOrder(ask_temp);
            } 

            Order tempone(BID_SIDE, nullptr, 50, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
            m.addOrder(tempone);
            cancels.emplace_back(ORDER_IDS - 1, i, instrument, nullptr);
            Order temptwo(ASK_SIDE, nullptr, 150, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
            m.addOrder(temptwo);
            cancels.emplace_back(ORDER_IDS - 1, i, instrument, nullptr);

            for (int i = 0; i < 10 - num_iterations; ++i) {
                Order bid_temp(BID_SIDE, nullptr, 50, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
                m.addOrder(bid_temp);
                Order ask_temp(ASK_SIDE, nullptr, 150, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
                m.addOrder(ask_temp);
            }
        }
    }
}

static void setupAddOrders(OrderBookManager& m, std::vector<Order>& adds) {
    for (uint64_t i = 0; i < (NUM_ORDERS / 10); ++i) {
        uint64_t instrument = i % 100; // send same orders to 100 different instruments
        // NUM_ORDERS / 10 orders of same pattern testing key add order behaviour
        adds.emplace_back(BID_SIDE, nullptr, 98, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(ASK_SIDE, nullptr, 103, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(BID_SIDE, nullptr, 100, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(ASK_SIDE, nullptr, 100, 50, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(BID_SIDE, nullptr, 101, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(ASK_SIDE, nullptr, 100, 50, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(BID_SIDE, nullptr, 99, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(ASK_SIDE, nullptr, 101, 50, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(BID_SIDE, nullptr, 102, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
        adds.emplace_back(BID_SIDE, nullptr, 98, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
    }
}

static void setupModifyOrders(OrderBookManager& m, std::vector<info::ModifyOrder>& modifys) {
    for (uint64_t i = 0; i < NUM_ORDERS / 2; ++i) {
        uint64_t instrument = i % 100;
        if (i < NUM_ORDERS / 5) {
            Order bid_temp(BID_SIDE, nullptr, 40, 50, info::OrderCommon(ORDER_IDS++, i, instrument));
            m.addOrder(bid_temp);
            modifys.emplace_back(BID_SIDE, nullptr, 40, 200, info::OrderCommon(ORDER_IDS - 1, i, instrument));

            Order ask_temp(ASK_SIDE, nullptr, 140, 50, info::OrderCommon(ORDER_IDS++, i, instrument));
            m.addOrder(ask_temp);
            modifys.emplace_back(ASK_SIDE, nullptr, 140, 200, info::OrderCommon(ORDER_IDS - 1, i, instrument));
        }
        else {
            Order bid_temp(BID_SIDE, nullptr, 40, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
            m.addOrder(bid_temp);
            modifys.emplace_back(BID_SIDE, nullptr, 30, 100, info::OrderCommon(ORDER_IDS - 1, i, instrument));

            Order ask_temp(ASK_SIDE, nullptr, 140, 100, info::OrderCommon(ORDER_IDS++, i, instrument));
            m.addOrder(ask_temp);
            modifys.emplace_back(ASK_SIDE, nullptr, 130, 100, info::OrderCommon(ORDER_IDS - 1, i, instrument));
        }
    }
}

static void setupOrdersAndBook(OrderBookManager& m, std::vector<Order>& adds, 
std::vector<info::CancelOrder>& cancels, std::vector<info::ModifyOrder>& modifys) {
    for (uint64_t i = 0; i < 100; ++i) {
        m.createOrderBook(i); // 100 instruments
    }
    setupAddOrders(m, adds);
    setupCancelOrders(m, cancels);
    setupModifyOrders(m, modifys);
}

static void BM_OrderBook(benchmark::State& state) {
    for (auto arg : state) {
        for (uint64_t i = 0; i < NUM_ORDERS; ++i) {
            bench_manager.addOrder(adds[i]);
            bench_manager.modifyOrder(mods[i]);
            bench_manager.cancelOrder(cancels[i]);
        }
    }
}

BENCHMARK(BM_OrderBook)->Iterations(100);

int main(int argc, char* argv[]) {
    setupOrdersAndBook(bench_manager, adds, cancels, mods);
    ::benchmark::Initialize(&argc, argv);
    ::benchmark::RunSpecifiedBenchmarks();
}
