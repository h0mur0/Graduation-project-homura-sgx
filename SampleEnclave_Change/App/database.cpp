#include <iostream>
#include <vector>
#include <cmath>
#include <string>

#include "database.h"
#include "Enclave_u.h"
#include "sgx_eid.h"

using namespace std;
extern sgx_enclave_id_t global_eid; 

// 构造函数实现
database::database(int client_id, int database_id, string role, string state, vector<vector<vector<int>>>& incidence_vectors)
    :client_id(client_id), database_id(database_id), role(role), state(state), incidence_vectors(incidence_vectors) {
    }

void database::create_and_send_relatived_randomness(int L, int b, int M, int N) {
    if (role == "base") {
        return;
    }
    if (state == "normal") {
        relatived_randomness = generate_random_vector(L,b);
        database_send_to_database = relatived_randomness;
    }
    else {
        relatived_randomness = vector<int>(b,L - (M - 1));
        for (auto& rand : database_recv_from_database) {
            for (int ell = 0; ell < b; ell++) {
                relatived_randomness[ell] = (relatived_randomness[ell] - rand[ell]) % L;
            }
        }
    }
}

// 创建并发送回复
void database::create_and_send_reply(int L, int b, int N) {
    location_randomness = database_recv_from_client;
    if (role == "not base") {
        vector<int> tmp;
        for (int ell = 0; ell < b; ell++) {
            int reply = global_randomness * (dot_product(incidence_vectors[0][ell], database_recv_from_leader[0][ell]) + location_randomness[ell] + relatived_randomness[ell]) % L;
            tmp.push_back(reply);
        }
        database_send_to_leader.push_back(tmp);
    }
    else {

        database_send_to_leader.push_back(vector<int>(0));       
        for (int j = 1; j < N; j++) {
            vector<int> tmp; 
            for (int ell = 0; ell < b; ell++) {
                int reply = global_randomness * (dot_product(incidence_vectors[j][ell], database_recv_from_leader[j][ell]) + location_randomness[ell]) % L;
                tmp.push_back(reply);
            }
            database_send_to_leader.push_back(tmp);
        }        
    }
    
}
