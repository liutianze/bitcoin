
#include <stdio.h>
#include <mysql.h>
#include <errmsg.h>
#include <mysql_db.h>


static BtchDB* g_btch_db = 0;

BtchDB* BtchDB::GetInstance()
{
    if(!g_btch_db){
        g_btch_db = new BtchDB();
    }

    return g_btch_db;
}

BtchDB::BtchDB():
_addConn(0), _enableConn(0), _disableConn(0), _ucAddConn(0)
{

	
}

bool BtchDB::Init(const char* db, const char* table, const char* ucTable)
{
    _db = db;
    _table = table;
    _ucTable = ucTable;
    _addConn = Connect();
    _enableConn = Connect();
    _disableConn = Connect();
    _ucAddConn = Connect();

}


bool BtchDB::ReConnect(void** conn)
{
    if(*conn != 0){
         MYSQL *c = (MYSQL*)(*conn);
         mysql_close(c);
    }

    *conn = 0;

    void* c = Connect();
    if(!c){
        return false;
    }

    *conn = c;
    return true;   
}

void* BtchDB::Connect()
{
    MYSQL *conn;
    MYSQL_RES *res;
	MYSQL_ROW row;
	const char *server = "127.0.0.1";
	const char *user = "root";
	const char *password = "sd-9898w";  
	const char *database = _db;//"bitcoin";
	conn = mysql_init(NULL);


    if(!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0))
	{
		fprintf(stderr, "%s\n", mysql_error(conn));
        return 0;
	}

    return conn;
}



bool BtchDB::Execute(void** conn, const char* sql)
{
    if(*conn == 0){
        if(!ReConnect(conn)){
            return false;
        }
    }
    
    bool toBreak = false;
    while (!toBreak)
    {
        MYSQL* c = (MYSQL*)(*conn);
        if (mysql_query(c, sql))
        {
            int err = mysql_errno(c);
            switch (err)
            {
                case CR_SERVER_GONE_ERROR:
                case CR_SERVER_LOST:
                    {
                         if(ReConnect(conn)){
                             continue;
                         }   
                   }
            }

            fprintf(stderr, "%s\n", mysql_error(c));
            return false;
        }
        else{
            return true;
        }
    }

    return true;
}


long BtchDB::Insert(void** conn, const char* sql)
{
    if(*conn == 0){
        if(!ReConnect(conn)){
            return false;
        }
    }
    
    bool toBreak = false;
    while (!toBreak)
    {
        MYSQL* c = (MYSQL*)(*conn);
        if (mysql_query(c, sql))
        {
            int err = mysql_errno(c);
            switch (err)
            {
                case CR_SERVER_GONE_ERROR:
                case CR_SERVER_LOST: 
                    {
                         if(ReConnect(conn)){
                             continue;
                         }   
                   }
            }

            fprintf(stderr, "%s\n", mysql_error(c));
            return 0;
        }
        else{
            return mysql_insert_id(c);
        }
    }

    return 0;
}

bool BtchDB::AddAddress(int type, const char *ip, int port)
{
    char buffer[4096];
    const char* sql = "insert into %s(`type`, `ip`,`port`,`status`) values(%d, '%s', %d, 0) on duplicate \
                    key update `status`=0";

    sprintf(buffer, sql, _table, type, ip, port);
    return Execute( &_addConn, buffer);
}

bool BtchDB::EnableAddress(int type, const char *ip, int port)
{
    char buffer[4096];
    const char* sql = "insert into %s(`type`, `ip`,`port`,`status`) values(%d, '%s', %d, 1) on duplicate \
                    key update `status`=1";

    sprintf(buffer, sql, _table, type, ip, port);
    return Execute( &_enableConn, buffer);

}

bool BtchDB::DisableAddress(int type, const char *ip, int port)
{
    char buffer[4096];
    const char* sql = "insert into %s(`type`,`ip`,`port`,`status`) values(%d, '%s', %d, 1) on duplicate \
                    key update `status`=2";

    sprintf(buffer, sql, _table, type, ip, port);
    return Execute(&_disableConn ,buffer);

}

