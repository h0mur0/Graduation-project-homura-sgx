#include <vector>
#include <iostream>
#include <type_traits>
#include "channel.h"
#include "Enclave_u.h"
#include "sgx_eid.h"

using namespace std;
extern sgx_enclave_id_t global_eid; 

// 构造函数实现
channel::channel() {}

// leader 到 database 的数据传输
void channel::leader_to_database(leader& ld, database& db) {
    db.database_recv_from_leader = ld.leader_send[db.client_id][db.database_id];
}

/*
// client 到 database 的数据传输
void channel::client_to_database(client& cl, database& db) {
    db.database_recv_from_client = cl.client_send_to_database[db.database_id];    

// database 间传输    
void channel::database_to_database(database& db1, database& db2) {
    db2.database_recv_from_database.push_back(db1.database_send_to_database);
}
} */

void channel::client_to_database(client& cl, database& db) {
    vector<int> sends = cl.client_send_to_database[db.database_id];
    vector<int> rv(sends.size());
    sgx_status_t st = ecall_security_channel(
        global_eid,
        sends.data(),  // 传入send 首地址
        sends.size(),
        rv.data()
    );
    if (st != SGX_SUCCESS) abort();
    db.database_recv_from_client = rv;
}

void channel::database_to_database(database& db1, database& db2) {
    vector<int> rv(db1.database_send_to_database.size());
    ecall_security_channel(global_eid, db1.database_send_to_database.data(), db1.database_send_to_database.size(), rv.data());
    db2.database_recv_from_database.push_back(rv);
}

// database 到 leader 的数据传输
void channel::database_to_leader(database& db, leader& ld) {
    ld.leader_recv[db.client_id][db.database_id] = db.database_send_to_leader;
}
