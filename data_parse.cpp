#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include "data_parse.h"
using namespace std;


Parser::Parser(string stream_file){
	packetNum = 0;
	entryNum = 0;
	ackNum = 0;
	fillNum = 0;
    ifstream is (stream_file, ios::in | ios::binary);
	if(is){
		is.seekg (0, is.end);
		int length = is.tellg();
		is.seekg (0, is.beg);

		char * buffer = new char [length];
		is.read (buffer,length);
		string stream(buffer,length);
		string marker = "ST";
		string termination = "DBDBDBDB";
		bool flag = true;
		int i = 0;
		while(i<length){
			string start = stream.substr(i,2);
			if(start.compare("ST")==0){
				packetNum++;
				i+=2;
				int type = buffer[i];
				i+=20;
				switch(type){
				case 1:
					entryNum++;
					handleEntryMessage(buffer, i);
					break;
				case 2:
					ackNum++;
					handleAckMessage(buffer, i);
					break;
				case 3:
					fillNum++;
					handleFillMessage(buffer, i);
					break;
				}


				int found = stream.find(termination, i);//find termination
				if(found!=string::npos){
					i =found+8;
				}
			}else{
				i++;
			}
		}
	}
}

void Parser::handleFillMessage(char* buffer, int& begin){ //start from the positon of order_id of fill message
	FillMessage fm;
	int sumOfQty =  0;
	fm.order_id = readALE(buffer, begin, 4);//read order id
	begin+=8;
	sumOfQty+=readALE(buffer,begin,4);
	fm.fill_qty = sumOfQty;
	int no_of_contras = buffer[begin++];
	for(int k = 0; k < no_of_contras; k++){
		begin+=4;
		sumOfQty+=readALE(buffer,begin,4);
	}
	fm.qty=sumOfQty;
	
	fillMessageList.push_back(fm);
}

void Parser::handleAckMessage(char* buffer, int& begin){//start from the position of order_id of ack message
	unsigned int orderId = readALE(buffer, begin, 4);
	unsigned long clientId = readALE(buffer, begin, 8);
	order_client[orderId] = clientId;
	order_status[orderId] = buffer[begin++];
}

void Parser::handleEntryMessage(char* buffer, int& begin){//start from the position of price of entry message
	begin+=8;
	EtryMessage entryMessage;
	entryMessage.qty = readALE(buffer, begin, 4);
	string instrument = "";
	for(int k = 0; k < 10; k++){
		instrument+=buffer[begin++];
	}
	int side = buffer[begin++];
	unsigned long clientId = readALE(buffer, begin, 8);
	unsigned int time_in_force = buffer[begin++];
	string trader = "";
	trader+=buffer[begin];
	trader+=buffer[begin+1];
	trader+=buffer[begin+2];
	begin+=3;
	entryMessage.instrument=instrument;
	entryMessage.side=side;
	entryMessage.client_id=clientId;
	entryMessage.time_in_force=time_in_force;
	entryMessage.trader=trader;
	client_entry[clientId]=entryMessage;
}
/*read in Little-endian*/
unsigned long Parser::readALE(char* buffer, int& begin, int size){
	int half = size/2;
	unsigned long res = 0;
	for(int k = half-1; k>=0; k--){
		res+=buffer[begin+k];
		if(k!=0)
			res<<8;
	}
	begin+=half;
	res<<(8*half);
	unsigned long res_temp = 0;
	for(int k = half-1; k>=0;k--){
		res_temp+=buffer[begin+k];
		if(k!=0)
			res_temp<<8;
	}
	begin+=half;
	res+=res_temp;
	return res;
}

string Parser::mostActiveTrader(){
	for(vector<FillMessage>::iterator itr = fillMessageList.begin(); itr!=fillMessageList.end();itr++){
		unsigned int order_id = (*itr).order_id;
		unsigned int qty = (*itr).qty;
		unsigned long clientId = order_client[order_id];
		string trader_tag = client_entry[clientId].trader;
		if(trader_qty.count(trader_tag)>0){
			trader_qty[trader_tag]+=qty;
		}else{
			trader_qty[trader_tag]=qty;
		}
	}
	int largestFilledVolume = 0;
	string mostActiveTrader="";
	for(auto i=trader_qty.begin();i!=trader_qty.end();++i){
		if(i->second>largestFilledVolume){
			largestFilledVolume = i->second;
			mostActiveTrader=i->first;
		}
	}
	return mostActiveTrader;
}

string Parser::mostLiquidityTrader(){
	for(auto itr = order_status.begin(); itr!=order_status.end();++itr){
		if(itr->second==1){
			unsigned long clientId = order_client[itr->first];
			if(client_entry.count(clientId)>0&&client_entry[clientId].time_in_force==2){
				if(trader_GFDvolume.count(client_entry[clientId].trader)>0){
					trader_GFDvolume[client_entry[clientId].trader]+=client_entry[clientId].qty;
				}else{
					trader_GFDvolume[client_entry[clientId].trader]=client_entry[clientId].qty;
				}
			}
		}
	}
	int largestGFDvolume = 0;
	string mostLiquidityTrader = "";
	for(auto i = trader_GFDvolume.begin();i!=trader_GFDvolume.end();++i){
		if(i->second>largestGFDvolume){
			largestGFDvolume = i->second;
			mostLiquidityTrader = i->first;
		}
	}
	return mostLiquidityTrader;
}

void Parser::tradesPerInstrument(){
	for(vector<FillMessage>::iterator itr = fillMessageList.begin(); itr!=fillMessageList.end();itr++){
		unsigned int order_id = (*itr).order_id;
		unsigned int volume = (*itr).fill_qty;
		unsigned long clientId = order_client[order_id];
		string instrument = client_entry[clientId].instrument;
		if(instrument_volume.count(instrument)>0){
			instrument_volume[instrument]+=volume;
		}else{
			instrument_volume[instrument]=volume;
		}
	}
}

int Parser::getPacketNum(){
	return this->packetNum;
}
int Parser::getEntryNum(){
	return this->entryNum;
}
int Parser::getAckNum(){
	return this->ackNum;
}
int Parser::getFillNum(){
	return this->fillNum;
}

int main(int argc, char *argv[]){
	if(argc>2||argc<2)
		cout<<"please enter correct argument"<<endl;
	else{
		string file_stream = argv[1];
		Parser parser(file_stream);//Q1,2
		int total_packets = parser.getPacketNum();
		int order_entry_msg_count = parser.getEntryNum();
		int order_ack_msg_count = parser.getAckNum();
		int order_fill_msg_count = parser.getFillNum();
		string most_active_trader_tag = parser.mostActiveTrader();
		string most_liquidity_trader_tag = parser.mostLiquidityTrader();
		//printf("%u,%u,%u,%u,%s,&s",total_packets,order_entry_msg_count,order_ack_msg_count,order_fill_msg_count,most_active_trader_tag,most_liquidity_trader_tag);
		cout<<total_packets<<", "<<order_entry_msg_count<<", "<<order_ack_msg_count<<", "<<order_fill_msg_count<<", "<<most_active_trader_tag<<", "<<most_liquidity_trader_tag<<", ";
		parser.tradesPerInstrument();
		for(auto itr = parser.instrument_volume.begin();itr!=parser.instrument_volume.end();++itr){
			cout<<itr->first<<":"<<itr->second<<", ";
		}
	}
}