#ifndef CHANNEL_H
#define CHANNEL_H

#include "leader.h"
#include "client.h"
#include "database.h"

class channel {
public:
    // 默认构造函数
    channel();

    // 将 leader 数据发送到数据库
    void leader_to_database(leader& ld, database& db);

    // 将 client 数据发送到数据库
    void client_to_database(client& cl, database& db);

    void database_to_database(database& db1, database& db2);
    // 将数据库数据发送到 leader
    void database_to_leader(database& db, leader& ld);
};

#endif // CHANNEL_H

