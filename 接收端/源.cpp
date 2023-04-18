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

//����ȫ�ֱ���
#define send_IP "127.0.0.1"
#define recv_IP "127.0.0.1"
#define send_Port 6000
#define recv_Port 53432
SOCKET sockRecv;
SOCKADDR_IN addrSend;
SOCKADDR_IN addrRecv;
const int max_size = 1024;//һ�����ݰ������������
int ack = 0;//ACK

//��ȡ��ǰϵͳ���ں�ʱ��
char* get_time()
{
	time_t now = time(0); // �� now ת��Ϊ�ַ�����ʽ 
	char* dt = ctime(&now);
	return dt;
}

//�������ݰ�
struct packet
{
	char flag;//��־λ '0':SYN;'1':ACK;'2':FIN
	bool is_final_file;//������ݰ��Ƿ�Ϊ�ļ����һ�� true:�ǣ�false:��
	bool not_empty;//���ݰ��ǿձ�־ true:�ǿ�;false:��
	DWORD send_ip, recv_ip;
	u_short send_port, recv_port;
	int Seq;//���
	int ACK;//Ӧ�����
	int len;//����
	u_short checksum;//16λ��У���
	char data[max_size];//�������

	packet(); //���캯��
};

//���ݰ���ʼ��
packet::packet()
{
	//���������ݰ�����Ϊȫ��
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

//����У���
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

//��֤У���
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

//recvfrom����
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

//sendto����,�����sendto����������ACK��У���
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

//��������
bool recv_Conn()
{
	packet send_syn, recv_ack;
	cout << "[log]" << get_time() << ">>>" << "���ڼ���......" << endl;

	//���ս�������
	while (1)
	{
		if (my_recvfrom(send_syn))
		{
			if (send_syn.not_empty && send_syn.flag == '0')
			{
				cout << "[log]" << get_time() << ">>>" << "�ɹ��յ���������" << endl;
				break;
			}
		}
		else
			Sleep(10);
	}

	//���ͻ�Ӧ
	if (my_sendto(recv_ack))
	{
		cout << "[log]" << get_time() << ">>>" << "�ɹ�Ӧ�����ӳɹ���" << endl;
		return true;
	}
	else {
		cout << "[log]" << get_time() << ">>>" << "Ӧ��ʧ�ܣ�����ʧ��" << endl;
		return false;
	}
}



//��������
bool recv_Clo()
{
	packet send_fin, recv_fin;
	while (1)
	{
		if (my_recvfrom(send_fin))
		{
			if (send_fin.not_empty && send_fin.flag == '2')
			{
				cout << "[log]" << get_time() << ">>>" << "���յ���������" << endl;
				break;
			}
		}
		else
			Sleep(10);
	}

	if (my_sendto(recv_fin)) {
		cout << "[log]" << get_time() << ">>>" << "�ɹ�Ӧ�𣬶����ɹ�!" << endl;
		return true;
	}
	else {
		cout << "[log]" << get_time() << ">>>" << "Ӧ��ʧ�ܣ�����ʧ��!" << endl;
		return false;
	}
}

//ͣ�Ƚ���
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
	int state1, state2;  //�����������ֵ
	bool state3, state4;

	WORD wVersionRequested = MAKEWORD(2, 2); //������ϣ��ʹ�õ�socket����߰汾
	WSADATA wsaData;  //���õ�Socket����ϸ��Ϣ��ͨ��WSAStartup������ֵ
	//��ʼ��Socket DLL��Э��ʹ�õ�Socket�汾����ʼ���ɹ��򷵻�0������Ϊ�����int������
	state1 = WSAStartup(wVersionRequested, &wsaData);
	//�жϳ�ʼ��dll�Ƿ�ɹ�
	if (state1 == 0)
		cout << "[log]" << get_time() << ">>>" << "���ն˳�ʼ��Socket DLL�ɹ�" << endl;
	else
		cout << "[log]" << get_time() << ">>>" << "���ն˳�ʼ��Socket DLLʧ��" << endl;

	//�����׽��ַ�����
	u_long imode = 1;
	ioctlsocket(sockRecv, FIONBIO, &imode);

	//�������ն�socket�����󶨴����Э��UDP�����ݱ��׽��֣�IPv4
	sockRecv = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//������ն˵�ַ
	addrRecv.sin_family = AF_INET;  //IPv4��ַ����
	addrRecv.sin_addr.s_addr = inet_addr(recv_IP);  //���ñ���ipΪ���ն�ip
	addrRecv.sin_port = htons(recv_Port);   //�˿ں�Ϊ1234
	
	//�󶨵�ַ
	state2 = bind(sockRecv, (SOCKADDR*)&addrRecv, sizeof(SOCKADDR));
	//�жϰ󶨱��ص�ַ�Ƿ�ɹ�
	if (state2 == 0)
		cout << "[log]" << get_time() << ">>>" << "���ն˰󶨱��ص�ַ�ɹ�" << endl;
	else
		cout << "[log]" << get_time() << ">>>" << "���ն˰󶨱��ص�ַʧ�ܣ�" << endl;

	//���巢�Ͷ˵�ַ
	addrSend.sin_family = AF_INET;
	addrSend.sin_addr.S_un.S_addr = inet_addr(send_IP);
	addrSend.sin_port = htons(send_Port);

	//��������
	state3 = recv_Conn();

	//�����ļ�
	if (state3)
	{
		//�����ļ�
		ofstream file_storage;
		string path;
		cout << "[log]" << get_time() << ">>>" << "��ʼ�����ļ�......" << endl;
		cout << "[log]" << get_time() << ">>>" << "����洢·����";
		cin >> path;
		file_storage.open(path, ofstream::out | ios::binary);
		//int q = 0;
		while (1)
		{
			packet packet1, packet2;
			//cout << "��ʼ���յ�" << q << "��";
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
			//cout << "��" << q << "��������" << endl;q++;
		}

		//�ر�ofstream
		file_storage.close();

		//���ֶ���
		recv_Clo();
	}
	
	//����ʹ��Socket���ͷ�Socket DLL��Դ
	closesocket(sockRecv);
	WSACleanup();
	return 0;
}