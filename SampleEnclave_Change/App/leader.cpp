#include <iostream>
#include <vector>
#include <map>

#include "leader.h"
#include <numeric>
#include <cmath>
#include <chrono> // 包含计时功能
#include <algorithm>
#include "sgx_eid.h"

using namespace std;
extern sgx_enclave_id_t global_eid; 

// 构造函数实现
leader::leader(std::vector<std::vector<int>> schedule_hash_tables, std::vector<std::vector<std::vector<int>>> pre_query_vectors, map<int,vector<int>> element_to_position, int leader_id, int M, int N, int b)
    : schedule_hash_tables(schedule_hash_tables), pre_query_vectors(pre_query_vectors), leader_id(leader_id), element_to_position(element_to_position) {
        leader_send = vector<vector<vector<vector<vector<int>>>>>(M,vector<vector<vector<vector<int>>>>(N,vector<vector<vector<int>>>(N,vector<vector<int>>(b,vector<int>(3,0)))));        
        leader_recv = vector<vector<vector<vector<int>>>>(M,vector<vector<vector<int>>>(N,vector<vector<int>>(b,vector<int>(3,0))));
    }

// 创建并发送查询
void leader::create_and_send_query(int M, int N, int b, int L) {
    auto random_vector = create_random_vector(N,b,3,L);
    create_and_query(random_vector,L,M,N,b);
}

// 创建随机向量
vector<vector<vector<int>>> leader::create_random_vector(int row, int col, int dim, int L) {
    vector<vector<vector<int>>> random_vector(row,vector<vector<int>>(col));
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++){
        random_vector[i][j] = generate_random_vector(L, dim);
        }
    }
    return random_vector;
}

// 创建并执行查询
void leader::create_and_query(const vector<vector<vector<int>>>& random_vector, int L,int M,int N, int b) {
    for (int i = 0; i < M - 1; i++){
        for (int j = 1; j < N; j++) {
            for (int ell = 0; ell < b; ell++) {
                for (int k = 0; k < 3; k++){
                    leader_send[i][0][j][ell][k] = random_vector[j][ell][k];
                    leader_send[i][j][0][ell][k] = random_vector[j][ell][k] + pre_query_vectors[j][ell][k];
                }
            }
        }
    }
}

// 计算交集
vector<int> leader::calculate_intersection(int M, int L) {
    vector<int> intersection;
    for (auto& [element, position] : element_to_position) {
        vector<int> z;
        int j = position[0];
        int ell = position[1];
        for (int i = 0; i < M - 1; i++) {
            z.push_back(leader_recv[i][j][0][ell] - leader_recv[i][0][j][ell]);
        }
        int e = accumulate(z.begin(), z.end(), 0) % L;
        // cout << e << " ";
        if (e == 0) {
            intersection.push_back(element);
        }
    }
     // cout << endl;
    return intersection;
}
