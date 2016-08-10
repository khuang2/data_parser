#include <string>
using namespace std;

typedef struct{
	string instrument;
	unsigned int qty;//uint32
	int side;//unit8 1=buy, 2=sell
	unsigned long client_id;//uint64 *
	unsigned int time_in_force;//uint8, 1=IOC, 2=GFD
	string trader;
}EtryMessage;

typedef struct{
	unsigned int order_id;//uint 32 *
	unsigned long client_id;//uint 64 
	unsigned int order_status;//1=good, 2=reject

}AckMessage;

typedef struct{
	unsigned int order_id;//uint 32 *
	unsigned int qty;//uint 32
	unsigned int fill_qty;
}FillMessage;

class Parser{
private:
	int packetNum;
	int entryNum;
	int ackNum;
	int fillNum;
	vector<FillMessage> fillMessageList;
	unordered_map<unsigned int,unsigned long> order_client;
	unordered_map<unsigned long, EtryMessage> client_entry;
	unordered_map<string, unsigned int>trader_qty;
	unordered_map<unsigned int,unsigned int>order_status;
	unordered_map<string, int>trader_GFDvolume;
	void handleFillMessage(char* buffer, int& begin);
	void handleAckMessage(char* buffer, int& begin);
	void handleEntryMessage(char* buffer, int& begin);
public:
	Parser(string stream_file);
	unordered_map<string,unsigned int> instrument_volume;
	static unsigned long readALE(char* buffer, int& begin, int size); 
	int getPacketNum();
	int getEntryNum();
	int getAckNum();
	int getFillNum();
	string mostActiveTrader();
	string mostLiquidityTrader();
	void tradesPerInstrument();
	
};
