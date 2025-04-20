#ifndef LEADER_H
#define LEADER_H
#include <map>
#include <vector>
#include "public_function.h"

class leader {
public:
    std::vector<std::vector<int>> schedule_hash_tables;
    std::vector<std::vector<std::vector<int>>> pre_query_vectors; 
    map<int,vector<int>> element_to_position;
    std::vector<std::vector<std::vector<std::vector<std::vector<int>>>>> leader_send;
    std::vector<std::vector<std::vector<std::vector<int>>>> leader_recv;
    int leader_id;

    // 构造函数声明
    leader(std::vector<std::vector<int>> schedule_hash_tables, std::vector<std::vector<std::vector<int>>> pre_query_vectors, map<int,vector<int>> element_to_position, int leader_id, int M, int N, int b);

    // 成员函数声明
    void create_and_send_query(int M, int N, int b, int L);
    std::vector<std::vector<std::vector<int>>> create_random_vector(int row, int col, int dim, int L);
    void create_and_query(const vector<vector<vector<int>>>& random_vector, int L,int M,int N, int b);
    std::vector<int> calculate_intersection(int M, int L);
};

#endif // LEADER_H

