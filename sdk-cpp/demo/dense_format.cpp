/***************************************************************************
 * 
 * Copyright (c) 2018 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file demo.cpp
 * @author wanlijin01(wanlijin01@baidu.com)
 * @date 2018/07/09 20:12:44
 * @brief 
 *  
 **/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include <fstream>
#include "predictor_sdk.h"
#include "dense_service.pb.h"
#include "builtin_format.pb.h"

using baidu::paddle_serving::sdk_cpp::Predictor;
using baidu::paddle_serving::sdk_cpp::PredictorApi;
using baidu::paddle_serving::predictor::dense_service::Request;
using baidu::paddle_serving::predictor::dense_service::Response;
using baidu::paddle_serving::predictor::format::DensePrediction;
using baidu::paddle_serving::predictor::format::DenseInstance;

int create_req(Request& req) {
    DenseInstance *ins = req.mutable_instances()->Add();
    ins->add_features(1.5);
    ins->add_features(16.0);
    ins->add_features(14.0);

    ins = req.mutable_instances()->Add();
    ins->add_features(1.0);
    ins->add_features(2.0);
    ins->add_features(3.0);
    return 0;
}

void print_res(
        const Request& req,
        const Response& res,
        std::string route_tag,
        uint64_t elapse_ms) {

    for (uint32_t i = 0; i < res.predictions_size(); ++i) {
        const DensePrediction &prediction = res.predictions(i);
        std::ostringstream oss;
        for (uint32_t j = 0; j < prediction.categories_size(); ++j) {
            oss << prediction.categories(j) << " ";
        }
        LOG(INFO) << "Receive result " << oss.str();
    }

    LOG(INFO) 
        << "Succ call predictor[dense_format], the tag is: " 
        << route_tag << ", elapse_ms: " << elapse_ms;
}

int main(int argc, char** argv) {
    PredictorApi api;

    // initialize logger instance
    struct stat st_buf;
    int ret = 0;
    if ((ret = stat("./log", &st_buf)) != 0) {
            mkdir("./log", 0777);
            ret = stat("./log", &st_buf);
            if (ret != 0) {
                    LOG(WARNING) << "Log path ./log not exist, and create fail";
                    return -1;
            }
    }
    FLAGS_log_dir = "./log";
    google::InitGoogleLogging(strdup(argv[0]));
     
    if (api.create("./conf", "predictors.prototxt") != 0) {
        LOG(ERROR) << "Failed create predictors api!"; 
        return -1;
    }

    Request req;
    Response res;

    api.thrd_initialize();

    while (true) {
        timeval start;
        gettimeofday(&start, NULL);

        api.thrd_clear();

        Predictor* predictor = api.fetch_predictor("dense_service");
        if (!predictor) {
            LOG(ERROR) << "Failed fetch predictor: echo_service"; 
            return -1;
        }

        req.Clear();
        res.Clear();

        if (create_req(req) != 0) {
            return -1;
        }

        butil::IOBufBuilder debug_os;
        if (predictor->debug(&req, &res, &debug_os) != 0) {
            LOG(ERROR) << "failed call predictor with req:"
                        << req.ShortDebugString();
            return -1;
        }

        butil::IOBuf debug_buf;
        debug_os.move_to(debug_buf);
        LOG(INFO) << "Debug string: " << debug_buf;

        timeval end;
        gettimeofday(&end, NULL);

        uint64_t elapse_ms = (end.tv_sec * 1000 + end.tv_usec / 1000)
            - (start.tv_sec * 1000 + start.tv_usec / 1000);
    
        print_res(req, res, predictor->tag(), elapse_ms);
        res.Clear();

        usleep(50);

    } // while (true)

    api.thrd_finalize();
    api.destroy();

    google::ShutdownGoogleLogging();

    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */