#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<fstream>
#include<string>
#include<ctime>
#include<vector>
#include<winsock2.h>
#pragma comment(lib,"ws2_32.lib")
using namespace std;

//定义全局变量
#define send_IP "127.0.0.1"
#define recv_IP "127.0.0.1"
#define send_Port 6000
#define recv_Port 53432
SOCKET sockRecv;
SOCKADDR_IN addrSend;
SOCKADDR_IN addrRecv;
const int max_size = 1024;//一个数据包的最大数据量
int ack = 0;//ACK

//获取当前系统日期和时间
char* get_time()
{
	time_t now = time(0); // 把 now 转换为字符串形式 
	char* dt = ctime(&now);
	return dt;
}

//定义数据包
struct packet
{
	char flag;//标志位 '0':SYN;'1':ACK;'2':FIN
	bool is_final_file;//标记数据包是否为文件最后一组 true:是；false:否
	bool not_empty;//数据包非空标志 true:非空;false:空
	DWORD send_ip, recv_ip;
	u_short send_port, recv_port;
	int Seq;//序号
	int ACK;//应答序号
	int len;//长度
	u_short checksum;//16位的校验和
	char data[max_size];//存放数据

	packet(); //构造函数
};

//数据包初始化
packet::packet()
{
	//把整个数据包设置为全零
	memset(this, 0, sizeof(packet));
	send_ip = DWORD(send_IP);
	recv_ip = DWORD(recv_IP);
	send_port = send_Port;
	recv_port = recv_Port;
	Seq = 0;
	ACK = 0;
	len = 0;
	checksum = 0;
}

//计算校验和
void set_checksum(packet& packet)
{
	memset(&packet.checksum, 0, sizeof(packet.checksum));
	int count = (sizeof(packet) + 1) / 2;
	u_short* bit16 = (u_short*)(&packet);
	u_long checksum = 0;
	for (int i = 0; i < count; i++)
	{
		checksum += *(bit16++);
		if (checksum & 0xffff0000)
		{
			checksum -= 0xffff;
			checksum += 1;
		}
	}
	packet.checksum = ~checksum;
	return;
}

//验证校验和
bool is_checksum(packet& packet)
{
	int count = (sizeof(packet) + 1) / 2;
	u_short* bit16 = (u_short*)(&packet);
	u_long checksum = 0;
	for (int i = 0; i < count; i++) {
		checksum += *(bit16++);
		if (checksum & 0xffff0000) {
			checksum -= 0xffff;
			checksum += 1;
		}
	}

	if ((checksum == 0xffff))
		return true;
	else
		return false;
}

//recvfrom函数
bool my_recvfrom(packet& packet)
{
	int addr_len = sizeof(sockaddr_in);
	memset(packet.data, 0, sizeof(packet.data));
	int state = recvfrom(sockRecv, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addrSend, &addr_len);
	if (state == SOCKET_ERROR)
		return false;
	else
		return true;
}

//sendto函数,相比于sendto函数，增加ACK，校验和
bool my_sendto(packet& packet)
{
	packet.ACK = ack++;
	packet.flag = '1';
	set_checksum(packet);
	int state;
	state = sendto(sockRecv, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&addrSend, sizeof(sockaddr));
	if (state == SOCKET_ERROR)
		return false;
	else
		return true;
}

//建连过程
bool recv_Conn()
{
	packet send_syn, recv_ack;
	cout << "[log]" << get_time() << ">>>" << "正在监听......" << endl;

	//接收建连请求
	while (1)
	{
		if (my_recvfrom(send_syn))
		{
			if (send_syn.not_empty && send_syn.flag == '0')
			{
				cout << "[log]" << get_time() << ">>>" << "成功收到连接请求" << endl;
				break;
			}
		}
		else
			Sleep(10);
	}

	//发送回应
	if (my_sendto(recv_ack))
	{
		cout << "[log]" << get_time() << ">>>" << "成功应答，连接成功！" << endl;
		return true;
	}
	else {
		cout << "[log]" << get_time() << ">>>" << "应答失败，连接失败" << endl;
		return false;
	}
}



