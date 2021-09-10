#ifndef TRADE_SERVER_HPP
#define TRADE_SERVER_HPP

#include <memory>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <list>
#include <mutex>
#include <grpcpp/grpcpp.h>
#include <thread>

#include "logger.hpp"
#include "ordermanager.hpp"
#include "bidirstreamrpcjob.hpp"
#include "unaryrpcjob.hpp"
#include "orderentry.grpc.pb.h"
#include "jobhandlers.hpp"
#include "rpcprocessor.hpp"

using ServiceType = orderentry::OrderEntryService::AsyncService;
using OERequestType = orderentry::OrderEntryRequest;
using OEResponseType = orderentry::OrderEntryResponse;

namespace server {
class TradeServer {
public:
    TradeServer(char* port, const std::string& filename);
    ~TradeServer();
private:
    void handleRemoteProcedureCalls();
    struct OrderEntryResponder {
        std::function<bool(OEResponseType*)> send_resp;
        grpc::ServerContext* server_context;
    };
    void createOrderEntryRPC();
    void makeOrderEntryRPC();
    static void setOrderEntryContext(
        ServiceType* service, 
        RPCJob* job, 
        grpc::ServerContext* serv_context, 
        std::function<bool(OEResponseType*)> response
    );
    static void orderEntryDone(ServiceType* service, RPCJob* job, bool);
    static void orderEntryProcessor(RPCJob* job, const OERequestType* order_entry);
    void makeMarketDataRPC();

    logging::Logger logger_;
    std::mutex taglist_mutex_;
    std::condition_variable condv_;
    tradeorder::OrderManager ordermanager_;
    std::unique_ptr<grpc::Server> trade_server_;
    std::unique_ptr<grpc::ServerCompletionQueue> cq_;
    orderentry::OrderEntryService::AsyncService order_entry_service_;
    std::list<RPC::CallbackTag> taglist_;
    RPC::RPCProcessor rpc_processor_;
    static std::unordered_map<RPCJob*, OrderEntryResponder> entry_order_responders_;
};
}

#endif
