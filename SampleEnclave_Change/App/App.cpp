/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>
# define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "App.h"
#include "Enclave_u.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory> 
#include <chrono> 
#include <algorithm>
#include <unordered_set>
#include <ctime>
#include <iomanip>
#include "sgx_eid.h"
#include "client.h"
#include "leader.h"
#include "database.h"
#include "channel.h"
#include "public_function.h"

using namespace std;

int M;  
int leader_id;  
int sp_id;
vector<string> fileNames; 
vector<int> N;  
vector<int> Sk; 
map<string, int> data2Sk; 
vector<vector<int>> P(fileNames.size()); 
int K; 
int b;
vector<client> clients; 
vector<vector<database>> databases; 
shared_ptr<leader> ld; 
int L;  
int eta;
shared_ptr<channel> chan;
int com_bit = 0;
double cul_time = 0.0;

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
    {
        SGX_ERROR_MEMORY_MAP_FAILURE,
        "Failed to reserve memory for the enclave.",
        NULL
    },
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret)
{
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist/sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if(ret == sgx_errlist[idx].err) {
            if(NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }
    
    if (idx == ttl)
    	printf("Error code is 0x%X. Please refer to the \"Intel SGX SDK Developer Reference\" for more details.\n", ret);
}

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void)
{
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

/* OCall functions */

void pre_processing() {
    leader_id = M - 1;  // 选择领导者
    sp_id = M - 2;
    eta = (K - 1 + N[0] - 2) / (N[0] - 1);
    b = (3 * eta + 1) / 2;

    //执行Cuckoo哈希
    vector<CuckooHashTableProducer> producer_tables(N[0],CuckooHashTableProducer(b));
    vector<CuckooHashTableConsumer> consumer_tables(N[0],CuckooHashTableConsumer(b));
    vector<vector<int>> schedule_hash_tables(N[0]);
    vector<vector<vector<int>>> index_hash_tables(N[0]);
    vector<vector<vector<int>>> pre_query_vectors(N[0],vector<vector<int>>(b,vector<int>(3,0)));
    unordered_set<int> pset(P[leader_id].begin(), P[leader_id].end());
    map<int,vector<int>> element_to_position;
    vector<vector<vector<vector<int>>>> incidence_vectors(M,vector<vector<vector<int>>>(N[0],vector<vector<int>>(b,vector<int>(3,0))));
    auto st_time_1 = chrono::high_resolution_clock::now();
    // 生成Cuckoo哈希表
    for (int j = 1; j < N[0]; j++){
        int start = (j - 1) * eta;
        int end = j * eta;
        for (int k = start; k < end; k++) {
            producer_tables[j].insert(k);
            consumer_tables[j].insert(k);
        }
        schedule_hash_tables[j] = consumer_tables[j].table;
        index_hash_tables[j] = producer_tables[j].table;
    }
    auto st_time_2 = chrono::high_resolution_clock::now();
    auto duration_2 = chrono::duration_cast<std::chrono::microseconds>(st_time_2 - st_time_1);
    // cout << duration_2.count() << endl;
    // 领导方预先生成pre-query vector    
    for (int j = 1; j < N[0]; j++) {
        for (int ell = 0; ell < b; ell++) {
                int tmp = schedule_hash_tables[j][ell];
                if (pset.count(tmp)) {
                    int k = find(index_hash_tables[j][ell].begin(),index_hash_tables[j][ell].end(),tmp) - index_hash_tables[j][ell].begin();
                    pre_query_vectors[j][ell][k] = 1;  
                    element_to_position[tmp] = {j,ell};              
            }
        }
    }    
    auto st_time_3 = chrono::high_resolution_clock::now();
    auto duration_3 = chrono::duration_cast<std::chrono::microseconds>(st_time_3 - st_time_2);
    // cout << duration_3.count() << endl;
    // 客户方预先生成incidence vector
    for (int i = 0; i < M - 1; i++) {
        std::unordered_set<int> pset(P[i].begin(), P[i].end());
    
        for (int j = 1; j < N[0]; j++) {
            for (int ell = 0; ell < b; ell++) {
                for (int k = 0; k < 3; k++) {
                    int tmp = index_hash_tables[j][ell][k];
                    if (pset.count(tmp)) {
                        incidence_vectors[i][j][ell][k] = 1;
                    }
                }
            }
        }  
    } 
    auto st_time_4 = chrono::high_resolution_clock::now();
    auto duration_4 = chrono::duration_cast<std::chrono::microseconds>(st_time_4 - st_time_3);
    // cout << duration_4.count() << endl;
    // 创建客户端、领导者和数据库
    for (int i = 0; i < M; i++) {
        if (i == leader_id) {
            ld = make_shared<leader>(schedule_hash_tables, pre_query_vectors,element_to_position, i, M, N[0], b);  // 创建领导者
        } 
        else {
            string state = (i == sp_id) ? "special" : "normal";
            client one_client(state, i,N[0]);  
            clients.push_back(one_client);
            vector<database> tmp_db;
            for (int j = 0; j < N[0]; j++) {
                if (j == 0){
                    database one_database(i, j, "base",state, incidence_vectors[i]);  
                    tmp_db.push_back(move(one_database));
                }
                else{
                    vector<vector<vector<int>>> tmp = {incidence_vectors[i][j]};
                    database one_database(i, j, "not base",state, tmp); 
                    tmp_db.push_back(move(one_database));
                }               
            }
            databases.push_back(move(tmp_db));
        }        
    }
    // 创建 channel 的智能指针
    chan = make_shared<channel>();
    auto st_time_5 = chrono::high_resolution_clock::now();
    auto duration_5 = chrono::duration_cast<std::chrono::microseconds>(st_time_5 - st_time_4);
    // cout << duration_5.count() << endl;
}
// 初始化
void initial(){
    L = select_L(M);  // 选择一个大于 M 的素数 L
}
// 查询阶段
void query() {
    ld->create_and_send_query(M,N[0],b,L);  // 领导者生成查询并发送
    for (int i = 0; i < M - 1; i++) {
        for (int j = 0; j < N[0]; j++){
            chan->leader_to_database(*ld, databases[i][j]);  // 领导者将查询传递给数据库
        }
    }
}

// 随机数生成阶段
void create_randomness() {
    // 本地随机数
    for (int i = 0; i < M - 1; i++) {
        clients[i].create_and_send_local_randomness(L, b, N[0]);
        for(int j = 0; j < N[0]; j++){
            chan->client_to_database(clients[i], databases[i][j]);  // 客户端发送给数据库
        }
    }
    // 相关随机数
	
    //普通数据库
    for (int i = 0; i < M-2; i++) {
        for (int j = 0; j < N[0]; j++){        
            databases[i][j].create_and_send_relatived_randomness(L, b, M, N[0]);
            chan->database_to_database(databases[i][j], databases[M-2][j]);
        }
    }
    //特殊数据库
    for (int j = 0; j < N[0]; j++) {
        databases[M-2][j].create_and_send_relatived_randomness(L, b, M, N[0]);
    }
    // 全局随机数
    int c = rand() % (L - 1) + 1;  // 随机生成系数 c
    for (int i = 0; i < M-1; i++) {
        for (int j = 0; j < N[0]; j++){        
            databases[i][j].global_randomness = c;
        }
    }
}

// 数据库生成回复并发送给领导者
void reply() {
    for (int i = 0; i < M-1; i++){
        for (int j = 0; j < N[0]; j++) {
            databases[i][j].create_and_send_reply(L, b, N[0]);  // 生成并发送回复
            chan->database_to_leader(databases[i][j], *ld);  // 数据库将回复发送给领导者
        }
    }
}

// 获取交集
vector<int> get_intersection() {
    return ld->calculate_intersection(M, L);  // 计算交集
}

void ocall_print_string(const char *str)
{
    /* Proxy/Bridge will check the length and null-terminate 
     * the input string to prevent buffer overflow. 
     */
    printf("%s", str);
}


/* Application entry */
int SGX_CDECL main(int argc, char *argv[])
{
    (void)(argc);
    (void)(argv);


    /* Initialize the enclave */
    if(initialize_enclave() < 0){
        printf("Enter a character before exit ...\n");
        getchar();
        return -1; 
    }
 
    /* Utilize edger8r attributes */
    edger8r_array_attributes();
    edger8r_pointer_attributes();
    edger8r_type_attributes();
    edger8r_function_attributes();
    
    /* Utilize trusted libraries */
    ecall_libc_functions();
    ecall_libcxx_functions();
    ecall_thread_functions();
    /* ci chu bian xie dai ma */
    // ecall_generate_and_distribute(global_eid);
	time_t now = time(nullptr);        // 获取当前时间戳
    tm* local_time = localtime(&now);  // 转换为本地时间结构体
    ofstream outFile("output_PBC.txt",ios::app);
    outFile << "------------------------- "<< endl;
    outFile << "运行时间：" << put_time(local_time, "%Y-%m-%d %H:%M:%S") << endl;
    auto st_time_1 = chrono::high_resolution_clock::now();
    parse_args(argc,argv,M,fileNames,N);
    encode(fileNames, Sk, data2Sk, P, K);
    outFile << "领导方集合大小：" << P[M-1].size() << endl;
    outFile << "全集大小：" << K << endl;
    auto st_time_2 = chrono::high_resolution_clock::now();
    auto duration_2 = chrono::duration_cast<std::chrono::microseconds>(st_time_2 - st_time_1);
    outFile << "编码阶段用时：" << duration_2.count() << "微秒" << endl;
    pre_processing();
    auto st_time_3 = chrono::high_resolution_clock::now();
    auto duration_3 = chrono::duration_cast<std::chrono::microseconds>(st_time_3 - st_time_2);
    outFile << "预处理阶段用时：" << duration_3.count() << "微秒" << endl;
    initial();  // 初始化
    auto st_time_f1 = chrono::high_resolution_clock::now();
    auto duration_f1 = chrono::duration_cast<std::chrono::microseconds>(st_time_f1 - st_time_3);
    outFile << "初始化阶段用时：" << duration_f1.count() << "微秒" << endl;
    query();  // 查询阶段
    auto st_time_f2 = chrono::high_resolution_clock::now();
    auto duration_f2 = chrono::duration_cast<std::chrono::microseconds>(st_time_f2 - st_time_f1);
    outFile << "查询阶段用时：" << duration_f2.count() << "微秒" << endl;
    create_randomness();  // 生成并发送随机数
    auto st_time_f3 = chrono::high_resolution_clock::now();
    auto duration_f3 = chrono::duration_cast<std::chrono::microseconds>(st_time_f3 - st_time_f2);
    outFile << "随机数生成阶段用时：" << duration_f3.count() << "微秒" << endl;
    reply();  // 数据库生成并发送回复
    auto st_time_f4 = chrono::high_resolution_clock::now();
    auto duration_f4 = chrono::duration_cast<std::chrono::microseconds>(st_time_f4 - st_time_f3);
    outFile << "响应阶段用时：" << duration_f4.count() << "微秒" << endl;
    vector<int> intersection = get_intersection();  // 获取交集
    auto st_time_f5 = chrono::high_resolution_clock::now();
    auto duration_f5 = chrono::duration_cast<std::chrono::microseconds>(st_time_f5 - st_time_f4);
    outFile << "计算阶段用时：" << duration_f5.count() << "微秒" << endl;
    vector<string> intersection_string;
    decode(intersection,data2Sk,intersection_string);
    auto st_time_f6 = chrono::high_resolution_clock::now();
    auto duration_f6 = chrono::duration_cast<std::chrono::microseconds>(st_time_f6 - st_time_f5);
    outFile << "解码阶段用时：" << duration_f6.count() << "微秒" << endl;
    auto st_time_4 = chrono::high_resolution_clock::now();
    auto duration_4 = chrono::duration_cast<std::chrono::microseconds>(st_time_4 - st_time_3);
    outFile << "交集大小：" << intersection.size() << endl;
    outFile << "主要代码用时：" << duration_4.count() << "微秒" << endl;
    outFile << "总通信开销：" << com_bit << endl;
    auto duration_5 = chrono::duration_cast<std::chrono::microseconds>(st_time_4 - st_time_1);
    outFile << "总代码用时：" << duration_5.count() << "微秒" << endl;
    // 打印交集结果
    cout <<"交集大小："<< intersection.size() << endl;
    cout << "intersection is:";
    for (string elem : intersection_string) {
        cout << elem << endl;
    }
    cout << endl;

    /* Destroy the enclave */
    sgx_destroy_enclave(global_eid);
    
    printf("Info: SampleEnclave successfully returned.\n");
    
    printf("Enter a character before exit ...\n");
    getchar();
    return 0;
}