bool BtchDB::AddUnCheckedAddress(int type, const char *ip, int port)
{
    char buffer[4096];
    const char* sql = "insert into %s(`type`, `ip`,`port`,`status`) values(%d, '%s', %d, 0) on duplicate \
                    key update `status`=0";

    sprintf(buffer, sql, _ucTable, type, ip, port);
    return Execute(&_ucAddConn ,buffer);
}



bool BtchDB::AddBlock(BtchBlock* block)
{
    char buffer[4096];
    const char* sql = "insert into block(`block_hash`,`status`) values('%s', 2) on duplicate key update `status` = 2";

    sprintf(buffer, sql, block->block_hash);

    long block_id = Insert(&_blockConn, buffer);
    if(!block_id){
        fprintf(stderr, "%s\n", "insert block failed, block hash:%s", block->block_hash);
        return false;
    }
    

    for(int i = 0; i < block->tx_count; ++i){
        BtchTransaction* tx = block->tx + i;
        bool flag = AddBlockTransaction(tx, block_id);
        if(!flag){
            fprintf(stderr, "%s\n", "insert transaction failed, tx hash:%s", tx->tx_hash);
            return false;
        }
    }

    if(block->coin_base){
        return AddBlockTransactionCoinBase(block->coin_base, block_id);
    }

    return true;

}

bool BtchDB::AddTransaction(BtchTransaction* tx)
{
    return AddTxTransaction(tx);
}

// create table tx(
//     id bigint auto_increment not null,
//     tx_hash varchar(128) not null,
//     block_id bigint not null,
//     is_coinbase tinyint(3) not null,
//     status tinyint(3) not null,
//     primary key (`id`),
//     unique key(`tx_hash`),
//     CONSTRAINT block_id FOREIGN KEY (block_id) REFERENCES block (id)
// )engine=myisam;





// struct BtchTransaction{
//     std::string             tx_hash;
//     int                     tx_in_count;
//     int                     tx_out_out;
//     BtchTransactionIn*      tx_in;
//     BtchTransactionOut*     tx_out;
// };


bool BtchDB::AddBlockTransaction(BtchTransaction* tx, long block_id)
{
    char buffer[4096];
    const char* sql = "insert into tx(`tx_hash`, `block_id`, `is_coinbase`, `status`,) values('%s',\
                %ld, 1, 2) on duplicate key update status = 2";
    sprintf(buffer, sql, tx->tx_hash.c_str(), block_id);

    long tx_id = Insert(&_blockConn, buffer);
    if (!tx_id)
    {
        fprintf(stderr, "insert tx failed, block id:%ld, tx hash:%s\n", block_id, tx->tx_hash.c_str());
        return false;
    }

    for (int i = 0; i < tx->tx_in_count; ++i)
    {
        if (!AddTxTransactionIn(&_blockConn, tx->tx_in + i, tx_id, i))
        {
            fprintf(stderr, "insert tx in failed, block id:%ld, tx hash:%s, i:%d\n", block_id, tx->tx_hash.c_str(), i);
            return false;
        }
    }

    for (int i = 0; i < tx->tx_out_out; ++i)
    {
        if (!AddTxTransactionOut(&_txConn, tx->tx_out + i, tx_id, i))
        {
            fprintf(stderr, "insert tx out failed, block id:%ld, tx hash:%s, i:%d\n", block_id, tx->tx_hash.c_str(), i);
            return false;
        }
    }

    return true;
}


bool BtchDB::AddTxTransaction(BtchTransaction* tx)
{
    char buffer[4096];
    const char* sql = "insert into tx(`tx_hash`, `is_coinbase`, `status`,) values('%s', 0, 1) \
        on duplicate key update `status` = `status`";
    sprintf(buffer, sql, tx->tx_hash.c_str());

    long tx_id = Insert(&_txConn, buffer);
    if (!tx_id)
    {
        fprintf(stderr, "insert tx tx failed, tx hash:%s\n", tx->tx_hash.c_str());
        return false;
    }

    for (int i = 0; i < tx->tx_in_count; ++i)
    {
        if (!AddTxTransactionIn(&_txConn, tx->tx_in + i, tx_id, i))
        {
            fprintf(stderr, "insert tx tx in failed, tx hash:%s, i:%d\n",tx->tx_hash.c_str(), i);
            return false;
        }
    }

    for (int i = 0; i < tx->tx_out_out; ++i)
    {
        if (!AddTxTransactionOut(&_txConn, tx->tx_out + i, tx_id, i))
        {
            fprintf(stderr, "insert tx tx out failed, tx hash:%s, i:%d\n",tx->tx_hash.c_str(), i);
            return false;
        }
    }

    return true;
}