//断连过程
bool recv_Clo()
{
	packet send_fin, recv_fin;
	while (1)
	{
		if (my_recvfrom(send_fin))
		{
			if (send_fin.not_empty && send_fin.flag == '2')
			{
				cout << "[log]" << get_time() << ">>>" << "接收到断连请求！" << endl;
				break;
			}
		}
		else
			Sleep(10);
	}

	if (my_sendto(recv_fin)) {
		cout << "[log]" << get_time() << ">>>" << "成功应答，断连成功!" << endl;
		return true;
	}
	else {
		cout << "[log]" << get_time() << ">>>" << "应答失败，断连失败!" << endl;
		return false;
	}
}

//停等接收
bool stop_wait_recv(packet& packet1, packet& packet2)
{
	while (1)
	{
		my_recvfrom(packet1);
		if (packet1.not_empty)
		{
			if (is_checksum(packet1))
			{
				my_sendto(packet2);
				memset((char*)&packet2, 0, sizeof(packet2));
				return true;
			}
			else
				continue;
		}
	}
	return false;
}


int main()
{
	int state1, state2;  //存各函数返回值
	bool state3, state4;

	WORD wVersionRequested = MAKEWORD(2, 2); //调用者希望使用的socket的最高版本
	WSADATA wsaData;  //可用的Socket的详细信息，通过WSAStartup函数赋值
	//初始化Socket DLL，协商使用的Socket版本；初始化成功则返回0，否则为错误的int型数字
	state1 = WSAStartup(wVersionRequested, &wsaData);
	//判断初始化dll是否成功
	if (state1 == 0)
		cout << "[log]" << get_time() << ">>>" << "接收端初始化Socket DLL成功" << endl;
	else
		cout << "[log]" << get_time() << ">>>" << "接收端初始化Socket DLL失败" << endl;

	//设置套接字非阻塞
	u_long imode = 1;
	ioctlsocket(sockRecv, FIONBIO, &imode);

	//创建接收端socket，并绑定传输层协议UDP，数据报套接字，IPv4
	sockRecv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//定义接收端地址
	addrRecv.sin_family = AF_INET;  //IPv4地址类型
	addrRecv.sin_addr.s_addr = inet_addr(recv_IP);  //设置本机ip为接收端ip
	addrRecv.sin_port = htons(recv_Port);   //端口号为1234
	
	//绑定地址
	state2 = bind(sockRecv, (SOCKADDR*)&addrRecv, sizeof(SOCKADDR));
	//判断绑定本地地址是否成功
	if (state2 == 0)
		cout << "[log]" << get_time() << ">>>" << "接收端绑定本地地址成功" << endl;
	else
		cout << "[log]" << get_time() << ">>>" << "接收端绑定本地地址失败！" << endl;

	//定义发送端地址
	addrSend.sin_family = AF_INET;
	addrSend.sin_addr.S_un.S_addr = inet_addr(send_IP);
	addrSend.sin_port = htons(send_Port);

	//建立连接
	state3 = recv_Conn();

	//接收文件
	if (state3)
	{
		//接收文件
		ofstream file_storage;
		string path;
		cout << "[log]" << get_time() << ">>>" << "开始接收文件......" << endl;
		cout << "[log]" << get_time() << ">>>" << "输入存储路径：";
		cin >> path;
		file_storage.open(path, ofstream::out | ios::binary);
		//int q = 0;
		while (1)
		{
			packet packet1, packet2;
			//cout << "开始接收第" << q << "组";
			stop_wait_recv(packet1, packet2);
			if (!packet1.is_final_file)
			{
				char* file = new char[max_size];
				for (int i = 0; i < max_size; i++)
					file[i] = packet1.data[i];
				file_storage.write(file, max_size);
				delete[] file;
			}
			else
			{
				char* file = new char[packet1.len];
				for (int i = 0; i < packet1.len; i++)
					file[i] = packet1.data[i];
				file_storage.write(file, packet1.len);
				delete[] file;
				break;
			}
			//cout << "第" << q << "组接收完成" << endl;q++;
		}

		//关闭ofstream
		file_storage.close();

		//挥手断连
		recv_Clo();
	}
	
	//结束使用Socket，释放Socket DLL资源
	closesocket(sockRecv);
	WSACleanup();
	return 0;
}