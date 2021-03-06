#include <unistd.h>
#include <tx_db.h>



BtchTransactionCoinBase* createTxCB(){
    BtchTransactionCoinBase* cb = new BtchTransactionCoinBase();
    cb->tx_hash = "fdskljafdsa";
    cb->coinbase_hash = "fdsafdsa";
    cb->coinbase_sequence = 1;
    cb->coinbase_value = 2;
    cb->out_address = "fdsafdsa";
    return cb;
}

BtchTransaction* createTx(){
    BtchTransaction*    tx = new BtchTransaction[1];

    tx->tx_hash = "fdjklsfljdsfiekcvdsifdsnvid";
    tx->tx_in_count = 4;
    tx->tx_out_count = 4;
    tx->is_coinbase = 0;
    
    BtchTransactionIn* ins = new BtchTransactionIn[4];
    BtchTransactionOut* outs = new BtchTransactionOut[4];

    for(int i = 0; i < 4; ++i){
        BtchTransactionIn* p = ins + i;
        p->from_tx_hash = "abcdefghijklmn";
        p->vin = i;
        p->sequence = 1;
        p->sig_hex = "fdsafdsa";
        p->from_tx_vout = 1;
        
    }

    for(int i = 0; i < 4;  ++i){
        BtchTransactionOut* p = outs + i;
        p->vout = 1;
        p->out_value = 2;
        p->out_address = "abcdefghijil";
        p->out_addresses = "abcdefghijil;ffdksla;";
        p->out_addr_count = 2;
        p->out_type ="test";
    }

    tx->tx_in = ins;
    tx->tx_out = outs;

    return tx;
}

bool testInsertTx(BtchTxDB* db){
    BtchTransaction*     tx = createTx();
    bool ret = db->AddTransaction(tx);
    delete[] tx;

    return ret;

}



bool testInsertBlock(BtchTxDB* db){
    BtchBlock* block = new BtchBlock();
    block->block_hash = "fdsafdsa";
    block->tx_count = 1;
    block->pre_block_hash = "fabfdsa";
    block->bits = 1;
    block->nonce = 1;
    block->time = 1;
    block->version = 1;

    block->tx = createTx();
    //block->coin_base = createTxCB();
    bool ret = db->AddBlock(block, 1);
    delete block;
    return ret;
}




int main(int argc, char** argv)
{
    BtchTxDB* db = BtchTxDB::GetInstance();
    db->Init("test");

    // if(!testInsertTx(db)){
    //     printf("insert tx failed\n");
    //     return -1;
    // }

    // printf("insert tx finish\n");

    if(!testInsertBlock(db)){
        printf("insert block failed\n");
        return -1;
    }

    printf("insert block finish\n");


    return 0;
}