// struct BtchTransactionIn{
//     std::string     from_tx_hash;
//     long            vout;
// };



// create table tx_in(
//     id bigint auto_increment not null,
//     tx_id bigint not null,
//     from_tx_hash varchar(65) not null,
//     vout int not null,
//     primary key(id),
//     KEY tx_id (tx_id),
//     CONSTRAINT key_id FOREIGN KEY (tx_id) REFERENCES tx (id)
// )engine=myisam;




bool BtchDB::AddTxTransactionIn(void** conn, BtchTransactionIn* tx_in, long tx_id, int index)
{
    char buffer[4096];
    const char* sql = "insert into tx_in(`tx_id`, `indexof_tx`, `from_tx_hash`, `vout`, `status`) values(%ld,%d\
                '%s', %ld, 1) on duplicate key update set `status` = `status`";

    sprintf(buffer, sql, tx_id, index, tx_in->from_tx_hash, tx_in->vout);
    return Insert(conn, buffer) > 0;
}

// struct BtchTransactionOut{
//     long            vout;
//     long            out_value;
//     std::string     out_address;
// };

// create table tx_out(
//     id bigint auto_increment not null,
//     tx_id bigint not null,
//     vout int not null,
//     out_value bigint not null,
//     out_address varchar(64) not null,
//     primary key(id),
//     KEY tx_id (tx_id),
//     CONSTRAINT key_id FOREIGN KEY (tx_id) REFERENCES tx (id)
// )engine=myisam;

bool BtchDB::AddTxTransactionOut(void** conn, BtchTransactionOut* tx_out, long tx_id, int index)
{
    char buffer[4096];
    const char* sql = "insert into tx_out(`tx_id`, `indexof_tx`, `vout`, `out_value`,`out_address`,`status`) values(%ld, %d\
                %ld, %ld, '%s', 1) on duplicate key update set `status` = `status`";

    sprintf(buffer, sql, tx_id, index, tx_out->vout, tx_out->out_value, tx_out->out_address);
    return Insert(conn, buffer) > 0;
}

// create table tx(
//     id bigint auto_increment not null,
//     tx_hash varchar(128) not null,
//     block_id bigint not null,
//     is_coinbase tinyint(3) not null,
//     status tinyint(3) not null,
//     primary key (`id`),
//     unique key(`tx_hash`),
//     CONSTRAINT block_id FOREIGN KEY (block_id) REFERENCES block (id)
// )engine=myisam;

// create table tx_coinbase(
//     id bigint auto_increment not null,
//     tx_id bigint not null,
//     coinbase_hash varchar(128),
//     coinbase_sequence bigint,
//     coinbase_value bigint not null,
//     out_address varchar(64) not null,
//     primary key(id),
//     KEY tx_id (tx_id),
//     CONSTRAINT key_id FOREIGN KEY (tx_id) REFERENCES tx (id)
// )engine=myisam;

bool BtchDB::AddBlockTransactionCoinBase(BtchTransactionCoinBase* tx, long block_id)
{
    char buffer[4096];
    const char* sql = "insert into tx(`tx_hash`, `block_id`, `is_coinbase`, `status`,) values('%s',\
                %ld, 1, 2) on duplicate key update status = 2";
    sprintf(buffer, sql, tx->tx_hash, block_id);


    long tx_id = Insert(&_blockConn, buffer);
    if(!tx_id){
        fprintf(stderr, "insert coinbase failed, block id:%ld, tx hash:%s\n", block_id, tx->tx_hash.c_str());
        return false;
    }

    sql = "insert into tx_coinbase(`tx_id`, `coinbase_hash`, `coinbase_sequence`, `coinbase_value`, `out_address`, `status`) values(%d,\
                %s, %ld, %ld, '%s', 2) on duplicate key update `status` = 2";

    sprintf(buffer, sql, tx_id, tx->coinbase_hash.c_str(), tx->coinbase_sequence, tx->coinbase_value, tx->out_address.c_str());

    long cb_id = Insert(&_blockConn, buffer);
    return cb_id > 0;
}